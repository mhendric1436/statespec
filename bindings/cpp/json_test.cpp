#include "backend.hpp"
#include "external_system.hpp"
#include "feature_flag.hpp"
#include "log.hpp"
#include "memory/backend.hpp"
#include "metric.hpp"
#include "runtime/feature_flag_store.hpp"
#include "runtime/lease_store.hpp"
#include "runtime/log_sink.hpp"
#include "runtime/metric_sink.hpp"
#include "runtime/queue_store.hpp"
#include "runtime/workflow_store.hpp"
#include "schema_compatibility.hpp"

#include "catch2/catch_amalgamated.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

TEST_CASE("C++ binding JSON parses objects and arrays")
{
    auto parsed = statespec::backend::Json::parse(
        R"json({"active":true,"count":7,"tags":["a",null,2.5]})json"
    );

    REQUIRE(parsed.is_object());
    REQUIRE(parsed["active"].as_bool());
    REQUIRE(parsed["count"].as_int64() == 7);
    REQUIRE(parsed["tags"].is_array());
    REQUIRE(parsed["tags"].as_array().size() == 3);
    REQUIRE(parsed["tags"].as_array()[1].is_null());
    REQUIRE(parsed["tags"].as_array()[2].as_double() == 2.5);
}

TEST_CASE("C++ binding JSON decodes escapes and unicode")
{
    auto parsed = statespec::backend::Json::parse(
        R"json({"text":"line\nA\\\"","latin":"\u00e9","emoji":"\ud83d\ude00"})json"
    );

    REQUIRE(parsed["text"].as_string() == std::string("line\nA\\\""));
    REQUIRE(parsed["latin"].as_string() == std::string("\xC3\xA9"));
    REQUIRE(parsed["emoji"].as_string() == std::string("\xF0\x9F\x98\x80"));
}

TEST_CASE("C++ binding JSON canonical string is stable")
{
    auto value = statespec::backend::Json::object(
        {{"z", std::int64_t{1}}, {"a", statespec::backend::Json::array({true, "x"})}}
    );

    auto canonical = value.canonical_string();

    REQUIRE(canonical == R"json({"a":[true,"x"],"z":1})json");

    auto reparsed = statespec::backend::Json::parse(canonical);
    REQUIRE(reparsed == value);
}

TEST_CASE("C++ binding JSON object helpers distinguish missing fields")
{
    auto value = statespec::backend::Json::parse(R"json({"present":null,"name":"Alice"})json");

    REQUIRE(value.contains("present"));
    REQUIRE(value.find("present") != nullptr);
    REQUIRE(value.find("present")->is_null());
    REQUIRE(!value.contains("missing"));
    REQUIRE(value.find("missing") == nullptr);
}

TEST_CASE("C++ binding JSON rejects malformed input")
{
    REQUIRE_THROWS_AS(statespec::backend::Json::parse(""), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(statespec::backend::Json::parse("[1,]"), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(
        statespec::backend::Json::parse("{\"a\":1,\"a\":2}"), statespec::backend::JsonError
    );
    REQUIRE_THROWS_AS(statespec::backend::Json::parse("01"), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(statespec::backend::Json::parse("-01"), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(
        statespec::backend::Json::parse("\"raw\nnewline\""), statespec::backend::JsonError
    );
    REQUIRE_THROWS_AS(
        statespec::backend::Json::parse("\"\\uD800\""), statespec::backend::JsonError
    );
    REQUIRE_THROWS_AS(
        statespec::backend::Json::parse("\"\\uDC00\""), statespec::backend::JsonError
    );
    REQUIRE_THROWS_AS(statespec::backend::Json::parse("1e9999"), statespec::backend::JsonError);
    REQUIRE_THROWS_AS(
        statespec::backend::Json::object({{"a", 1}, {"a", 2}}), statespec::backend::JsonError
    );
    REQUIRE_THROWS_AS(
        statespec::backend::Json(std::numeric_limits<double>::infinity()),
        statespec::backend::JsonError
    );
}

TEST_CASE("C++ backend surfaces use typed JSON")
{
    auto document = statespec::backend::Json::object(
        {{"id", "user:1"}, {"active", true}, {"count", std::int64_t{3}}}
    );

    statespec::backend::VersionedRecord record{
        .collection = "users",
        .key = "user:1",
        .version = 3,
        .document = document,
    };

    REQUIRE(record.document.is_object());
    REQUIRE(record.document["id"].as_string() == "user:1");
    REQUIRE(record.document["active"].as_bool());
    REQUIRE(record.document["count"].as_int64() == 3);

    auto query = statespec::backend::Query::json_equals("$.active", statespec::backend::Json(true));
    REQUIRE(query.json_value.has_value());
    REQUIRE(query.json_value->is_bool());
    REQUIRE(query.json_value->as_bool());

    auto string_value = statespec::backend::IndexValue::string_value("alice@example.com");
    auto integer_value = statespec::backend::IndexValue::integer_value(42);
    auto decimal_value = statespec::backend::IndexValue::decimal_value(1.5);
    auto boolean_value = statespec::backend::IndexValue::boolean_value(false);
    auto timestamp_value = statespec::backend::IndexValue::timestamp_value("2026-05-12T00:00:00Z");

    REQUIRE(string_value.value.as_string() == "alice@example.com");
    REQUIRE(integer_value.value.as_int64() == 42);
    REQUIRE(decimal_value.value.as_double() == 1.5);
    REQUIRE(!boolean_value.value.as_bool());
    REQUIRE(timestamp_value.value.as_string() == "2026-05-12T00:00:00Z");
}

TEST_CASE("C++ schema compatibility helper classifies descriptor upgrades")
{
    auto base = statespec::backend::CollectionDescriptor{
        .name = "orders",
        .fields =
            {
                statespec::backend::FieldDescriptor{
                    .name = "tenant_id",
                    .type = statespec::backend::FieldType::String,
                    .type_name = "string",
                    .required = true,
                },
                statespec::backend::FieldDescriptor{
                    .name = "status",
                    .type = statespec::backend::FieldType::String,
                    .type_name = "string",
                    .required = false,
                },
            },
        .key_fields = {"tenant_id"},
        .indexes =
            {
                statespec::backend::IndexDescriptor{
                    .name = "by_status",
                    .fields = {"status"},
                    .unique = false,
                },
            },
        .schema_version = 1,
    };

    const auto identical = statespec::backend::compare_collection_descriptors(base, base);
    REQUIRE(identical.status == statespec::backend::SchemaCompatibilityStatus::Identical);
    REQUIRE(identical.compatible());

    auto require_reason = [&](const statespec::backend::CollectionDescriptor& requested,
                              statespec::backend::SchemaCompatibilityReason reason)
    {
        const auto result = statespec::backend::compare_collection_descriptors(base, requested);
        REQUIRE(result.status == statespec::backend::SchemaCompatibilityStatus::Incompatible);
        REQUIRE(
            std::any_of(
                result.differences.begin(), result.differences.end(),
                [&](const auto& difference) { return difference.reason == reason; }
            )
        );
    };

    auto optional_field = base;
    optional_field.schema_version = 2;
    optional_field.fields.push_back(
        statespec::backend::FieldDescriptor{
            .name = "description",
            .type = statespec::backend::FieldType::String,
            .type_name = "string",
            .required = false,
        }
    );
    const auto compatible =
        statespec::backend::compare_collection_descriptors(base, optional_field);
    REQUIRE(compatible.status == statespec::backend::SchemaCompatibilityStatus::Compatible);
    REQUIRE(compatible.compatible());

    auto non_unique_index = base;
    non_unique_index.schema_version = 2;
    non_unique_index.indexes.push_back(
        statespec::backend::IndexDescriptor{
            .name = "by_tenant_status",
            .fields = {"tenant_id", "status"},
            .unique = false,
        }
    );
    REQUIRE(
        statespec::backend::compare_collection_descriptors(base, non_unique_index).compatible()
    );

    auto required_field = optional_field;
    required_field.fields.back().required = true;
    require_reason(
        required_field, statespec::backend::SchemaCompatibilityReason::RequiredFieldAdded
    );

    auto changed_key = base;
    changed_key.schema_version = 2;
    changed_key.key_fields = {"tenant_id", "order_id"};
    require_reason(changed_key, statespec::backend::SchemaCompatibilityReason::KeyFieldsChanged);

    auto same_version = optional_field;
    same_version.schema_version = 1;
    require_reason(
        same_version, statespec::backend::SchemaCompatibilityReason::SchemaVersionNotIncremented
    );

    auto decreased_version = optional_field;
    decreased_version.schema_version = 0;
    require_reason(
        decreased_version, statespec::backend::SchemaCompatibilityReason::SchemaVersionDecreased
    );

    auto removed_field = base;
    removed_field.schema_version = 2;
    removed_field.fields.pop_back();
    require_reason(removed_field, statespec::backend::SchemaCompatibilityReason::FieldRemoved);

    auto changed_type = base;
    changed_type.schema_version = 2;
    changed_type.fields[1].type = statespec::backend::FieldType::Int;
    require_reason(changed_type, statespec::backend::SchemaCompatibilityReason::FieldTypeChanged);

    auto changed_type_name = base;
    changed_type_name.schema_version = 2;
    changed_type_name.fields[1].type_name = "Status";
    require_reason(
        changed_type_name, statespec::backend::SchemaCompatibilityReason::FieldTypeNameChanged
    );

    auto became_required = base;
    became_required.schema_version = 2;
    became_required.fields[1].required = true;
    require_reason(
        became_required, statespec::backend::SchemaCompatibilityReason::FieldBecameRequired
    );

    auto removed_index = base;
    removed_index.schema_version = 2;
    removed_index.indexes.clear();
    require_reason(removed_index, statespec::backend::SchemaCompatibilityReason::IndexRemoved);

    auto changed_index_fields = base;
    changed_index_fields.schema_version = 2;
    changed_index_fields.indexes[0].fields = {"tenant_id", "status"};
    require_reason(
        changed_index_fields, statespec::backend::SchemaCompatibilityReason::IndexFieldsChanged
    );

    auto changed_index_unique = base;
    changed_index_unique.schema_version = 2;
    changed_index_unique.indexes[0].unique = true;
    require_reason(
        changed_index_unique, statespec::backend::SchemaCompatibilityReason::IndexUniquenessChanged
    );

    auto unique_index_added = base;
    unique_index_added.schema_version = 2;
    unique_index_added.indexes.push_back(
        statespec::backend::IndexDescriptor{
            .name = "unique_status",
            .fields = {"status"},
            .unique = true,
        }
    );
    require_reason(
        unique_index_added, statespec::backend::SchemaCompatibilityReason::UniqueIndexAdded
    );
}

TEST_CASE("C++ feature flag bindings expose typed values and metadata")
{
    auto bool_value = statespec::backend::FeatureFlagValue::bool_value(true);
    auto string_value = statespec::backend::FeatureFlagValue::string_value("new");
    auto integer_value = statespec::backend::FeatureFlagValue::integer_value(100);
    auto decimal_value = statespec::backend::FeatureFlagValue::decimal_value(1.5);

    REQUIRE(bool_value.type() == statespec::backend::FeatureFlagType::Bool);
    REQUIRE(bool_value.as_bool().value());
    REQUIRE(!bool_value.as_string().has_value());
    REQUIRE(string_value.as_string() == "new");
    REQUIRE(integer_value.as_integer() == 100);
    REQUIRE(decimal_value.as_decimal() == 1.5);

    statespec::backend::FeatureFlagDefinition definition{
        .name = "NewScheduler",
        .type = statespec::backend::FeatureFlagType::Bool,
        .default_value = bool_value,
        .scope = statespec::backend::FeatureFlagScopeKind::Tenant,
        .owner = "platform",
        .description = "Route through the new scheduler",
        .expires = "2026-12-31",
    };

    REQUIRE(definition.name == "NewScheduler");
    REQUIRE(definition.default_value.as_bool() == true);
    REQUIRE(definition.scope == statespec::backend::FeatureFlagScopeKind::Tenant);

    statespec::backend::FeatureFlagRegisterDefinitionResult registration{
        .registered_new = true,
        .definition = definition,
    };
    REQUIRE(registration.registered_new);
    REQUIRE(registration.definition.name == "NewScheduler");

    statespec::backend::FeatureFlagEvaluationContext context{
        .tenant_id = "tenant-a",
    };
    statespec::backend::FeatureFlagEvaluationRequest request{
        .name = "NewScheduler",
        .context = context,
    };

    REQUIRE(context.tenant_id == "tenant-a");
    REQUIRE(request.name == "NewScheduler");
    REQUIRE(request.context.tenant_id == "tenant-a");
}

TEST_CASE("C++ external system metadata bindings expose lookup contracts")
{
    statespec::backend::ExternalSystemMetadataLookup lookup{
        .external_system = "Billing.Stripe",
        .metadata_entity = "ExternalSystemEndpoint",
        .tenant_field = "tenant_id",
        .profile_field = "profile",
        .key_fields = {"tenant_id", "external_system_id", "profile"},
        .key_values =
            {
                {"tenant_id", "tenant-a"},
                {"external_system_id", "stripe"},
                {"profile", "default"},
            },
        .required_fields = {"base_url", "auth_ref", "timeout_ms"},
    };

    auto document = statespec::backend::Json::object(
        {{"tenant_id", "tenant-a"}, {"base_url", "https://api.stripe.test"}}
    );
    const auto missing =
        statespec::backend::missing_required_metadata_fields(document, lookup.required_fields);

    REQUIRE(lookup.tenant_field == "tenant_id");
    REQUIRE(lookup.key_fields.size() == 3);
    REQUIRE(lookup.key_complete());
    REQUIRE(missing.size() == 2);
    REQUIRE(missing[0] == "auth_ref");

    auto incomplete_lookup = lookup;
    incomplete_lookup.key_values.pop_back();
    const auto missing_key_fields =
        statespec::backend::missing_metadata_key_fields(incomplete_lookup);
    REQUIRE(!incomplete_lookup.key_complete());
    REQUIRE(missing_key_fields.size() == 1);
    REQUIRE(missing_key_fields[0] == "profile");

    statespec::backend::ExternalSystemMetadataResolution resolution{
        .record =
            statespec::backend::VersionedRecord{
                .collection = lookup.metadata_entity,
                .key = "tenant-a/stripe/default",
                .version = 1,
                .document = document,
            },
        .missing_required_fields = missing,
    };
    REQUIRE(!resolution.complete());
}

TEST_CASE("C++ log bindings expose typed events")
{
    statespec::backend::LogEvent event{
        .name = "WorkflowLaunchDecision",
        .level = statespec::backend::LogLevel::Info,
        .event_name = "workflow.launch.decision",
        .fields =
            statespec::backend::Json::object({{"tenant_id", "tenant-a"}, {"decision", "accepted"}}),
    };

    REQUIRE(event.level == statespec::backend::LogLevel::Info);
    REQUIRE(event.event_name == "workflow.launch.decision");
    REQUIRE(event.fields["tenant_id"].as_string() == "tenant-a");
}

TEST_CASE("C++ metric bindings expose typed samples")
{
    statespec::backend::MetricSample sample{
        .name = "WorkflowLaunchAttempts",
        .kind = statespec::backend::MetricKind::Counter,
        .backend_name = "workflow_launch_attempts_total",
        .value = 1.0,
        .unit = "count",
        .labels = statespec::backend::Json::object({{"workflow_name", "OrderProcessing"}}),
    };

    REQUIRE(sample.kind == statespec::backend::MetricKind::Counter);
    REQUIRE(sample.backend_name == "workflow_launch_attempts_total");
    REQUIRE(sample.value == 1.0);
    REQUIRE(sample.labels["workflow_name"].as_string() == "OrderProcessing");
}

TEST_CASE("C++ in-memory backend composes runtime stores")
{
    using namespace statespec::backend;
    using namespace statespec::backend::memory;
    using namespace statespec::backend::runtime;

    InMemoryBackend backend;
    RuntimeFeatureFlagStore feature_flags;
    RuntimeQueueStore queues;
    RuntimeLeaseStore leases;
    RuntimeWorkflowStore workflows;
    RuntimeLogSink logs;
    RuntimeMetricSink metrics;

    backend.ensure_collection(CollectionDescriptor{.name = "orders", .key_fields = {"order_id"}});
    auto tx = backend.begin();
    backend.put(
        *tx, "orders", "order-1", Json::object({{"order_id", "order-1"}, {"status", "Pending"}})
    );

    FeatureFlagDefinition flag{
        .name = "NewScheduler",
        .type = FeatureFlagType::Bool,
        .default_value = FeatureFlagValue::bool_value(true),
        .scope = FeatureFlagScopeKind::Tenant,
    };
    feature_flags.register_definitionTx(*tx, flag);

    queues.register_definitionTx(
        *tx, RegisterQueueDefinitionRequest{
                 .definition = QueueDefinition{
                     .queue = "OrderQueue",
                     .channel = "orders",
                     .visibility_timeout = std::chrono::seconds{30},
                     .max_attempts = 3,
                 },
             }
    );
    queues.enqueueTx(
        *tx, EnqueueMessageRequest{
                 .message_id = "msg-1",
                 .queue = "OrderQueue",
                 .channel = "orders",
                 .idempotency_key = "order-1",
                 .payload = Json::object({{"order_id", "order-1"}}),
             }
    );

    LeaseDefinitionId lease_id{.name = "OrderLease", .version = 1};
    leases.register_definitionTx(
        *tx, LeaseDefinition{
                 .id = lease_id,
                 .resource_pattern = "order:{order_id}",
                 .ttl = std::chrono::seconds{60},
                 .renew_every = std::chrono::seconds{30},
             }
    );

    workflows.register_definitionTx(
        *tx, RegisterWorkflowDefinitionRequest{
                 .definition = WorkflowDefinition{
                     .workflow_name = "OrderProcessing",
                     .workflow_version = 1,
                     .start_step = "validate_order",
                     .expected_execution_time = std::chrono::seconds{60},
                     .steps = {
                         WorkflowStepDefinition{
                             .name = "validate_order",
                             .expected_execution_time = std::chrono::seconds{10},
                             .max_retries = 2,
                         },
                     },
                 },
             }
    );
    workflows.startTx(
        *tx, StartWorkflowRequest{
                 .workflow_execution_id = "wf-1",
                 .workflow_name = "OrderProcessing",
                 .workflow_version = 1,
                 .start_step = "validate_order",
                 .state = Json::object({{"order_id", "order-1"}}),
             }
    );

    logs.register_definitionTx(
        *tx, LogDefinition{
                 .name = "OrderLog",
                 .level = LogLevel::Info,
                 .event_name = "order.event",
             }
    );
    logs.emit_logTx(*tx, LogEvent{.name = "OrderLog", .event_name = "order.event"});

    metrics.register_definitionTx(
        *tx, MetricDefinition{
                 .name = "OrderAttempts",
                 .kind = MetricKind::Counter,
                 .backend_name = "order_attempts_total",
                 .unit = "count",
             }
    );
    metrics.record_metricTx(
        *tx, MetricSample{
                 .name = "OrderAttempts",
                 .kind = MetricKind::Counter,
                 .backend_name = "order_attempts_total",
                 .value = 1.0,
                 .unit = "count",
             }
    );
    backend.commit(*tx);

    auto read_tx = backend.begin();
    REQUIRE(backend.get(*read_tx, "orders", "order-1").has_value());
    REQUIRE(
        feature_flags.evaluateTx(*read_tx, FeatureFlagEvaluationRequest{.name = "NewScheduler"})
            .as_bool() == true
    );
    REQUIRE(queues.inspect_definitionTx(*read_tx, "OrderQueue", "orders").has_value());
    REQUIRE(queues.inspectTx(*read_tx, "msg-1").has_value());
    REQUIRE(leases.inspect_definitionTx(*read_tx, lease_id).has_value());
    REQUIRE(workflows.inspect_definitionTx(*read_tx, "OrderProcessing", 1).has_value());
    REQUIRE(workflows.inspectTx(*read_tx, "wf-1").has_value());
    REQUIRE(logs.inspect_definitionTx(*read_tx, "OrderLog").has_value());
    REQUIRE(metrics.inspect_definitionTx(*read_tx, "OrderAttempts").has_value());
    backend.commit(*read_tx);

    REQUIRE(logs.inspect_events(backend).size() == 1);
    REQUIRE(metrics.inspect_samples(backend).size() == 1);
}
