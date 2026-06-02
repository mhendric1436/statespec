#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"
#include "generator_workflow_metadata.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_runtime_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn queue_definitions() -> Vec<QueueDefinition> {\n";
    out << "    vec![\n";
    for (const auto& queue : system.queues)
    {
        out << "        QueueDefinition {\n";
        out << "            queue: " << rust_string(queue.name) << ".to_string(),\n";
        out << "            channel: " << rust_string(queue.channel.value_or("default"))
            << ".to_string(),\n";
        out << "            visibility_timeout: Duration::from_secs("
            << parse_rust_duration_seconds(queue.visibility_timeout) << "),\n";
        out << "            max_attempts: " << queue.max_attempts.value_or(1) << ",\n";
        out << "            dead_letter_queue: " << rust_optional_string_expr(queue.dead_letter)
            << ",\n";
        out << "            metadata: Json::Object(BTreeMap::new()),\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn lease_definitions() -> Vec<LeaseDefinition> {\n";
    out << "    vec![\n";
    for (const auto& lease : system.leases)
    {
        out << "        LeaseDefinition {\n";
        out << "            id: LeaseDefinitionId { name: " << rust_string(lease.name)
            << ".to_string(), version: 1 },\n";
        out << "            resource_pattern: " << rust_string(lease.resource_pattern)
            << ".to_string(),\n";
        out << "            ttl: Duration::from_secs(" << lease.ttl_seconds << "),\n";
        out << "            renew_every: Duration::from_secs(" << lease.renew_every_seconds
            << "),\n";
        if (lease.max_ttl_seconds.has_value())
        {
            out << "            max_ttl: Some(Duration::from_secs(" << *lease.max_ttl_seconds
                << ")),\n";
        }
        else
        {
            out << "            max_ttl: None,\n";
        }
        out << "            fencing_token: " << (lease.fencing_token_enabled ? "true" : "false")
            << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << generate_rust_workflow_descriptor_umbrella(system);
    return out.str();
}

std::string generate_rust_workflow_descriptor(const IrWorkflow& workflow)
{
    std::ostringstream out;
    out << "use std::time::Duration;\n\n";
    out << "use crate::json::Json;\n";
    out << "use crate::workflow::{WorkflowDefinition, WorkflowStepDefinition};\n\n";
    out << "pub fn workflow_definition() -> WorkflowDefinition {\n";
    out << "    WorkflowDefinition {\n";
    out << "        workflow_name: " << rust_string(workflow.name) << ".to_string(),\n";
    out << "        workflow_version: " << workflow.version.value_or(1) << ",\n";
    out << "        start_step: " << rust_string(workflow.start_step.value_or(""))
        << ".to_string(),\n";
    out << "        expected_execution_time: Duration::from_secs("
        << parse_rust_duration_seconds(workflow.expected_execution_time) << "),\n";
    out << "        singleton: " << (workflow.singleton.value_or(false) ? "true" : "false")
        << ",\n";
    out << "        steps: vec![\n";
    for (const auto& step : workflow.steps)
    {
        out << "            WorkflowStepDefinition { name: " << rust_string(step.name)
            << ".to_string(), expected_execution_time: Duration::from_secs("
            << parse_rust_duration_seconds(step.expected_execution_time)
            << "), max_retries: " << step.max_retries.value_or(0) << " },\n";
    }
    out << "        ],\n";
    out << "        metadata: Json::parse("
        << rust_string(workflow_descriptor_metadata_json(workflow)) << ").unwrap(),\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string generate_rust_workflow_descriptor_umbrella(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn workflow_definitions() -> Vec<WorkflowDefinition> {\n";
    out << "    vec![\n";
    for (const auto& workflow : system.workflows)
    {
        out << "        workflow_" << snake_identifier(workflow.name)
            << "::workflow_definition(),\n";
    }
    out << "    ]\n";
    out << "}\n";
    return out.str();
}

} // namespace statespec
