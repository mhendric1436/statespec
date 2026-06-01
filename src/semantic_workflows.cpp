#include "semantic_workflows.hpp"

#include "semantic_symbols.hpp"

#include <utility>

namespace statespec
{

namespace
{

std::optional<SemanticReference> resolve_workflow_statement_target(
    const SymbolTable& symbols,
    const std::string& workflow_name,
    const WorkflowStatementDecl& statement
)
{
    if (!statement.target.has_value())
    {
        return std::nullopt;
    }

    if (statement.kind == "transition_to")
    {
        return resolve_reference(symbols, workflow_name + "." + *statement.target);
    }

    return resolve_reference(symbols, *statement.target);
}

SemanticWorkflowStatement resolve_workflow_statement(
    const SymbolTable& symbols,
    const std::string& workflow_name,
    const WorkflowStatementDecl& statement
)
{
    SemanticWorkflowStatement resolved;
    resolved.kind = statement.kind;
    resolved.target = statement.target;
    resolved.resolved_target = resolve_workflow_statement_target(symbols, workflow_name, statement);
    resolved.expression = statement.expression;
    resolved.assignable = statement.assignable;
    resolved.from_assignable = statement.from_assignable;
    resolved.to_assignable = statement.to_assignable;
    resolved.binding = statement.binding;
    for (const auto& assignment : statement.payload)
    {
        resolved.payload.push_back(
            SemanticWorkflowAssignment{
                assignment.name,
                assignment.expression,
            }
        );
    }
    for (const auto& nested : statement.statements)
    {
        resolved.statements.push_back(resolve_workflow_statement(symbols, workflow_name, nested));
    }
    return resolved;
}

} // namespace

void resolve_semantic_workflows(
    const SystemDecl& system,
    const SymbolTable& symbols,
    SemanticSystem& resolved
)
{
    for (const auto& workflow : system.workflows)
    {
        SemanticWorkflow resolved_workflow;
        resolved_workflow.name = workflow.name;
        resolved_workflow.version = workflow.version;
        resolved_workflow.singleton = workflow.singleton;
        resolved_workflow.expected_execution_time = workflow.expected_execution_time;
        resolved_workflow.start_step = workflow.start_step;
        resolved_workflow.on = resolve_optional_reference(symbols, workflow.on);
        resolved_workflow.input = resolve_optional_reference(symbols, workflow.input);
        resolved_workflow.state = resolve_optional_reference(symbols, workflow.state);
        for (const auto& load : workflow.loads)
        {
            resolved_workflow.loads.push_back(
                SemanticWorkflowLoad{
                    resolve_reference(symbols, load.entity),
                    load.key_field,
                    load.binding,
                }
            );
        }
        for (const auto& child_set : workflow.child_sets)
        {
            resolved_workflow.child_sets.push_back(
                SemanticWorkflowChildSet{
                    child_set.name,
                    resolve_optional_reference(symbols, child_set.entity),
                    child_set.parent_field,
                    child_set.id_type,
                    child_set.pending,
                    child_set.creating,
                    child_set.succeeded,
                    child_set.failed,
                    child_set.desired_count,
                }
            );
        }
        for (const auto& step : workflow.steps)
        {
            SemanticWorkflowStep resolved_step;
            resolved_step.name = step.name;
            resolved_step.expected_execution_time = step.expected_execution_time;
            resolved_step.max_retries = step.max_retries;
            for (const auto& statement : step.statements)
            {
                resolved_step.statements.push_back(
                    resolve_workflow_statement(symbols, workflow.name, statement)
                );
            }
            resolved_workflow.steps.push_back(std::move(resolved_step));
        }
        resolved.workflows.push_back(std::move(resolved_workflow));
    }
}

} // namespace statespec
