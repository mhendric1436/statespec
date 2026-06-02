#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"
#include "generator_workflow_metadata.hpp"
#include "identifier_case.hpp"

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

    out << "func LeaseDefinitions() []LeaseDefinition {\n";
    out << "\treturn []LeaseDefinition{\n";
    for (const auto& lease : system.leases)
    {
        out << "\t\t{\n";
        out << "\t\t\tID: LeaseDefinitionID{Name: " << go_string(lease.name) << ", Version: 1},\n";
        out << "\t\t\tResourcePattern: " << go_string(lease.resource_pattern) << ",\n";
        out << "\t\t\tTTL: " << lease.ttl_seconds << " * time.Second,\n";
        out << "\t\t\tRenewEvery: " << lease.renew_every_seconds << " * time.Second,\n";
        out << "\t\t\tFencingToken: " << (lease.fencing_token_enabled ? "true" : "false") << ",\n";
        if (lease.max_ttl_seconds.has_value())
        {
            out << "\t\t\tMaxTTL: durationPtr(" << *lease.max_ttl_seconds << " * time.Second),\n";
        }
        else
        {
            out << "\t\t\tMaxTTL: nil,\n";
        }
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << generate_go_workflow_descriptor_umbrella(system);

    return out.str();
}

std::string generate_go_workflow_descriptor(const IrWorkflow& workflow)
{
    std::ostringstream out;
    out << "package workflows\n\n";
    out << "import \"time\"\n\n";
    out << "func " << pascal_identifier(workflow.name)
        << "WorkflowDefinition() WorkflowDefinition {\n";
    out << "\treturn WorkflowDefinition{\n";
    out << "\t\tWorkflowName: " << go_string(workflow.name) << ",\n";
    out << "\t\tWorkflowVersion: " << workflow.version.value_or(1) << ",\n";
    out << "\t\tStartStep: " << go_string(workflow.start_step.value_or("")) << ",\n";
    out << "\t\tExpectedExecutionTime: "
        << parse_go_duration_seconds(workflow.expected_execution_time) << " * time.Second,\n";
    out << "\t\tSingleton: " << (workflow.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "\t\tSteps: []WorkflowStepDefinition{\n";
    for (const auto& step : workflow.steps)
    {
        out << "\t\t\t{Name: " << go_string(step.name) << ", ExpectedExecutionTime: "
            << parse_go_duration_seconds(step.expected_execution_time)
            << " * time.Second, MaxRetries: " << step.max_retries.value_or(0) << "},\n";
    }
    for (const auto& phase : workflow_synthetic_child_phases(workflow))
    {
        out << "\t\t\t{Name: " << go_string(phase.step_name)
            << ", ExpectedExecutionTime: 0 * time.Second, MaxRetries: 0},\n";
    }
    out << "\t\t},\n";
    out << "\t\tMetadataJSON: " << go_string(workflow_descriptor_metadata_json(workflow)) << ",\n";
    out << "\t}\n";
    out << "}\n";
    return out.str();
}

std::string generate_go_workflow_descriptor_umbrella(const IrSystem& system)
{
    std::ostringstream out;
    out << "func WorkflowDefinitions() []WorkflowDefinition {\n";
    out << "\treturn []WorkflowDefinition{\n";
    for (const auto& workflow : system.workflows)
    {
        out << "\t\ttoWorkflowDefinition(workflows." << pascal_identifier(workflow.name)
            << "WorkflowDefinition()),\n";
    }
    out << "\t}\n";
    out << "}\n";
    if (system.workflows.empty())
    {
        return out.str();
    }
    out << "\n";
    out << "func toWorkflowDefinition(definition workflows.WorkflowDefinition) WorkflowDefinition "
           "{\n";
    out << "\tsteps := make([]WorkflowStepDefinition, 0, len(definition.Steps))\n";
    out << "\tfor _, step := range definition.Steps {\n";
    out << "\t\tsteps = append(steps, WorkflowStepDefinition{Name: step.Name, "
           "ExpectedExecutionTime: step.ExpectedExecutionTime, MaxRetries: step.MaxRetries})\n";
    out << "\t}\n";
    out << "\tmetadata, err := ParseJSON(definition.MetadataJSON)\n";
    out << "\tif err != nil {\n";
    out << "\t\tpanic(\"generated workflow metadata is invalid for \" + "
           "definition.WorkflowName + \": \" + err.Error())\n";
    out << "\t}\n";
    out << "\treturn WorkflowDefinition{\n";
    out << "\t\tWorkflowName: definition.WorkflowName,\n";
    out << "\t\tWorkflowVersion: definition.WorkflowVersion,\n";
    out << "\t\tStartStep: definition.StartStep,\n";
    out << "\t\tExpectedExecutionTime: definition.ExpectedExecutionTime,\n";
    out << "\t\tSingleton: definition.Singleton,\n";
    out << "\t\tSteps: steps,\n";
    out << "\t\tMetadata: metadata,\n";
    out << "\t}\n";
    out << "}\n";
    return out.str();
}

} // namespace statespec
