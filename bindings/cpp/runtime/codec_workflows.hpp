#pragma once

#include "codec_core.hpp"

#include "../workflow.hpp"

namespace statespec::backend::runtime::detail
{

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

} // namespace statespec::backend::runtime::detail
