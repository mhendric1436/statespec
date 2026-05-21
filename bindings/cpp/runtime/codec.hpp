#pragma once

#include "../feature_flag.hpp"
#include "../lease.hpp"
#include "../log.hpp"
#include "../metric.hpp"
#include "../queue.hpp"
#include "../workflow.hpp"

#include <chrono>
#include <cstdint>

namespace statespec::backend::runtime::detail
{

inline std::int64_t seconds_count(std::chrono::seconds value)
{
    return value.count();
}

inline std::chrono::seconds seconds_from(const Json& value)
{
    return std::chrono::seconds{value.as_int64()};
}

inline std::int64_t timestamp_millis(Timestamp value)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(value.time_since_epoch()).count();
}

inline Timestamp timestamp_from(const Json& value)
{
    return Timestamp{std::chrono::milliseconds{value.as_int64()}};
}

inline void put_optional_string(
    Json::Object& object,
    const std::string& key,
    const std::optional<std::string>& value
)
{
    if (value.has_value())
    {
        object.emplace(key, *value);
    }
}

inline std::optional<std::string> optional_string(
    const Json& object,
    const std::string& key
)
{
    if (const auto* value = object.find(key))
    {
        return value->as_string();
    }
    return std::nullopt;
}

inline Json feature_flag_value_to_json(const FeatureFlagValue& value)
{
    switch (value.type())
    {
    case FeatureFlagType::Bool:
        return Json::object({{"type", "bool"}, {"value", *value.as_bool()}});
    case FeatureFlagType::String:
        return Json::object({{"type", "string"}, {"value", *value.as_string()}});
    case FeatureFlagType::Integer:
        return Json::object({{"type", "integer"}, {"value", *value.as_integer()}});
    case FeatureFlagType::Decimal:
        return Json::object({{"type", "decimal"}, {"value", *value.as_decimal()}});
    }
    throw BackendError("unknown feature flag value type");
}

inline FeatureFlagValue feature_flag_value_from_json(const Json& json)
{
    const auto type = json.at("type").as_string();
    if (type == "bool")
    {
        return FeatureFlagValue::bool_value(json.at("value").as_bool());
    }
    if (type == "string")
    {
        return FeatureFlagValue::string_value(json.at("value").as_string());
    }
    if (type == "integer")
    {
        return FeatureFlagValue::integer_value(json.at("value").as_int64());
    }
    if (type == "decimal")
    {
        return FeatureFlagValue::decimal_value(json.at("value").as_double());
    }
    throw BackendError("unknown feature flag value type: " + type);
}

inline std::string feature_flag_type_name(FeatureFlagType type)
{
    switch (type)
    {
    case FeatureFlagType::Bool:
        return "bool";
    case FeatureFlagType::String:
        return "string";
    case FeatureFlagType::Integer:
        return "integer";
    case FeatureFlagType::Decimal:
        return "decimal";
    }
    throw BackendError("unknown feature flag type");
}

inline FeatureFlagType feature_flag_type_from_string(const std::string& value)
{
    if (value == "bool")
    {
        return FeatureFlagType::Bool;
    }
    if (value == "string")
    {
        return FeatureFlagType::String;
    }
    if (value == "integer")
    {
        return FeatureFlagType::Integer;
    }
    if (value == "decimal")
    {
        return FeatureFlagType::Decimal;
    }
    throw BackendError("unknown feature flag type: " + value);
}

inline std::string feature_flag_scope_name(FeatureFlagScopeKind scope)
{
    switch (scope)
    {
    case FeatureFlagScopeKind::System:
        return "system";
    case FeatureFlagScopeKind::Tenant:
        return "tenant";
    case FeatureFlagScopeKind::User:
        return "user";
    case FeatureFlagScopeKind::Entity:
        return "entity";
    }
    throw BackendError("unknown feature flag scope");
}

inline FeatureFlagScopeKind feature_flag_scope_from_string(const std::string& value)
{
    if (value == "system")
    {
        return FeatureFlagScopeKind::System;
    }
    if (value == "tenant")
    {
        return FeatureFlagScopeKind::Tenant;
    }
    if (value == "user")
    {
        return FeatureFlagScopeKind::User;
    }
    if (value == "entity")
    {
        return FeatureFlagScopeKind::Entity;
    }
    throw BackendError("unknown feature flag scope: " + value);
}

inline Json feature_flag_definition_to_json(const FeatureFlagDefinition& definition)
{
    Json::Object object{
        {"name", definition.name},
        {"type", feature_flag_type_name(definition.type)},
        {"default_value", feature_flag_value_to_json(definition.default_value)},
        {"scope", feature_flag_scope_name(definition.scope)},
    };
    put_optional_string(object, "owner", definition.owner);
    put_optional_string(object, "description", definition.description);
    put_optional_string(object, "expires", definition.expires);
    return Json::object(std::move(object));
}

inline FeatureFlagDefinition feature_flag_definition_from_json(const Json& json)
{
    return FeatureFlagDefinition{
        .name = json.at("name").as_string(),
        .type = feature_flag_type_from_string(json.at("type").as_string()),
        .default_value = feature_flag_value_from_json(json.at("default_value")),
        .scope = feature_flag_scope_from_string(json.at("scope").as_string()),
        .owner = optional_string(json, "owner"),
        .description = optional_string(json, "description"),
        .expires = optional_string(json, "expires"),
    };
}

inline Json queue_definition_to_json(const QueueDefinition& definition)
{
    Json::Object object{
        {"queue", definition.queue},
        {"channel", definition.channel},
        {"visibility_timeout_seconds", seconds_count(definition.visibility_timeout)},
        {"max_attempts", static_cast<std::int64_t>(definition.max_attempts)},
        {"metadata", definition.metadata},
    };
    put_optional_string(object, "dead_letter_queue", definition.dead_letter_queue);
    return Json::object(std::move(object));
}

inline QueueDefinition queue_definition_from_json(const Json& json)
{
    QueueDefinition definition;
    definition.queue = json.at("queue").as_string();
    definition.channel = json.at("channel").as_string();
    definition.visibility_timeout = seconds_from(json.at("visibility_timeout_seconds"));
    definition.max_attempts = static_cast<std::uint32_t>(json.at("max_attempts").as_int64());
    definition.dead_letter_queue = optional_string(json, "dead_letter_queue");
    definition.metadata = json.contains("metadata") ? json.at("metadata") : Json::object({});
    return definition;
}

inline Json queue_message_to_json(const QueueMessageRecord& message)
{
    Json::Object object{
        {"message_id", message.message_id},
        {"queue", message.queue},
        {"channel", message.channel},
        {"status", message.status},
        {"attempts", static_cast<std::int64_t>(message.attempts)},
        {"payload", message.payload},
    };
    put_optional_string(object, "claimed_by", message.claimed_by);
    if (message.claim_expires_at.has_value())
    {
        object.emplace("claim_expires_at_ms", timestamp_millis(*message.claim_expires_at));
    }
    return Json::object(std::move(object));
}

inline QueueMessageRecord queue_message_from_json(const Json& json)
{
    QueueMessageRecord message;
    message.message_id = json.at("message_id").as_string();
    message.queue = json.at("queue").as_string();
    message.channel = json.at("channel").as_string();
    message.status = json.at("status").as_string();
    message.attempts = static_cast<std::uint64_t>(json.at("attempts").as_int64());
    message.claimed_by = optional_string(json, "claimed_by");
    if (const auto* value = json.find("claim_expires_at_ms"))
    {
        message.claim_expires_at = timestamp_from(*value);
    }
    message.payload = json.contains("payload") ? json.at("payload") : Json::object({});
    return message;
}

inline Json lease_definition_id_to_json(const LeaseDefinitionId& id)
{
    return Json::object({{"name", id.name}, {"version", static_cast<std::int64_t>(id.version)}});
}

inline LeaseDefinitionId lease_definition_id_from_json(const Json& json)
{
    return LeaseDefinitionId{
        .name = json.at("name").as_string(),
        .version = static_cast<std::uint64_t>(json.at("version").as_int64()),
    };
}

inline Json lease_definition_to_json(const LeaseDefinition& definition)
{
    Json::Object object{
        {"id", lease_definition_id_to_json(definition.id)},
        {"resource_pattern", definition.resource_pattern},
        {"ttl_seconds", seconds_count(definition.ttl)},
        {"renew_every_seconds", seconds_count(definition.renew_every)},
        {"fencing_token", definition.fencing_token},
    };
    if (definition.max_ttl.has_value())
    {
        object.emplace("max_ttl_seconds", seconds_count(*definition.max_ttl));
    }
    return Json::object(std::move(object));
}

inline LeaseDefinition lease_definition_from_json(const Json& json)
{
    LeaseDefinition definition;
    definition.id = lease_definition_id_from_json(json.at("id"));
    definition.resource_pattern = json.at("resource_pattern").as_string();
    definition.ttl = seconds_from(json.at("ttl_seconds"));
    definition.renew_every = seconds_from(json.at("renew_every_seconds"));
    if (const auto* value = json.find("max_ttl_seconds"))
    {
        definition.max_ttl = seconds_from(*value);
    }
    definition.fencing_token = json.at("fencing_token").as_bool();
    return definition;
}

inline Json lease_record_to_json(const LeaseRecord& lease)
{
    Json::Object object{
        {"definition_id", lease_definition_id_to_json(lease.definition_id)},
        {"resource", lease.resource},
        {"expires_at_ms", timestamp_millis(lease.expires_at)},
        {"fencing_token", static_cast<std::int64_t>(lease.fencing_token)},
    };
    put_optional_string(object, "holder", lease.holder);
    return Json::object(std::move(object));
}

inline LeaseRecord lease_record_from_json(const Json& json)
{
    LeaseRecord record;
    record.definition_id = lease_definition_id_from_json(json.at("definition_id"));
    record.resource = json.at("resource").as_string();
    record.holder = optional_string(json, "holder");
    record.expires_at = timestamp_from(json.at("expires_at_ms"));
    record.fencing_token = static_cast<std::uint64_t>(json.at("fencing_token").as_int64());
    return record;
}

inline Json workflow_step_definition_to_json(const WorkflowStepDefinition& step)
{
    return Json::object(
        {{"name", step.name},
         {"expected_execution_time_seconds", seconds_count(step.expected_execution_time)},
         {"max_retries", static_cast<std::int64_t>(step.max_retries)}}
    );
}

inline WorkflowStepDefinition workflow_step_definition_from_json(const Json& json)
{
    return WorkflowStepDefinition{
        .name = json.at("name").as_string(),
        .expected_execution_time = seconds_from(json.at("expected_execution_time_seconds")),
        .max_retries = static_cast<std::uint32_t>(json.at("max_retries").as_int64()),
    };
}

inline Json workflow_definition_to_json(const WorkflowDefinition& definition)
{
    Json::Array steps;
    for (const auto& step : definition.steps)
    {
        steps.push_back(workflow_step_definition_to_json(step));
    }
    return Json::object(
        {{"workflow_name", definition.workflow_name},
         {"workflow_version", definition.workflow_version},
         {"start_step", definition.start_step},
         {"expected_execution_time_seconds", seconds_count(definition.expected_execution_time)},
         {"singleton", definition.singleton},
         {"steps", Json::array(std::move(steps))},
         {"metadata", definition.metadata}}
    );
}

inline WorkflowDefinition workflow_definition_from_json(const Json& json)
{
    WorkflowDefinition definition;
    definition.workflow_name = json.at("workflow_name").as_string();
    definition.workflow_version = json.at("workflow_version").as_int64();
    definition.start_step = json.at("start_step").as_string();
    definition.expected_execution_time = seconds_from(json.at("expected_execution_time_seconds"));
    definition.singleton = json.at("singleton").as_bool();
    for (const auto& step : json.at("steps").as_array())
    {
        definition.steps.push_back(workflow_step_definition_from_json(step));
    }
    definition.metadata = json.contains("metadata") ? json.at("metadata") : Json::object({});
    return definition;
}

inline Json workflow_execution_to_json(const WorkflowExecutionRecord& execution)
{
    Json::Object object{
        {"workflow_execution_id", execution.workflow_execution_id},
        {"workflow_name", execution.workflow_name},
        {"workflow_version", execution.workflow_version},
        {"current_step", execution.current_step},
        {"status", execution.status},
        {"attempt", static_cast<std::int64_t>(execution.attempt)},
        {"state", execution.state},
    };
    put_optional_string(object, "claimed_by", execution.claimed_by);
    if (execution.claim_expires_at.has_value())
    {
        object.emplace("claim_expires_at_ms", timestamp_millis(*execution.claim_expires_at));
    }
    return Json::object(std::move(object));
}

inline WorkflowExecutionRecord workflow_execution_from_json(const Json& json)
{
    WorkflowExecutionRecord execution;
    execution.workflow_execution_id = json.at("workflow_execution_id").as_string();
    execution.workflow_name = json.at("workflow_name").as_string();
    execution.workflow_version = json.at("workflow_version").as_int64();
    execution.current_step = json.at("current_step").as_string();
    execution.status = json.at("status").as_string();
    execution.attempt = static_cast<std::uint64_t>(json.at("attempt").as_int64());
    execution.claimed_by = optional_string(json, "claimed_by");
    if (const auto* value = json.find("claim_expires_at_ms"))
    {
        execution.claim_expires_at = timestamp_from(*value);
    }
    execution.state = json.contains("state") ? json.at("state") : Json::object({});
    return execution;
}

inline std::string log_level_name(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug:
        return "debug";
    case LogLevel::Info:
        return "info";
    case LogLevel::Warn:
        return "warn";
    case LogLevel::Error:
        return "error";
    }
    throw BackendError("unknown log level");
}

inline LogLevel log_level_from_string(const std::string& value)
{
    if (value == "debug")
    {
        return LogLevel::Debug;
    }
    if (value == "info")
    {
        return LogLevel::Info;
    }
    if (value == "warn")
    {
        return LogLevel::Warn;
    }
    if (value == "error")
    {
        return LogLevel::Error;
    }
    throw BackendError("unknown log level: " + value);
}

inline Json log_definition_to_json(const LogDefinition& definition)
{
    return Json::object(
        {{"name", definition.name},
         {"level", log_level_name(definition.level)},
         {"event_name", definition.event_name}}
    );
}

inline LogDefinition log_definition_from_json(const Json& json)
{
    return LogDefinition{
        .name = json.at("name").as_string(),
        .level = log_level_from_string(json.at("level").as_string()),
        .event_name = json.at("event_name").as_string(),
    };
}

inline Json log_event_to_json(const LogEvent& event)
{
    return Json::object(
        {{"name", event.name},
         {"level", log_level_name(event.level)},
         {"event_name", event.event_name},
         {"fields", event.fields}}
    );
}

inline LogEvent log_event_from_json(const Json& json)
{
    return LogEvent{
        .name = json.at("name").as_string(),
        .level = log_level_from_string(json.at("level").as_string()),
        .event_name = json.at("event_name").as_string(),
        .fields = json.contains("fields") ? json.at("fields") : Json::object({}),
    };
}

inline std::string metric_kind_name(MetricKind kind)
{
    switch (kind)
    {
    case MetricKind::Counter:
        return "counter";
    case MetricKind::Gauge:
        return "gauge";
    case MetricKind::Histogram:
        return "histogram";
    }
    throw BackendError("unknown metric kind");
}

inline MetricKind metric_kind_from_string(const std::string& value)
{
    if (value == "counter")
    {
        return MetricKind::Counter;
    }
    if (value == "gauge")
    {
        return MetricKind::Gauge;
    }
    if (value == "histogram")
    {
        return MetricKind::Histogram;
    }
    throw BackendError("unknown metric kind: " + value);
}

inline Json metric_definition_to_json(const MetricDefinition& definition)
{
    return Json::object(
        {{"name", definition.name},
         {"kind", metric_kind_name(definition.kind)},
         {"backend_name", definition.backend_name},
         {"unit", definition.unit}}
    );
}

inline MetricDefinition metric_definition_from_json(const Json& json)
{
    return MetricDefinition{
        .name = json.at("name").as_string(),
        .kind = metric_kind_from_string(json.at("kind").as_string()),
        .backend_name = json.at("backend_name").as_string(),
        .unit = json.at("unit").as_string(),
    };
}

inline Json metric_sample_to_json(const MetricSample& sample)
{
    return Json::object(
        {{"name", sample.name},
         {"kind", metric_kind_name(sample.kind)},
         {"backend_name", sample.backend_name},
         {"value", sample.value},
         {"unit", sample.unit},
         {"labels", sample.labels}}
    );
}

inline MetricSample metric_sample_from_json(const Json& json)
{
    return MetricSample{
        .name = json.at("name").as_string(),
        .kind = metric_kind_from_string(json.at("kind").as_string()),
        .backend_name = json.at("backend_name").as_string(),
        .value = json.at("value").as_double(),
        .unit = json.at("unit").as_string(),
        .labels = json.contains("labels") ? json.at("labels") : Json::object({}),
    };
}

} // namespace statespec::backend::runtime::detail
