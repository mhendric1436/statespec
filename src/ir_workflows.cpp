#include "ir_workflows.hpp"

#include "ir_lowering_helpers.hpp"

#include <utility>

namespace statespec
{

namespace
{

IrWorkflowStatement lower_workflow_statement(const SemanticWorkflowStatement& statement)
{
    IrWorkflowStatement lowered;
    lowered.kind = statement.kind;
    lowered.target = statement.target;
    lowered.expression = statement.expression;
    lowered.assignable = statement.assignable;
    lowered.from_assignable = statement.from_assignable;
    lowered.to_assignable = statement.to_assignable;
    lowered.binding = statement.binding;
    for (const auto& assignment : statement.payload)
    {
        lowered.payload.push_back(
            IrWorkflowAssignment{
                assignment.name,
                assignment.expression,
            }
        );
    }
    for (const auto& nested : statement.statements)
    {
        lowered.statements.push_back(lower_workflow_statement(nested));
    }
    return lowered;
}

} // namespace

void lower_ir_workflows(
    const SemanticSystem& system,
    IrSystem& ir
)
{
    for (const auto& workflow : system.workflows)
    {
        IrWorkflow ir_workflow;
        ir_workflow.name = workflow.name;
        ir_workflow.version = workflow.version;
        ir_workflow.singleton = workflow.singleton;
        ir_workflow.expected_execution_time = workflow.expected_execution_time;
        ir_workflow.start_step = workflow.start_step;
        ir_workflow.on = reference_name(workflow.on);
        ir_workflow.input = reference_name(workflow.input);
        ir_workflow.state = reference_name(workflow.state);
        for (const auto& load : workflow.loads)
        {
            ir_workflow.loads.push_back(
                IrWorkflowLoad{
                    load.entity.name,
                    load.key_field,
                    load.binding,
                }
            );
        }
        for (const auto& child_set : workflow.child_sets)
        {
            ir_workflow.child_sets.push_back(
                IrWorkflowChildSet{
                    child_set.name,
                    reference_name(child_set.entity),
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
        for (const auto& child_workflow : workflow.child_workflows)
        {
            IrWorkflowChildWorkflow ir_child_workflow;
            ir_child_workflow.name = child_workflow.name;
            ir_child_workflow.child_entity = reference_name(child_workflow.child_entity);
            ir_child_workflow.child_workflow = reference_name(child_workflow.child_workflow);
            ir_child_workflow.child_id_field = child_workflow.child_id_field;
            ir_child_workflow.child_id_type = child_workflow.child_id_type;
            ir_child_workflow.parent_ref_field = child_workflow.parent_ref_field;
            ir_child_workflow.parent_ref_expression = child_workflow.parent_ref_expression;
            ir_child_workflow.desired_count = child_workflow.desired_count;
            for (const auto& assignment : child_workflow.create_assignments)
            {
                ir_child_workflow.create_assignments.push_back(
                    IrWorkflowAssignment{
                        assignment.name,
                        assignment.expression,
                    }
                );
            }
            ir_child_workflow.success_expression = child_workflow.success_expression;
            ir_child_workflow.failure_expression = child_workflow.failure_expression;
            if (child_workflow.derived_names.has_value())
            {
                ir_child_workflow.id_bucket_base = child_workflow.derived_names->id_bucket_base;
                ir_child_workflow.pending_bucket = child_workflow.derived_names->pending_bucket;
                ir_child_workflow.creating_bucket = child_workflow.derived_names->creating_bucket;
                ir_child_workflow.succeeded_bucket = child_workflow.derived_names->succeeded_bucket;
                ir_child_workflow.failed_bucket = child_workflow.derived_names->failed_bucket;
                ir_child_workflow.generate_ids_step =
                    child_workflow.derived_names->generate_ids_step;
                ir_child_workflow.create_children_step =
                    child_workflow.derived_names->create_children_step;
                ir_child_workflow.wait_children_step =
                    child_workflow.derived_names->wait_children_step;
            }
            ir_workflow.child_workflows.push_back(std::move(ir_child_workflow));
        }
        for (const auto& step : workflow.steps)
        {
            IrWorkflowStep ir_step;
            ir_step.name = step.name;
            ir_step.expected_execution_time = step.expected_execution_time;
            ir_step.max_retries = step.max_retries;
            for (const auto& statement : step.statements)
            {
                ir_step.statements.push_back(lower_workflow_statement(statement));
            }
            ir_workflow.steps.push_back(std::move(ir_step));
        }
        ir.workflows.push_back(std::move(ir_workflow));
    }
}

} // namespace statespec
