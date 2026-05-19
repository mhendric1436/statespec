#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR" generated/cpp generated/go generated/java generated/rust
    rmdir generated 2>/dev/null || true
}
trap cleanup EXIT

SPEC="$TMPDIR/minimal.sspec"
cat > "$SPEC" <<'SSPEC'
system Demo {
  tenant scoped_by tenant_id
  system_tenant configured

  feature_flag NewScheduler {
    type bool
    default false
    scope tenant
    owner "platform"
    description "Route order processing through the new scheduler"
    expires "2026-12-31"
  }

  feature_flag MaxPendingOrders {
    type int
    default 100
    scope tenant
  }

  log WorkflowLaunchDecision {
    level info
    event_name "workflow.launch.decision"
    fields {
      tenant_id string
      order_id string
      decision string
    }
  }

  metric WorkflowLaunchAttempts {
    kind counter
    name "workflow_launch_attempts_total"
    unit count
    labels {
      tenant_id string
      workflow_name string
      decision string
    }
  }

  value OrderAmount: decimal where OrderAmount >= 0;

  enum OrderStatus {
    Pending = "pending"
    Active = "active"
    Failed = "failed"
  }

  event OrderAccepted {
    fields {
      order_id string
      amount OrderAmount
      status OrderStatus
    }
  }

  namespace Billing {
    external_system Stripe {
      owner: "payments"
      endpoint: "https://api.stripe.test"
      metadata {
        entity ExternalSystemEndpoint
        profile_field profile
        required_fields [base_url, auth_ref, timeout_ms]
        mappings {
          metadata.base_url -> client.base_url
          metadata.auth_ref -> client.auth_ref
          metadata.timeout_ms -> client.timeout_ms
          input.order_id -> request.order_id
        }
      }
    }
  }

  entity ExternalSystemEndpoint {
    key tenant_id, external_system_id, profile
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      external_system_id string
      profile string
      base_url string
      auth_ref string
      timeout_ms int
      retry_policy string
    }
    state_machine {
      state Active
      state Deleted {
        terminal: true
        garbage_collection {
          after: P30D
          mode: tombstone
        }
      }
      initial Active
      terminal [Deleted]
      Active -> Deleted
    }
  }

  shape StartOrderProcessingRequest {
    tenant_id string
    order_id string
  }

  shape StartOrderProcessingResponse {
    accepted bool
  }

  entity Account {
    key tenant_id, account_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    children {
      orders: Order by account_id
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      account_id string
    }
    state_machine {
      state Active
      initial Active
    }
  }

  entity Order {
    key tenant_id, order_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    relations {
      parent account_id: ref<Account> {
        kind: composition
        on_parent_delete: block
        parent_must_be_in: [Active]
        unique_within_parent: [order_id]
      }
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      order_id string
      account_id string
      retry_count int?
    }
    invariants {
      valid_status: status != ""
    }
    indexes {
      index by_tenant_status on tenant_id, status
      unique by_tenant_order on tenant_id, order_id
    }
    state_machine {
      state Pending
      state Active
      state Failed {
        terminal: true
        garbage_collection {
          after: P30D
          mode: tombstone
        }
      }
      initial Pending
      terminal Failed
      Pending -> Active
      Pending -> Failed
    }
  }

  queue EmailDispatch {
    namespace workflow_ns
    channel email
    visibility_timeout PT30S
    max_attempts 3
    message SendConfirmation {
      idempotency_key message_id
      payload {
        message_id string
        tenant_id string
        order_id string
      }
    }
  }

  lease OrderReconciler {
    resource "orders:reconciler"
    ttl PT30S
    renew_every PT10S
    holder worker_id
    fencing_token true
    max_ttl PT5M
  }

  workflow OrderProcessing {
    version 2
    on OrderAccepted
    singleton false
    expected_execution_time PT60S
    start validate_order

    step validate_order {
      expected_execution_time PT10S
      max_retries 2
      emit OrderAccepted;
    }
  }

  worker OrderWorker {
    singleton true
    lease OrderReconciler
    polls EmailDispatch.SendConfirmation
    executes OrderProcessing
    concurrency 4
  }

  api StartOrderProcessing {
    method POST
    path "/v1/tenants/{tenantId}/orders/{order_id}/start"
    input StartOrderProcessingRequest
    output StartOrderProcessingResponse
    starts workflow OrderProcessing
  }

  api_server OrderApi {
    serves StartOrderProcessing
    concurrency 16
  }

  policy OrderAccess {
    tenant scoped_by tenant_id
    allow StartOrderProcessing when caller.role == operator;
    quota starts_per_minute: 60;
    audit StartOrderProcessing;
  }
}
SSPEC

NO_SYSTEM_SPEC="$TMPDIR/no-system.sspec"
cat > "$NO_SYSTEM_SPEC" <<'SSPEC'
version "0.1"
SSPEC

INCLUDED_SPEC="$TMPDIR/included-bindings.sspec"
cat > "$INCLUDED_SPEC" <<'SSPEC'
statespec 0.1;

system IncludedBindings {
  tenant scoped_by tenant_id
  system_tenant configured

  entity IncludedEntity {
    key tenant_id, included_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      included_id string
    }
    state_machine {
      state Active
      initial Active
    }
  }

  queue IncludedQueue {
    namespace included_ns
    channel included
    visibility_timeout PT30S
    max_attempts 3
    message IncludedMessage {
      idempotency_key message_id
      payload {
        message_id string
        tenant_id string
        included_id string
      }
    }
  }

  workflow IncludedWorkflow {
    version 1
    singleton false
    expected_execution_time PT30S
    start included_step
    step included_step {
      expected_execution_time PT10S
      max_retries 1
    }
  }
}
SSPEC

INCLUDE_ROOT_SPEC="$TMPDIR/include-bindings-root.sspec"
cat > "$INCLUDE_ROOT_SPEC" <<'SSPEC'
statespec 0.1;
include "./included-bindings.sspec";

system IncludeBindingsRoot {
  feature_flag IncludedEntityLaunch {
    type bool
    default true
    scope entity IncludedEntity
  }
}
SSPEC

run_expect_status() {
    expected="$1"
    shift
    output="$TMPDIR/output.txt"
    set +e
    "$@" > "$output" 2>&1
    status="$?"
    set -e
    if [ "$status" -ne "$expected" ]; then
        echo "expected status $expected, got $status for: $*" >&2
        cat "$output" >&2
        exit 1
    fi
}

assert_output_contains() {
    needle="$1"
    if ! grep -F -- "$needle" "$TMPDIR/output.txt" >/dev/null 2>&1; then
        echo "expected output to contain: $needle" >&2
        cat "$TMPDIR/output.txt" >&2
        exit 1
    fi
}

assert_file_exists() {
    path="$1"
    if [ ! -f "$path" ]; then
        echo "expected generated file: $path" >&2
        cat "$TMPDIR/output.txt" >&2 || true
        exit 1
    fi
}

assert_file_contains() {
    path="$1"
    needle="$2"
    if ! grep -F -- "$needle" "$path" >/dev/null 2>&1; then
        echo "expected $path to contain: $needle" >&2
        cat "$path" >&2 || true
        exit 1
    fi
}

# Help.
run_expect_status 0 "$CLI" help
assert_output_contains "usage:"
assert_output_contains "statespec help"
assert_output_contains "statespec fmt [--check] <file.sspec>"
assert_output_contains "statespec generate bindings --lang <cpp|go|java|rust>"

run_expect_status 0 "$CLI" --help
assert_output_contains "statespec validate <file.sspec>"

run_expect_status 0 "$CLI" -h
assert_output_contains "statespec ast <file.sspec>"

# Positive generation: C++.
run_expect_status 0 "$CLI" generate bindings --lang cpp "$SPEC" --out "$TMPDIR/out-cpp"
assert_output_contains "generated $TMPDIR/out-cpp/backend.hpp"
assert_file_exists "$TMPDIR/out-cpp/json.hpp"
assert_file_exists "$TMPDIR/out-cpp/backend.hpp"
assert_file_exists "$TMPDIR/out-cpp/external_system.hpp"
assert_file_exists "$TMPDIR/out-cpp/feature_flag.hpp"
assert_file_exists "$TMPDIR/out-cpp/lease.hpp"
assert_file_exists "$TMPDIR/out-cpp/log.hpp"
assert_file_exists "$TMPDIR/out-cpp/metric.hpp"
assert_file_exists "$TMPDIR/out-cpp/queue.hpp"
assert_file_exists "$TMPDIR/out-cpp/workflow.hpp"
assert_file_exists "$TMPDIR/out-cpp/system_descriptors.hpp"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "FeatureFlagDefinition"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "feature_flag_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ShapeDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "shape_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "struct StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "std::string tenant_id"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "NamespaceDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "namespace_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ValueDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "value_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EnumDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "enum_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EventDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "event_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "struct EventEnvelope"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "make_order_accepted_event"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ExternalSystemDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ExternalSystemMetadataDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ExternalSystemMetadataMappingDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ExternalSystemMetadataMappingPlan"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ExternalSystemMetadataMappingInputs"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "IExternalSystemMetadataMappingApplicator"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "external_system_metadata_mapping_plan"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "std::optional<std::string> tenant_field"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "std::vector<std::string> key_fields"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"metadata.base_url\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"client.base_url\""
assert_file_contains "$TMPDIR/out-cpp/external_system.hpp" "IExternalSystemMetadataResolver"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "external_system_metadata_lookup"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "resolve_external_system_metadataTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "key_complete()"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "external_system_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ApiDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "api_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "api_server_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ApiRequestContext"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ApiResponse"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "class IApiHandler"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "api_route_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"OrderApi\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"OrderApi.StartOrderProcessing\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "WorkerDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "WorkerContext"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "class IWorker"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "worker_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "worker_contexts"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"OrderWorker\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"EmailDispatch.SendConfirmation\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "PolicyDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "policy_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "GarbageCollectionPolicy"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EntityStateDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EntityOwnershipDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EntityRelationDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EntityChildDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EntityInvariantDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "entity_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "CollectionDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "IndexDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "queue_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "lease_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "workflow_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EmailDispatch"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderReconciler"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderProcessing"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "validate_order"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "NewScheduler"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "MaxPendingOrders"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderAmount"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderStatus"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderAccepted"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "Billing.Stripe"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "StartOrderProcessing"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "/v1/tenants/{tenantId}/orders/{order_id}/start"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderAccess"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "struct LogDefinition"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "log_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "WorkflowLaunchDecision"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "workflow.launch.decision"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "struct MetricDefinition"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "metric_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ensure_system_collections"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "register_feature_flag_definitionsTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "create_queue_definitionsTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "register_lease_definitionsTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "register_runtime_catalogTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "register_observability_catalogTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "register_workflow_definitionsTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "workflow_launch_attempts_total"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"composition\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"account_id\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"valid_status\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"by_tenant_status\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"by_tenant_order\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"P30D\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"tombstone\""

cat > "$TMPDIR/out-cpp/metadata_resolver_fixture.cpp" <<'CPP'
#include "system_descriptors.hpp"

#include <string>
#include <vector>

class FakeTx final : public statespec::backend::ITransaction
{
  public:
    bool is_open() const override { return true; }
    void abort() override {}
};

class FakeResolver final : public statespec::backend::IExternalSystemMetadataResolver
{
  public:
    int calls = 0;

    std::optional<statespec::backend::ExternalSystemMetadataResolution> resolve_metadata(
        statespec::backend::IBackend&,
        const statespec::backend::ExternalSystemMetadataLookup&
    ) override
    {
        return std::nullopt;
    }

    std::optional<statespec::backend::ExternalSystemMetadataResolution> resolve_metadataTx(
        statespec::backend::ITransaction&,
        const statespec::backend::ExternalSystemMetadataLookup& lookup
    ) override
    {
        ++calls;
        const auto document = statespec::backend::Json::object(
            {{"tenant_id", "tenant-a"}, {"base_url", "https://api.stripe.test"}}
        );
        return statespec::backend::ExternalSystemMetadataResolution{
            statespec::backend::VersionedRecord{
                lookup.metadata_entity,
                "tenant-a/stripe/default",
                1,
                document,
            },
            statespec::backend::missing_required_metadata_fields(document, lookup.required_fields),
        };
    }
};

class FakeMappingApplicator final
    : public statespec_generated::IExternalSystemMetadataMappingApplicator
{
  public:
    statespec_generated::ExternalSystemMetadataMappingOutput apply(
        const statespec_generated::ExternalSystemMetadataMappingPlan& plan,
        const statespec_generated::ExternalSystemMetadataMappingInputs& inputs
    ) override
    {
        statespec_generated::ExternalSystemMetadataMappingOutput output;
        for (const auto& assignment : plan.all_mappings)
        {
            const auto& source = assignment.source_root == "metadata"
                ? inputs.metadata.at(assignment.source_field)
                : inputs.input.at(assignment.source_field);
            if (assignment.target_root == "client")
            {
                output.client_config.emplace(assignment.field, source);
            }
            else if (assignment.target_root == "request")
            {
                output.request_payload.emplace(assignment.field, source);
            }
        }
        return output;
    }
};

int main()
{
    FakeTx tx;
    FakeResolver resolver;
    const auto descriptors = statespec_generated::external_system_descriptors();
    const auto plan = statespec_generated::external_system_metadata_mapping_plan(descriptors[0]);
    if (plan.all_mappings.size() != 4 ||
        plan.client_mappings.size() != 3 || plan.request_mappings.size() != 1 ||
        plan.client_mappings[0].source_root != "metadata" ||
        plan.client_mappings[0].source_field != "base_url" ||
        plan.client_mappings[0].target_root != "client" ||
        plan.client_mappings[0].field != "base_url" ||
        plan.request_mappings[0].source_root != "input" ||
        plan.request_mappings[0].source_field != "order_id" ||
        plan.request_mappings[0].target_root != "request" ||
        plan.request_mappings[0].field != "order_id")
    {
        return 1;
    }
    FakeMappingApplicator applicator;
    statespec_generated::IExternalSystemMetadataMappingApplicator& mapping_applicator =
        applicator;
    const auto mapped = mapping_applicator.apply(
        plan,
        statespec_generated::ExternalSystemMetadataMappingInputs{
            {{"order_id", "order-1"}},
            {},
            {},
            {{"base_url", "https://api.stripe.test"},
             {"auth_ref", "secret:stripe"},
             {"timeout_ms", std::int64_t{5000}}},
        }
    );
    if (mapped.client_config.size() != 3 || mapped.request_payload.size() != 1 ||
        mapped.client_config.find("base_url") == mapped.client_config.end() ||
        mapped.request_payload.find("order_id") == mapped.request_payload.end())
    {
        return 1;
    }
    std::vector<statespec::backend::ExternalSystemMetadataKeyValue> keys{
        {"tenant_id", "tenant-a"},
        {"external_system_id", "stripe"},
        {"profile", "default"},
    };
    auto resolved = statespec_generated::resolve_external_system_metadataTx(
        resolver, tx, "Billing.Stripe", keys
    );
    if (!resolved.has_value() || resolved->complete() || resolver.calls != 1)
    {
        return 1;
    }

    keys.pop_back();
    auto skipped = statespec_generated::resolve_external_system_metadataTx(
        resolver, tx, "Billing.Stripe", keys
    );
    return skipped.has_value() || resolver.calls != 1 ? 1 : 0;
}
CPP
${CXX:-c++} -std=c++20 -I"$TMPDIR/out-cpp" "$TMPDIR/out-cpp/metadata_resolver_fixture.cpp" -o "$TMPDIR/out-cpp/metadata_resolver_fixture"
"$TMPDIR/out-cpp/metadata_resolver_fixture"

# Positive generation: Go.
run_expect_status 0 "$CLI" generate bindings --lang go "$SPEC" --out "$TMPDIR/out-go"
assert_output_contains "generated $TMPDIR/out-go/backend/backend.go"
assert_file_exists "$TMPDIR/out-go/backend/json.go"
assert_file_exists "$TMPDIR/out-go/backend/backend.go"
assert_file_exists "$TMPDIR/out-go/backend/external_system.go"
assert_file_exists "$TMPDIR/out-go/backend/feature_flag.go"
assert_file_exists "$TMPDIR/out-go/backend/lease.go"
assert_file_exists "$TMPDIR/out-go/backend/log.go"
assert_file_exists "$TMPDIR/out-go/backend/metric.go"
assert_file_exists "$TMPDIR/out-go/backend/queue.go"
assert_file_exists "$TMPDIR/out-go/backend/workflow.go"
assert_file_exists "$TMPDIR/out-go/backend/descriptors.go"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type FeatureFlagDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func FeatureFlagDefinitions() []FeatureFlagDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ShapeDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ShapeDescriptors() []ShapeDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type StartOrderProcessingRequest struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "TenantId string"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type NamespaceDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func NamespaceDescriptors() []NamespaceDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ValueDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ValueDescriptors() []ValueDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EnumDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func EnumDescriptors() []EnumDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EventDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func EventDescriptors() []EventDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EventEnvelope struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func NewOrderAcceptedEvent"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ExternalSystemDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ExternalSystemMetadataDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ExternalSystemMetadataMappingDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ExternalSystemMetadataMappingPlan struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ExternalSystemMetadataMappingInputs struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ExternalSystemMetadataMappingApplicator interface"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func BuildExternalSystemMetadataMappingPlan"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "TenantField *string"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "KeyFields []string"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "TenantField: stringPtr(\"tenant_id\")"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Source: \"metadata.base_url\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Target: \"client.base_url\""
assert_file_contains "$TMPDIR/out-go/backend/external_system.go" "type ExternalSystemMetadataResolver interface"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func BuildExternalSystemMetadataLookup"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ExternalSystemMetadataLookupByName"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ResolveExternalSystemMetadataTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ResolveExternalSystemMetadataByNameTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "KeyComplete()"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ExternalSystemDescriptors() []ExternalSystemDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ApiDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ApiDescriptors() []ApiDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ApiServerDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ApiServerDescriptors() []ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ApiRouteDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type APIRequestContext struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type APIResponse struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type APIHandler interface"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ApiRouteDescriptors() []ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Name: \"OrderApi\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Serves: []string{\"StartOrderProcessing\"}"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Name: \"OrderApi.StartOrderProcessing\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "ServerName: \"OrderApi\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type WorkerDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type WorkerContext struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type Worker interface"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func WorkerDescriptors() []WorkerDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func WorkerContexts() []WorkerContext"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Name: \"OrderWorker\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Polls: stringPtr(\"EmailDispatch.SendConfirmation\")"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type PolicyDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func PolicyDescriptors() []PolicyDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type GarbageCollectionPolicy struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EntityStateDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EntityOwnershipDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EntityRelationDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EntityChildDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EntityInvariantDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func EntityDescriptors() []EntityDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func CollectionDescriptors() []CollectionDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Indexes: []IndexDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func QueueDefinitions() []QueueDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func LeaseDefinitions() []LeaseDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func WorkflowDefinitions() []WorkflowDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "EmailDispatch"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "OrderReconciler"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "WorkflowVersion: 2"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "validate_order"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "NewScheduler"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "DefaultValue: \"false\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "OrderAmount"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "OrderStatus"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "OrderAccepted"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Billing.Stripe"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "StartOrderProcessing"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Path: stringPtr(\"/v1/tenants/{tenantId}/orders/{order_id}/start\")"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "OrderAccess"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type LogDefinition struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func LogDefinitions() []LogDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "EventName: \"workflow.launch.decision\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type MetricDefinition struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func MetricDefinitions() []MetricDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func EnsureSystemCollections(ctx context.Context, backend Backend) error"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func RegisterFeatureFlagDefinitionsTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func CreateQueueDefinitionsTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func RegisterLeaseDefinitionsTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func RegisterRuntimeCatalogTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func RegisterObservabilityCatalogTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func RegisterWorkflowDefinitionsTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "BackendName: \"workflow_launch_attempts_total\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Metadata: JSONObject(map[string]JSON{})"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "RelationKind: stringPtr(\"composition\")"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Name: \"valid_status\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Name: \"by_tenant_status\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Unique: true"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "GarbageCollection: &GarbageCollectionPolicy{After: \"P30D\", Mode: \"tombstone\"}"

cat > "$TMPDIR/out-go/backend/metadata_resolver_fixture_test.go" <<'GO'
package backend

import (
	"context"
	"testing"
)

type fixtureTx struct{}

func (fixtureTx) IsOpen() bool { return true }
func (fixtureTx) Abort(context.Context) error { return nil }

type fixtureResolver struct {
	calls int
}

func (r *fixtureResolver) ResolveMetadata(context.Context, Backend, ExternalSystemMetadataLookup) (*ExternalSystemMetadataResolution, error) {
	return nil, nil
}

func (r *fixtureResolver) ResolveMetadataTx(ctx context.Context, tx Transaction, lookup ExternalSystemMetadataLookup) (*ExternalSystemMetadataResolution, error) {
	r.calls++
	document := JSONObject(map[string]JSON{
		"tenant_id": JSONString("tenant-a"),
		"base_url": JSONString("https://api.stripe.test"),
	})
	return &ExternalSystemMetadataResolution{
		Record: VersionedRecord{
			Collection: CollectionName(lookup.MetadataEntity),
			Key:        Key("tenant-a/stripe/default"),
			Version:    Version(1),
			Document:   document,
		},
		MissingRequiredFields: MissingRequiredMetadataFields(document, lookup.RequiredFields),
	}, nil
}

type fixtureMappingApplicator struct{}

func (fixtureMappingApplicator) ApplyExternalSystemMetadataMappings(ctx context.Context, plan ExternalSystemMetadataMappingPlan, inputs ExternalSystemMetadataMappingInputs) (ExternalSystemMetadataMappingOutput, error) {
	output := ExternalSystemMetadataMappingOutput{
		ClientConfig:   map[string]JSON{},
		RequestPayload: map[string]JSON{},
	}
	for _, assignment := range plan.AllMappings {
		var value JSON
		if assignment.SourceRoot == "metadata" {
			value = inputs.Metadata[assignment.SourceField]
		} else {
			value = inputs.Input[assignment.SourceField]
		}
		if assignment.TargetRoot == "client" {
			output.ClientConfig[assignment.Field] = value
		} else if assignment.TargetRoot == "request" {
			output.RequestPayload[assignment.Field] = value
		}
	}
	return output, nil
}

func TestGeneratedMetadataResolverFixture(t *testing.T) {
	ctx := context.Background()
	resolver := &fixtureResolver{}
	tx := fixtureTx{}
	plan := BuildExternalSystemMetadataMappingPlan(ExternalSystemDescriptors()[0])
	if len(plan.AllMappings) != 4 || len(plan.ClientMappings) != 3 || len(plan.RequestMappings) != 1 || plan.ClientMappings[0].SourceRoot != "metadata" || plan.ClientMappings[0].SourceField != "base_url" || plan.ClientMappings[0].TargetRoot != "client" || plan.ClientMappings[0].Field != "base_url" || plan.RequestMappings[0].SourceRoot != "input" || plan.RequestMappings[0].SourceField != "order_id" || plan.RequestMappings[0].TargetRoot != "request" || plan.RequestMappings[0].Field != "order_id" {
		t.Fatalf("unexpected metadata mapping plan: %#v", plan)
	}
	var applicator ExternalSystemMetadataMappingApplicator = fixtureMappingApplicator{}
	mapped, err := applicator.ApplyExternalSystemMetadataMappings(ctx, plan, ExternalSystemMetadataMappingInputs{
		Input: map[string]JSON{
			"order_id": JSONString("order-1"),
		},
		Metadata: map[string]JSON{
			"base_url":   JSONString("https://api.stripe.test"),
			"auth_ref":   JSONString("secret:stripe"),
			"timeout_ms": JSONInt(5000),
		},
	})
	if err != nil || len(mapped.ClientConfig) != 3 || len(mapped.RequestPayload) != 1 {
		t.Fatalf("unexpected mapped metadata output: %#v err=%v", mapped, err)
	}
	keys := []ExternalSystemMetadataKeyValue{
		{Field: "tenant_id", Value: JSONString("tenant-a")},
		{Field: "external_system_id", Value: JSONString("stripe")},
		{Field: "profile", Value: JSONString("default")},
	}
	resolved, ok, err := ResolveExternalSystemMetadataByNameTx(ctx, resolver, tx, "Billing.Stripe", keys)
	if err != nil || !ok || resolved == nil || resolved.Complete() || resolver.calls != 1 {
		t.Fatalf("expected incomplete metadata resolution through resolver, ok=%v resolved=%#v err=%v calls=%d", ok, resolved, err, resolver.calls)
	}

	keys = keys[:len(keys)-1]
	skipped, ok, err := ResolveExternalSystemMetadataByNameTx(ctx, resolver, tx, "Billing.Stripe", keys)
	if err != nil || !ok || skipped != nil || resolver.calls != 1 {
		t.Fatalf("expected incomplete key to skip resolver, ok=%v skipped=%#v err=%v calls=%d", ok, skipped, err, resolver.calls)
	}
}
GO
cat > "$TMPDIR/out-go/go.mod" <<'GOMOD'
module statespec-generated-fixture

go 1.22
GOMOD
(cd "$TMPDIR/out-go" && go test ./...)

# Positive generation: Java.
run_expect_status 0 "$CLI" generate bindings --lang java "$SPEC" --out "$TMPDIR/out-java"
assert_output_contains "generated $TMPDIR/out-java/com/statespec/backend/Backend.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Json.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Backend.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/ExternalSystem.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/FeatureFlag.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Lease.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Log.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Metric.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Queue.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Workflow.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/generated/Descriptors.java"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "class Descriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record FeatureFlagDefinition"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ShapeDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "String tenant_id"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record NamespaceDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ValueDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EnumDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EventDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EventEnvelope"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "buildOrderAcceptedEvent"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ExternalSystemDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataMappingDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataMappingPlan"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataMappingInputs"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "interface ExternalSystemMetadataMappingApplicator"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "externalSystemMetadataMappingPlan"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Optional<String> tenantField"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "List<String> keyFields"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Optional.of(\"tenant_id\")"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"metadata.base_url\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"client.base_url\""
assert_file_contains "$TMPDIR/out-java/com/statespec/backend/ExternalSystem.java" "interface ExternalSystem"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "externalSystemMetadataLookup"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "resolveExternalSystemMetadataTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "keyComplete()"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiRequestContext"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiResponse"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "interface ApiHandler"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "apiRouteDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"OrderApi\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "List.of(\"StartOrderProcessing\")"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"OrderApi.StartOrderProcessing\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record WorkerDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record WorkerContext"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "interface Worker"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"OrderWorker\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Optional.of(\"EmailDispatch.SendConfirmation\")"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record PolicyDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record GarbageCollectionPolicy"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EntityStateDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EntityOwnershipDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EntityRelationDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EntityChildDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EntityInvariantDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "entityDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "featureFlagDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "shapeDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "namespaceDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "valueDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "enumDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "eventDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "externalSystemDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "apiDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "apiServerDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workerDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workerContexts"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "policyDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "queueDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "leaseDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workflowDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "EmailDispatch"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "OrderReconciler"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "2L"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "validate_order"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "NewScheduler"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "OrderAmount"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "OrderStatus"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "OrderAccepted"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Billing.Stripe"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "StartOrderProcessing"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Optional.of(\"/v1/tenants/{tenantId}/orders/{order_id}/start\")"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "OrderAccess"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record LogDefinition"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "logDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workflow.launch.decision"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record MetricDefinition"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "metricDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "ensureSystemCollections"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "registerFeatureFlagDefinitionsTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "createQueueDefinitionsTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "registerLeaseDefinitionsTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "registerRuntimeCatalogTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "new IndexDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "registerObservabilityCatalogTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "registerWorkflowDefinitionsTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workflow_launch_attempts_total"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Optional.of(\"composition\")"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "new EntityInvariantDescriptor(\"valid_status\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"by_tenant_status\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"by_tenant_order\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "new GarbageCollectionPolicy(\"P30D\", \"tombstone\")"

cat > "$TMPDIR/out-java/com/statespec/generated/MetadataResolverFixture.java" <<'JAVA'
package com.statespec.generated;

import com.statespec.backend.Backend;
import com.statespec.backend.ExternalSystem;
import com.statespec.backend.Json;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public final class MetadataResolverFixture
{
    private static final class FixtureTx implements Backend.Transaction
    {
        @Override public boolean isOpen() { return true; }
        @Override public void abort() throws Backend.BackendException {}
    }

    private static final class FixtureResolver implements ExternalSystem
    {
        int calls = 0;

        @Override public Optional<MetadataResolution> resolveMetadata(
            Backend backend,
            MetadataLookup lookup
        ) throws Backend.BackendException
        {
            return Optional.empty();
        }

        @Override public Optional<MetadataResolution> resolveMetadataTx(
            Backend.Transaction tx,
            MetadataLookup lookup
        ) throws Backend.BackendException
        {
            calls++;
            Json document = Json.object(Map.of(
                "tenant_id", Json.string("tenant-a"),
                "base_url", Json.string("https://api.stripe.test")
            ));
            return Optional.of(new MetadataResolution(
                new Backend.VersionedRecord(
                    lookup.metadataEntity(),
                    "tenant-a/stripe/default",
                    1L,
                    document
                ),
                ExternalSystem.missingRequiredMetadataFields(document, lookup.requiredFields())
            ));
        }
    }

    private static final class FixtureMappingApplicator
        implements Descriptors.ExternalSystemMetadataMappingApplicator
    {
        @Override public Descriptors.ExternalSystemMetadataMappingOutput
        applyExternalSystemMetadataMappings(
            Descriptors.ExternalSystemMetadataMappingPlan plan,
            Descriptors.ExternalSystemMetadataMappingInputs inputs
        )
        {
            java.util.HashMap<String, Json> clientConfig = new java.util.HashMap<>();
            java.util.HashMap<String, Json> requestPayload = new java.util.HashMap<>();
            for (Descriptors.ExternalSystemMetadataMappingAssignment assignment :
                plan.allMappings())
            {
                Json value = assignment.sourceRoot().equals("metadata")
                    ? inputs.metadata().get(assignment.sourceField())
                    : inputs.input().get(assignment.sourceField());
                if (assignment.targetRoot().equals("client"))
                {
                    clientConfig.put(assignment.field(), value);
                }
                else if (assignment.targetRoot().equals("request"))
                {
                    requestPayload.put(assignment.field(), value);
                }
            }
            return new Descriptors.ExternalSystemMetadataMappingOutput(
                clientConfig,
                requestPayload
            );
        }
    }

    public static void main(String[] args) throws Exception
    {
        FixtureResolver resolver = new FixtureResolver();
        FixtureTx tx = new FixtureTx();
        Descriptors.ExternalSystemMetadataMappingPlan plan =
            Descriptors.externalSystemMetadataMappingPlan(
                Descriptors.externalSystemDescriptors().get(0)
            );
        if (plan.allMappings().size() != 4 ||
            plan.clientMappings().size() != 3 || plan.requestMappings().size() != 1 ||
            !plan.clientMappings().get(0).sourceRoot().equals("metadata") ||
            !plan.clientMappings().get(0).sourceField().equals("base_url") ||
            !plan.clientMappings().get(0).targetRoot().equals("client") ||
            !plan.clientMappings().get(0).field().equals("base_url") ||
            !plan.requestMappings().get(0).sourceRoot().equals("input") ||
            !plan.requestMappings().get(0).sourceField().equals("order_id") ||
            !plan.requestMappings().get(0).targetRoot().equals("request") ||
            !plan.requestMappings().get(0).field().equals("order_id"))
        {
            throw new AssertionError("unexpected metadata mapping plan");
        }
        Descriptors.ExternalSystemMetadataMappingApplicator applicator =
            new FixtureMappingApplicator();
        Descriptors.ExternalSystemMetadataMappingOutput mapped =
            applicator.applyExternalSystemMetadataMappings(
                plan,
                new Descriptors.ExternalSystemMetadataMappingInputs(
                    Map.of("order_id", Json.string("order-1")),
                    Map.of(),
                    Map.of(),
                    Map.of(
                        "base_url", Json.string("https://api.stripe.test"),
                        "auth_ref", Json.string("secret:stripe"),
                        "timeout_ms", Json.integer(5000)
                    )
                )
            );
        if (mapped.clientConfig().size() != 3 || mapped.requestPayload().size() != 1)
        {
            throw new AssertionError("unexpected mapped metadata output");
        }
        List<ExternalSystem.MetadataKeyValue> keys = List.of(
            new ExternalSystem.MetadataKeyValue("tenant_id", Json.string("tenant-a")),
            new ExternalSystem.MetadataKeyValue("external_system_id", Json.string("stripe")),
            new ExternalSystem.MetadataKeyValue("profile", Json.string("default"))
        );
        Optional<ExternalSystem.MetadataResolution> resolved =
            Descriptors.resolveExternalSystemMetadataTx(resolver, tx, "Billing.Stripe", keys);
        if (resolved.isEmpty() || resolved.orElseThrow().complete() || resolver.calls != 1)
        {
            throw new AssertionError("expected incomplete metadata resolution through resolver");
        }

        Optional<ExternalSystem.MetadataResolution> skipped =
            Descriptors.resolveExternalSystemMetadataTx(
                resolver, tx, "Billing.Stripe", keys.subList(0, 2)
            );
        if (skipped.isPresent() || resolver.calls != 1)
        {
            throw new AssertionError("expected incomplete key to skip resolver");
        }
    }
}
JAVA
find "$TMPDIR/out-java" -name '*.java' -print > "$TMPDIR/out-java/sources.list"
${JAVAC:-javac} @"$TMPDIR/out-java/sources.list"
${JAVA:-java} -cp "$TMPDIR/out-java" com.statespec.generated.MetadataResolverFixture

# Positive generation: Rust.
run_expect_status 0 "$CLI" generate bindings --lang rust "$SPEC" --out "$TMPDIR/out-rust"
assert_output_contains "generated $TMPDIR/out-rust/backend.rs"
assert_file_exists "$TMPDIR/out-rust/json.rs"
assert_file_exists "$TMPDIR/out-rust/backend.rs"
assert_file_exists "$TMPDIR/out-rust/external_system.rs"
assert_file_exists "$TMPDIR/out-rust/feature_flag.rs"
assert_file_exists "$TMPDIR/out-rust/lease.rs"
assert_file_exists "$TMPDIR/out-rust/log.rs"
assert_file_exists "$TMPDIR/out-rust/metric.rs"
assert_file_exists "$TMPDIR/out-rust/queue.rs"
assert_file_exists "$TMPDIR/out-rust/workflow.rs"
assert_file_exists "$TMPDIR/out-rust/descriptors.rs"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct FeatureFlagDefinition"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn feature_flag_definitions() -> Vec<FeatureFlagDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ShapeDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn shape_descriptors() -> Vec<ShapeDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub tenant_id: String"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct NamespaceDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn namespace_descriptors() -> Vec<NamespaceDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ValueDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn value_descriptors() -> Vec<ValueDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EnumDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn enum_descriptors() -> Vec<EnumDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EventDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn event_descriptors() -> Vec<EventDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EventEnvelope"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn make_order_accepted_event"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ExternalSystemDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ExternalSystemMetadataDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ExternalSystemMetadataMappingDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ExternalSystemMetadataMappingPlan"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ExternalSystemMetadataMappingInputs"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub trait ExternalSystemMetadataMappingApplicator"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn external_system_metadata_mapping_plan"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub tenant_field: Option<String>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub key_fields: Vec<String>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "tenant_field: Some(\"tenant_id\".to_string())"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "source: \"metadata.base_url\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "target: \"client.base_url\".to_string()"
assert_file_contains "$TMPDIR/out-rust/external_system.rs" "pub trait ExternalSystemMetadataResolver"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn external_system_metadata_lookup"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn external_system_metadata_lookup_by_name"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn resolve_external_system_metadata_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn resolve_external_system_metadata_by_name_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "key_complete()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn external_system_descriptors() -> Vec<ExternalSystemDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn api_descriptors() -> Vec<ApiDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn api_server_descriptors() -> Vec<ApiServerDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiRequestContext"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiResponse"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub trait ApiHandler"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn api_route_descriptors() -> Vec<ApiRouteDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "name: \"OrderApi\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "serves: vec![\"StartOrderProcessing\".to_string()]"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "name: \"OrderApi.StartOrderProcessing\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "server_name: \"OrderApi\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct WorkerDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct WorkerContext"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub trait Worker"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn worker_descriptors() -> Vec<WorkerDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn worker_contexts() -> Vec<WorkerContext>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "name: \"OrderWorker\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "polls: Some(\"EmailDispatch.SendConfirmation\".to_string())"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct PolicyDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn policy_descriptors() -> Vec<PolicyDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct GarbageCollectionPolicy"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EntityStateDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EntityOwnershipDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EntityRelationDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EntityChildDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EntityInvariantDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn entity_descriptors() -> Vec<EntityDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn collection_descriptors() -> Vec<CollectionDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "IndexDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn queue_definitions() -> Vec<QueueDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn lease_definitions() -> Vec<LeaseDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn workflow_definitions() -> Vec<WorkflowDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "EmailDispatch"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "OrderReconciler"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "workflow_version: 2"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "validate_order"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "NewScheduler"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "OrderAmount"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "OrderStatus"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "OrderAccepted"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "Billing.Stripe"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "StartOrderProcessing"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "path: Some(\"/v1/tenants/{tenantId}/orders/{order_id}/start\".to_string())"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "OrderAccess"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct LogDefinition"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn log_definitions() -> Vec<LogDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "event_name: \"workflow.launch.decision\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct MetricDefinition"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn metric_definitions() -> Vec<MetricDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn ensure_system_collections"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn register_feature_flag_definitions_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn create_queue_definitions_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn register_lease_definitions_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn register_runtime_catalog_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn register_observability_catalog_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn register_workflow_definitions_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "backend_name: \"workflow_launch_attempts_total\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "relation_kind: Some(\"composition\".to_string())"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "name: \"valid_status\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "name: \"by_tenant_status\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "unique: true"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "garbage_collection: Some(GarbageCollectionPolicy { after: \"P30D\".to_string(), mode: \"tombstone\".to_string() })"

cat > "$TMPDIR/out-rust/Cargo.toml" <<'TOML'
[package]
name = "statespec-generated-fixture"
version = "0.1.0"
edition = "2021"

[lib]
path = "lib.rs"
TOML

cat > "$TMPDIR/out-rust/lib.rs" <<'RUST'
pub mod backend;
pub mod descriptors;
pub mod external_system;
pub mod feature_flag;
pub mod json;
pub mod lease;
pub mod log;
pub mod metric;
pub mod queue;
pub mod workflow;

#[cfg(test)]
mod tests {
    use std::cell::Cell;
    use std::collections::BTreeMap;

    use crate::backend::{
        Backend, BackendCapabilities, BackendResult, CollectionDescriptor, Query, Transaction,
        VersionedRecord,
    };
    use crate::external_system::{
        ExternalSystemMetadataKeyValue, ExternalSystemMetadataLookup,
        ExternalSystemMetadataResolution, ExternalSystemMetadataResolver,
    };
    use crate::json::Json;

    #[derive(Default)]
    struct FixtureTx;

    impl Transaction for FixtureTx {
        fn is_open(&self) -> bool {
            true
        }

        fn abort(&mut self) -> BackendResult<()> {
            Ok(())
        }
    }

    struct FixtureBackend;

    impl Backend for FixtureBackend {
        type Tx = FixtureTx;

        fn capabilities(&self) -> BackendCapabilities {
            BackendCapabilities::default()
        }

        fn ensure_collection(&self, _descriptor: &CollectionDescriptor) -> BackendResult<()> {
            Ok(())
        }

        fn ensure_collections(&self, _descriptors: &[CollectionDescriptor]) -> BackendResult<()> {
            Ok(())
        }

        fn begin(&self) -> BackendResult<Self::Tx> {
            Ok(FixtureTx)
        }

        fn get(
            &self,
            _tx: &mut Self::Tx,
            _collection: &str,
            _key: &str,
        ) -> BackendResult<Option<VersionedRecord>> {
            Ok(None)
        }

        fn query(
            &self,
            _tx: &mut Self::Tx,
            _collection: &str,
            _query: &Query,
        ) -> BackendResult<Vec<VersionedRecord>> {
            Ok(vec![])
        }

        fn put(
            &self,
            _tx: &mut Self::Tx,
            _collection: &str,
            _key: &str,
            _document: Json,
        ) -> BackendResult<()> {
            Ok(())
        }

        fn erase(&self, _tx: &mut Self::Tx, _collection: &str, _key: &str) -> BackendResult<()> {
            Ok(())
        }

        fn commit(&self, _tx: Self::Tx) -> BackendResult<()> {
            Ok(())
        }
    }

    struct FixtureResolver {
        calls: Cell<usize>,
    }

    impl ExternalSystemMetadataResolver<FixtureBackend> for FixtureResolver {
        fn resolve_metadata(
            &self,
            _backend: &FixtureBackend,
            _lookup: &ExternalSystemMetadataLookup,
        ) -> BackendResult<Option<ExternalSystemMetadataResolution>> {
            Ok(None)
        }

        fn resolve_metadata_tx(
            &self,
            _tx: &mut FixtureTx,
            lookup: &ExternalSystemMetadataLookup,
        ) -> BackendResult<Option<ExternalSystemMetadataResolution>> {
            self.calls.set(self.calls.get() + 1);
            let document = Json::Object(
                [
                    ("tenant_id".to_string(), Json::String("tenant-a".to_string())),
                    (
                        "base_url".to_string(),
                        Json::String("https://api.stripe.test".to_string()),
                    ),
                ]
                .into_iter()
                .collect(),
            );
            Ok(Some(ExternalSystemMetadataResolution {
                record: VersionedRecord {
                    collection: lookup.metadata_entity.clone(),
                    key: "tenant-a/stripe/default".to_string(),
                    version: 1,
                    document: document.clone(),
                },
                missing_required_fields: crate::external_system::missing_required_metadata_fields(
                    &document,
                    &lookup.required_fields,
                ),
            }))
        }
    }

    struct FixtureMappingApplicator;

    impl crate::descriptors::ExternalSystemMetadataMappingApplicator
        for FixtureMappingApplicator
    {
        fn apply_external_system_metadata_mappings(
            &self,
            plan: &crate::descriptors::ExternalSystemMetadataMappingPlan,
            inputs: &crate::descriptors::ExternalSystemMetadataMappingInputs,
        ) -> BackendResult<crate::descriptors::ExternalSystemMetadataMappingOutput> {
            let mut output = crate::descriptors::ExternalSystemMetadataMappingOutput::default();
            for assignment in &plan.all_mappings {
                let source = if assignment.source_root == "metadata" {
                    inputs.metadata.get(&assignment.source_field)
                } else {
                    inputs.input.get(&assignment.source_field)
                };
                if let Some(value) = source {
                    if assignment.target_root == "client" {
                        output
                            .client_config
                            .insert(assignment.field.clone(), value.clone());
                    } else if assignment.target_root == "request" {
                        output
                            .request_payload
                            .insert(assignment.field.clone(), value.clone());
                    }
                }
            }
            Ok(output)
        }
    }

    #[test]
    fn generated_metadata_resolver_fixture() {
        let resolver = FixtureResolver {
            calls: Cell::new(0),
        };
        let mut tx = FixtureTx;
        let descriptors = crate::descriptors::external_system_descriptors();
        let plan = crate::descriptors::external_system_metadata_mapping_plan(&descriptors[0]);
        assert_eq!(plan.all_mappings.len(), 4);
        assert_eq!(plan.client_mappings.len(), 3);
        assert_eq!(plan.request_mappings.len(), 1);
        assert_eq!(plan.client_mappings[0].source_root, "metadata");
        assert_eq!(plan.client_mappings[0].source_field, "base_url");
        assert_eq!(plan.client_mappings[0].target_root, "client");
        assert_eq!(plan.client_mappings[0].field, "base_url");
        assert_eq!(plan.request_mappings[0].source_root, "input");
        assert_eq!(plan.request_mappings[0].source_field, "order_id");
        assert_eq!(plan.request_mappings[0].target_root, "request");
        assert_eq!(plan.request_mappings[0].field, "order_id");
        let applicator = FixtureMappingApplicator;
        let mapped = crate::descriptors::ExternalSystemMetadataMappingApplicator::apply_external_system_metadata_mappings(
            &applicator,
            &plan,
            &crate::descriptors::ExternalSystemMetadataMappingInputs {
                input: BTreeMap::from([(
                    "order_id".to_string(),
                    Json::String("order-1".to_string()),
                )]),
                metadata: BTreeMap::from([
                    (
                        "base_url".to_string(),
                        Json::String("https://api.stripe.test".to_string()),
                    ),
                    (
                        "auth_ref".to_string(),
                        Json::String("secret:stripe".to_string()),
                    ),
                    ("timeout_ms".to_string(), Json::Integer(5000)),
                ]),
                ..Default::default()
            },
        )
        .expect("mapping applicator should not fail");
        assert_eq!(mapped.client_config.len(), 3);
        assert_eq!(mapped.request_payload.len(), 1);
        let keys = vec![
            ExternalSystemMetadataKeyValue {
                field: "tenant_id".to_string(),
                value: Json::String("tenant-a".to_string()),
            },
            ExternalSystemMetadataKeyValue {
                field: "external_system_id".to_string(),
                value: Json::String("stripe".to_string()),
            },
            ExternalSystemMetadataKeyValue {
                field: "profile".to_string(),
                value: Json::String("default".to_string()),
            },
        ];

        let resolved = crate::descriptors::resolve_external_system_metadata_by_name_tx::<
            FixtureBackend,
            FixtureResolver,
        >(&resolver, &mut tx, "Billing.Stripe", keys.clone())
        .expect("resolver call should not fail")
        .expect("metadata should resolve");
        assert!(!resolved.complete());
        assert_eq!(resolver.calls.get(), 1);

        let skipped = crate::descriptors::resolve_external_system_metadata_by_name_tx::<
            FixtureBackend,
            FixtureResolver,
        >(&resolver, &mut tx, "Billing.Stripe", keys[..2].to_vec())
        .expect("resolver call should not fail");
        assert!(skipped.is_none());
        assert_eq!(resolver.calls.get(), 1);
    }
}
RUST
(cd "$TMPDIR/out-rust" && CARGO_TARGET_DIR="$TMPDIR/rust-target" cargo test)

# Include composition is used by binding generation.
run_expect_status 0 "$CLI" generate bindings --lang cpp "$INCLUDE_ROOT_SPEC" --out "$TMPDIR/out-include-cpp"
assert_output_contains "generated $TMPDIR/out-include-cpp/system_descriptors.hpp"
assert_file_exists "$TMPDIR/out-include-cpp/system_descriptors.hpp"
assert_file_contains "$TMPDIR/out-include-cpp/system_descriptors.hpp" "IncludedEntity"
assert_file_contains "$TMPDIR/out-include-cpp/system_descriptors.hpp" "IncludedQueue"
assert_file_contains "$TMPDIR/out-include-cpp/system_descriptors.hpp" "IncludedWorkflow"
assert_file_contains "$TMPDIR/out-include-cpp/system_descriptors.hpp" "IncludedEntityLaunch"

# Default output directories.
run_expect_status 0 "$CLI" generate bindings --lang cpp "$SPEC"
assert_output_contains "generated generated/cpp/json.hpp"
assert_file_exists "generated/cpp/backend.hpp"
assert_file_exists "generated/cpp/system_descriptors.hpp"
rm -rf generated/cpp
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang go "$SPEC"
assert_output_contains "generated generated/go/backend/json.go"
assert_file_exists "generated/go/backend/backend.go"
assert_file_exists "generated/go/backend/descriptors.go"
rm -rf generated/go
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang java "$SPEC"
assert_output_contains "generated generated/java/com/statespec/backend/Json.java"
assert_file_exists "generated/java/com/statespec/backend/Backend.java"
assert_file_exists "generated/java/com/statespec/generated/Descriptors.java"
rm -rf generated/java
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang rust "$SPEC"
assert_output_contains "generated generated/rust/json.rs"
assert_file_exists "generated/rust/backend.rs"
assert_file_exists "generated/rust/descriptors.rs"
rm -rf generated/rust
rmdir generated 2>/dev/null || true

# --lang is required.
run_expect_status 2 "$CLI" generate bindings "$SPEC"
assert_output_contains "generate bindings requires --lang <cpp|go|java|rust>"

# --lang requires a value.
run_expect_status 2 "$CLI" generate bindings --lang
assert_output_contains "--lang requires one of: cpp|go|java|rust"

# --lang must be supported.
run_expect_status 2 "$CLI" generate bindings --lang python "$SPEC"
assert_output_contains "unsupported binding language 'python'"
assert_output_contains "cpp|go|java|rust"

# Input file is required.
run_expect_status 2 "$CLI" generate bindings --lang cpp
assert_output_contains "generate bindings requires an input .sspec file"

# --out requires a value.
run_expect_status 2 "$CLI" generate bindings --lang cpp "$SPEC" --out
assert_output_contains "--out requires a directory"

# Extra positional arguments are rejected.
run_expect_status 2 "$CLI" generate bindings --lang cpp "$SPEC" extra
assert_output_contains "unexpected argument for generate bindings: extra"

# A system declaration is required for binding generation.
run_expect_status 1 "$CLI" generate bindings --lang cpp "$NO_SYSTEM_SPEC" --out "$TMPDIR/no-system"
assert_output_contains "SSPEC0201"
assert_output_contains "expected system declaration"

# Generic generation is not supported.
run_expect_status 2 "$CLI" generate "$SPEC" legacy
assert_output_contains "unsupported generate kind: $SPEC"

run_expect_status 2 "$CLI" generate legacy "$SPEC"
assert_output_contains "unsupported generate kind: legacy"
