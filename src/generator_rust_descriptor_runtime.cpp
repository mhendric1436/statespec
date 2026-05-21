#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_runtime_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn collection_descriptors() -> Vec<CollectionDescriptor> {\n";
    out << "    vec![\n";

    for (const auto& entity : system.entities)
    {
        out << "        CollectionDescriptor {\n";
        out << "            name: " << rust_string(entity.name) << ".to_string(),\n";
        out << "            fields: vec![\n";
        for (const auto& field : entity.fields)
        {
            out << "                " << rust_field_descriptor_expr(field) << ",\n";
        }
        out << "            ],\n";
        out << "            key_fields: vec![";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(entity.key_fields[i]) << ".to_string()";
        }
        out << "],\n";
        out << "            indexes: vec![\n";
        for (const auto& index : entity.indexes)
        {
            out << "                IndexDescriptor {\n";
            out << "                    name: " << rust_string(index.name) << ".to_string(),\n";
            out << "                    fields: vec![";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(index.fields[i]) << ".to_string()";
            }
            out << "],\n";
            out << "                    unique: " << (index.unique ? "true" : "false") << ",\n";
            out << "                },\n";
        }
        out << "            ],\n";
        out << "            schema_version: 1,\n";
        out << "        },\n";
    }

    out << "    ]\n";
    out << "}\n\n";

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
        out << "            name: " << rust_string(lease.name) << ".to_string(),\n";
        out << "            resource: " << rust_optional_string_expr(lease.resource) << ",\n";
        out << "            ttl: Duration::from_secs(" << parse_rust_duration_seconds(lease.ttl)
            << "),\n";
        out << "            renew_every: " << rust_optional_duration_expr(lease.renew_every)
            << ",\n";
        out << "            holder: " << rust_optional_string_expr(lease.holder) << ",\n";
        out << "            fencing_token: "
            << (lease.fencing_token.value_or(false) ? "true" : "false") << ",\n";
        out << "            max_ttl: " << rust_optional_duration_expr(lease.max_ttl) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn workflow_definitions() -> Vec<WorkflowDefinition> {\n";
    out << "    vec![\n";
    for (const auto& workflow : system.workflows)
    {
        out << "        WorkflowDefinition {\n";
        out << "            workflow_name: " << rust_string(workflow.name) << ".to_string(),\n";
        out << "            workflow_version: " << workflow.version.value_or(1) << ",\n";
        out << "            start_step: " << rust_string(workflow.start_step.value_or(""))
            << ".to_string(),\n";
        out << "            expected_execution_time: Duration::from_secs("
            << parse_rust_duration_seconds(workflow.expected_execution_time) << "),\n";
        out << "            singleton: " << (workflow.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "            steps: vec![\n";
        for (const auto& step : workflow.steps)
        {
            out << "                WorkflowStepDefinition { name: " << rust_string(step.name)
                << ".to_string(), expected_execution_time: Duration::from_secs("
                << parse_rust_duration_seconds(step.expected_execution_time)
                << "), max_retries: " << step.max_retries.value_or(0) << " },\n";
        }
        out << "            ],\n";
        out << "            metadata: Json::Object(BTreeMap::new()),\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n";

    return out.str();
}

} // namespace statespec
