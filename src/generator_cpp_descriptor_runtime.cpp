#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"
#include "generator_workflow_metadata.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_runtime_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline std::vector<statespec::backend::QueueDefinition> queue_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& queue : system.queues)
    {
        out << "        statespec::backend::QueueDefinition{\n";
        out << "            " << cpp_string(queue.name) << ",\n";
        out << "            " << cpp_string(queue.channel.value_or("default")) << ",\n";
        out << "            std::chrono::seconds{"
            << parse_duration_seconds(queue.visibility_timeout) << "},\n";
        out << "            " << queue.max_attempts.value_or(1) << ",\n";
        out << "            " << optional_string_expr(queue.dead_letter) << ",\n";
        out << "            \"{}\",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<statespec::backend::LeaseDefinition> lease_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& lease : system.leases)
    {
        out << "        statespec::backend::LeaseDefinition{\n";
        out << "            statespec::backend::LeaseDefinitionId{" << cpp_string(lease.name)
            << ", 1},\n";
        out << "            " << cpp_string(lease.resource_pattern) << ",\n";
        out << "            std::chrono::seconds{" << lease.ttl_seconds << "},\n";
        out << "            std::chrono::seconds{" << lease.renew_every_seconds << "},\n";
        if (lease.max_ttl_seconds.has_value())
        {
            out << "            std::optional<std::chrono::seconds>{std::chrono::seconds{"
                << *lease.max_ttl_seconds << "}},\n";
        }
        else
        {
            out << "            std::nullopt,\n";
        }
        out << "            " << (lease.fencing_token_enabled ? "true" : "false") << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << generate_cpp_workflow_descriptor_umbrella(system);
    return out.str();
}

std::string generate_cpp_workflow_descriptor(const IrWorkflow& workflow)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../workflow.hpp\"\n\n";
    out << "#include <chrono>\n";
    out << "#include <vector>\n\n";
    out << "namespace statespec_generated\n";
    out << "{\n\n";
    out << "inline statespec::backend::WorkflowDefinition " << snake_identifier(workflow.name)
        << "_workflow_definition()\n";
    out << "{\n";
    out << "    return statespec::backend::WorkflowDefinition{\n";
    out << "        " << cpp_string(workflow.name) << ",\n";
    out << "        " << workflow.version.value_or(1) << ",\n";
    out << "        " << cpp_string(workflow.start_step.value_or("")) << ",\n";
    out << "        std::chrono::seconds{"
        << parse_duration_seconds(workflow.expected_execution_time) << "},\n";
    out << "        " << (workflow.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "        {\n";
    for (const auto& step : workflow.steps)
    {
        out << "            statespec::backend::WorkflowStepDefinition{" << cpp_string(step.name)
            << ", std::chrono::seconds{" << parse_duration_seconds(step.expected_execution_time)
            << "}, " << step.max_retries.value_or(0) << "},\n";
    }
    for (const auto& phase : workflow_synthetic_child_phases(workflow))
    {
        out << "            statespec::backend::WorkflowStepDefinition{"
            << cpp_string(phase.step_name) << ", std::chrono::seconds{0}, 0},\n";
    }
    out << "        },\n";
    out << "        statespec::backend::Json::parse("
        << cpp_string(workflow_descriptor_metadata_json(workflow)) << "),\n";
    out << "    };\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated\n";
    return out.str();
}

std::string generate_cpp_workflow_descriptor_umbrella(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline std::vector<statespec::backend::WorkflowDefinition> workflow_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& workflow : system.workflows)
    {
        out << "        " << snake_identifier(workflow.name) << "_workflow_definition(),\n";
    }
    out << "    };\n";
    out << "}\n\n";
    return out.str();
}

} // namespace statespec
