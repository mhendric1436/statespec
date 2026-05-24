#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_runtime_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<QueueDefinition> queueDefinitions() {\n";
    out << "        return List.of(\n";
    if (!system.queues.empty())
    {
        for (std::size_t i = 0; i < system.queues.size(); ++i)
        {
            const auto& queue = system.queues[i];
            out << "            new QueueDefinition(\n";
            out << "                " << java_string(queue.name) << ",\n";
            out << "                " << java_string(queue.channel.value_or("default")) << ",\n";
            out << "                Duration.ofSeconds("
                << parse_java_duration_seconds(queue.visibility_timeout) << "L),\n";
            out << "                " << queue.max_attempts.value_or(1) << ",\n";
            out << "                " << java_optional_string_expr(queue.dead_letter) << ",\n";
            out << "                \"{}\"\n";
            out << "            )" << (i + 1 < system.queues.size() ? "," : "") << "\n";
        }
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<LeaseDefinition> leaseDefinitions() {\n";
    out << "        return List.of(\n";
    if (!system.leases.empty())
    {
        for (std::size_t i = 0; i < system.leases.size(); ++i)
        {
            const auto& lease = system.leases[i];
            out << "            new LeaseDefinition(\n";
            out << "                " << java_string(lease.name) << ",\n";
            out << "                " << java_optional_string_expr(lease.resource) << ",\n";
            out << "                Duration.ofSeconds(" << parse_java_duration_seconds(lease.ttl)
                << "L),\n";
            out << "                " << optional_duration_expr(lease.renew_every) << ",\n";
            out << "                " << java_optional_string_expr(lease.holder) << ",\n";
            out << "                " << (lease.fencing_token.value_or(false) ? "true" : "false")
                << ",\n";
            out << "                " << optional_duration_expr(lease.max_ttl) << "\n";
            out << "            )" << (i + 1 < system.leases.size() ? "," : "") << "\n";
        }
    }
    out << "        );\n";
    out << "    }\n\n";

    out << generate_java_workflow_descriptor_umbrella(system);
    return out.str();
}

std::string generate_java_workflow_descriptor(
    const IrWorkflow& workflow,
    std::string_view package_name,
    std::string_view class_name
)
{
    std::ostringstream out;
    out << "package " << package_name << ";\n\n";
    out << "import com.statespec.backend.Workflow.WorkflowDefinition;\n";
    out << "import com.statespec.backend.Workflow.WorkflowStepDefinition;\n";
    out << "import java.time.Duration;\n";
    out << "import java.util.List;\n\n";
    out << "public final class " << class_name << " {\n";
    out << "    private " << class_name << "() {}\n\n";
    out << "    public static WorkflowDefinition workflowDefinition() {\n";
    out << "        return new WorkflowDefinition(\n";
    out << "            " << java_string(workflow.name) << ",\n";
    out << "            " << workflow.version.value_or(1) << "L,\n";
    out << "            " << java_string(workflow.start_step.value_or("")) << ",\n";
    out << "            Duration.ofSeconds("
        << parse_java_duration_seconds(workflow.expected_execution_time) << "L),\n";
    out << "            " << (workflow.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "            List.of(\n";
    for (std::size_t step_index = 0; step_index < workflow.steps.size(); ++step_index)
    {
        const auto& step = workflow.steps[step_index];
        out << "                new WorkflowStepDefinition(" << java_string(step.name)
            << ", Duration.ofSeconds(" << parse_java_duration_seconds(step.expected_execution_time)
            << "L), " << step.max_retries.value_or(0) << ")";
        out << (step_index + 1 < workflow.steps.size() ? "," : "") << "\n";
    }
    out << "            ),\n";
    out << "            \"{}\"\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string generate_java_workflow_descriptor_umbrella(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<WorkflowDefinition> workflowDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t i = 0; i < system.workflows.size(); ++i)
    {
        const auto& workflow = system.workflows[i];
        out << "            " << pascal_identifier(workflow.name)
            << "DescriptorModule.workflowDefinition()"
            << (i + 1 < system.workflows.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n";
    return out.str();
}

} // namespace statespec
