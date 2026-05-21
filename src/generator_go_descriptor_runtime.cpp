#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_go_runtime_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "func QueueDefinitions() []QueueDefinition {\n";
    out << "\treturn []QueueDefinition{\n";
    for (const auto& queue : system.queues)
    {
        out << "\t\t{\n";
        out << "\t\t\tQueue: " << go_string(queue.name) << ",\n";
        out << "\t\t\tChannel: " << go_string(queue.channel.value_or("default")) << ",\n";
        out << "\t\t\tVisibilityTimeout: " << parse_go_duration_seconds(queue.visibility_timeout)
            << " * time.Second,\n";
        out << "\t\t\tMaxAttempts: " << queue.max_attempts.value_or(1) << ",\n";
        out << "\t\t\tDeadLetterQueue: " << string_ptr_expr(queue.dead_letter) << ",\n";
        out << "\t\t\tMetadata: JSONObject(map[string]JSON{}),\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func LeaseDefinitions() []LeaseDescriptor {\n";
    out << "\treturn []LeaseDescriptor{\n";
    for (const auto& lease : system.leases)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(lease.name) << ",\n";
        out << "\t\t\tResource: " << string_ptr_expr(lease.resource) << ",\n";
        out << "\t\t\tTTL: " << parse_go_duration_seconds(lease.ttl) << " * time.Second,\n";
        out << "\t\t\tRenewEvery: " << duration_ptr_expr(lease.renew_every) << ",\n";
        out << "\t\t\tHolder: " << string_ptr_expr(lease.holder) << ",\n";
        out << "\t\t\tFencingToken: " << (lease.fencing_token.value_or(false) ? "true" : "false")
            << ",\n";
        out << "\t\t\tMaxTTL: " << duration_ptr_expr(lease.max_ttl) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func WorkflowDefinitions() []WorkflowDefinition {\n";
    out << "\treturn []WorkflowDefinition{\n";
    for (const auto& workflow : system.workflows)
    {
        out << "\t\t{\n";
        out << "\t\t\tWorkflowName: " << go_string(workflow.name) << ",\n";
        out << "\t\t\tWorkflowVersion: " << workflow.version.value_or(1) << ",\n";
        out << "\t\t\tStartStep: " << go_string(workflow.start_step.value_or("")) << ",\n";
        out << "\t\t\tExpectedExecutionTime: "
            << parse_go_duration_seconds(workflow.expected_execution_time) << " * time.Second,\n";
        out << "\t\t\tSingleton: " << (workflow.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "\t\t\tSteps: []WorkflowStepDefinition{\n";
        for (const auto& step : workflow.steps)
        {
            out << "\t\t\t\t{Name: " << go_string(step.name) << ", ExpectedExecutionTime: "
                << parse_go_duration_seconds(step.expected_execution_time)
                << " * time.Second, MaxRetries: " << step.max_retries.value_or(0) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tMetadata: JSONObject(map[string]JSON{}),\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n";

    return out.str();
}

} // namespace statespec
