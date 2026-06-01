#include "validator_workflows.hpp"

#include "validator_declarations.hpp"
#include "validator_helpers.hpp"

#include <string>
#include <unordered_set>

namespace statespec::validator_detail
{

namespace
{

int workflow_member_order_index(const std::string& kind)
{
    if (kind == "version")
    {
        return 0;
    }
    if (kind == "singleton")
    {
        return 1;
    }
    if (kind == "expected_execution_time")
    {
        return 2;
    }
    if (kind == "start")
    {
        return 3;
    }
    if (kind == "on")
    {
        return 4;
    }
    if (kind == "input")
    {
        return 5;
    }
    if (kind == "state")
    {
        return 6;
    }
    if (kind == "load")
    {
        return 7;
    }
    if (kind == "child_set")
    {
        return 8;
    }
    if (kind == "step")
    {
        return 9;
    }
    return 10;
}

void validate_workflow_member_order(
    const WorkflowDecl& workflow,
    DiagnosticBag& diagnostics
)
{
    int previous_order = -1;
    for (const auto& member : workflow.member_order)
    {
        const auto order = workflow_member_order_index(member.kind);
        if (order < previous_order)
        {
            diagnostics.warning(
                member.range, diagnostic_codes::NoncanonicalWorkflowOrder,
                "workflow '" + workflow.name +
                    "' members should use canonical order: version, singleton, "
                    "expected_execution_time, start, on, input, state, load, child_set, step"
            );
            return;
        }
        previous_order = order;
    }
}

std::unordered_set<std::string> entity_field_names(const EntityDecl& entity)
{
    std::unordered_set<std::string> fields;
    for (const auto& field : entity.fields)
    {
        fields.insert(field.name);
    }
    return fields;
}

bool workflow_statement_is_child_set_operation(const WorkflowStatementDecl& statement)
{
    return statement.kind == "reserve_child_set" || statement.kind == "materialize_child_set" ||
           statement.kind == "reconcile_child_set";
}

void validate_workflow_statement(
    const SystemDecl& system,
    const SymbolTable& symbols,
    const std::unordered_set<std::string>& steps,
    const std::unordered_set<std::string>& child_sets,
    const WorkflowStatementDecl& statement,
    DiagnosticBag& diagnostics
)
{
    if (statement.expression.has_value())
    {
        validate_expression(system, statement.range, *statement.expression, diagnostics);
    }
    for (const auto& assignment : statement.payload)
    {
        validate_expression(system, assignment.range, assignment.expression, diagnostics);
    }
    for (const auto& nested : statement.statements)
    {
        validate_workflow_statement(system, symbols, steps, child_sets, nested, diagnostics);
    }

    if (statement.kind == "emit" && statement.target.has_value())
    {
        validate_symbol_reference(
            symbols, statement.range, "workflow emit target", *statement.target,
            {SymbolKind::Event, SymbolKind::Log}, diagnostics
        );
    }
    else if (statement.kind == "enqueue" && statement.target.has_value())
    {
        validate_symbol_reference(
            symbols, statement.range, "workflow enqueue target", *statement.target,
            {SymbolKind::Message}, diagnostics
        );
    }
    else if ((statement.kind == "acquire_lease" || statement.kind == "renew_lease" ||
              statement.kind == "release_lease") &&
             statement.target.has_value())
    {
        validate_symbol_reference(
            symbols, statement.range, "workflow lease target", *statement.target,
            {SymbolKind::Lease}, diagnostics
        );
    }
    else if (statement.kind == "start_workflow" && statement.target.has_value())
    {
        validate_symbol_reference(
            symbols, statement.range, "workflow start target", *statement.target,
            {SymbolKind::Workflow}, diagnostics
        );
    }
    else if ((statement.kind == "create_child" || statement.kind == "observe_child") &&
             statement.target.has_value())
    {
        validate_symbol_reference(
            symbols, statement.range, "workflow child entity target", *statement.target,
            {SymbolKind::Entity}, diagnostics
        );
    }

    if (statement.kind == "transition_to" && statement.target.has_value() &&
        !contains(steps, *statement.target))
    {
        unknown_reference_error(
            diagnostics, statement.range, "workflow transition target", *statement.target
        );
    }
    if (workflow_statement_is_child_set_operation(statement) && statement.target.has_value() &&
        !contains(child_sets, *statement.target))
    {
        unknown_reference_error(
            diagnostics, statement.range, "workflow child_set target", *statement.target
        );
    }

    if (statement.kind == "for_each" && !statement.binding.has_value())
    {
        required_error(diagnostics, statement.range, "workflow for_each statement", "iterator");
    }
    if ((statement.kind == "atomic" || statement.kind == "for_each" || statement.kind == "when") &&
        statement.statements.empty())
    {
        required_error(
            diagnostics, statement.range, "workflow " + statement.kind + " block",
            "at least one statement"
        );
    }
    if (statement.kind == "move")
    {
        if (!statement.expression.has_value() || statement.expression->empty())
        {
            required_error(diagnostics, statement.range, "workflow move statement", "expression");
        }
        if (!statement.from_assignable.has_value() || statement.from_assignable->empty())
        {
            required_error(diagnostics, statement.range, "workflow move statement", "from");
        }
        if (!statement.to_assignable.has_value() || statement.to_assignable->empty())
        {
            required_error(diagnostics, statement.range, "workflow move statement", "to");
        }
    }
}

} // namespace

void validate_workflows(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& workflow : system.workflows)
    {
        validate_workflow_member_order(workflow, diagnostics);

        if (!workflow.version.has_value())
        {
            required_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'", "version"
            );
        }
        else if (!is_positive_integer(*workflow.version))
        {
            positive_integer_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'", "version"
            );
        }
        if (!workflow.start_step.has_value())
        {
            required_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'", "start step"
            );
        }
        if (!workflow.singleton.has_value())
        {
            required_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'", "singleton"
            );
        }
        if (workflow.steps.empty())
        {
            required_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'", "at least one step"
            );
        }
        if (!workflow.expected_execution_time.has_value())
        {
            required_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'",
                "expected_execution_time"
            );
        }
        else if (!is_duration_literal(*workflow.expected_execution_time))
        {
            duration_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'",
                "expected_execution_time"
            );
        }
        if (workflow.on.has_value())
        {
            validate_symbol_reference(
                symbols, workflow.range, "workflow trigger", *workflow.on,
                {SymbolKind::Api, SymbolKind::Event, SymbolKind::Message}, diagnostics
            );
        }
        if (workflow.input.has_value() && !is_known_type_reference(*workflow.input, symbols))
        {
            unknown_reference_error(diagnostics, workflow.range, "workflow input", *workflow.input);
        }
        if (workflow.state.has_value() && !is_known_type_reference(*workflow.state, symbols))
        {
            unknown_reference_error(diagnostics, workflow.range, "workflow state", *workflow.state);
        }

        std::unordered_set<std::string> load_bindings;
        for (const auto& load : workflow.loads)
        {
            if (!load_bindings.insert(load.binding).second)
            {
                duplicate_error(diagnostics, load.range, load.binding);
            }

            const auto* entity = find_entity(system, load.entity);
            if (entity == nullptr)
            {
                unknown_reference_error(
                    diagnostics, load.range, "workflow load entity", load.entity
                );
                continue;
            }

            const auto key_fields = std::unordered_set<std::string>{
                entity->key_fields.begin(), entity->key_fields.end()
            };
            if (!contains(key_fields, load.key_field))
            {
                unknown_reference_error(
                    diagnostics, load.range, "workflow load key field", load.key_field
                );
            }
        }

        std::unordered_set<std::string> child_sets;
        for (const auto& child_set : workflow.child_sets)
        {
            if (!child_sets.insert(child_set.name).second)
            {
                duplicate_error(diagnostics, child_set.range, child_set.name);
            }
            if (!child_set.entity.has_value())
            {
                required_error(
                    diagnostics, child_set.range, "workflow child_set '" + child_set.name + "'",
                    "entity"
                );
                continue;
            }
            const auto* child_entity = find_entity(system, *child_set.entity);
            if (child_entity == nullptr)
            {
                unknown_reference_error(
                    diagnostics, child_set.range, "workflow child_set entity", *child_set.entity
                );
                continue;
            }
            const auto fields = entity_field_names(*child_entity);
            if (child_set.parent_field.has_value() && !contains(fields, *child_set.parent_field))
            {
                unknown_reference_error(
                    diagnostics, child_set.range, "workflow child_set parent field",
                    *child_set.parent_field
                );
            }
            if (child_set.desired_count.has_value())
            {
                validate_expression(system, child_set.range, *child_set.desired_count, diagnostics);
            }
        }

        std::unordered_set<std::string> steps;
        for (const auto& step : workflow.steps)
        {
            if (!steps.insert(step.name).second)
            {
                duplicate_error(diagnostics, step.range, workflow_step_name(workflow, step));
            }
            if (!step.expected_execution_time.has_value())
            {
                required_error(
                    diagnostics, step.range,
                    "workflow step '" + workflow_step_name(workflow, step) + "'",
                    "expected_execution_time"
                );
            }
            else if (!is_duration_literal(*step.expected_execution_time))
            {
                duration_error(
                    diagnostics, step.range,
                    "workflow step '" + workflow_step_name(workflow, step) + "'",
                    "expected_execution_time"
                );
            }
            if (!step.max_retries.has_value())
            {
                required_error(
                    diagnostics, step.range,
                    "workflow step '" + workflow_step_name(workflow, step) + "'", "max_retries"
                );
            }
            else if (!is_non_negative_integer(*step.max_retries))
            {
                non_negative_integer_error(
                    diagnostics, step.range,
                    "workflow step '" + workflow_step_name(workflow, step) + "'", "max_retries"
                );
            }
        }

        if (workflow.start_step.has_value() && !contains(steps, *workflow.start_step))
        {
            unknown_reference_error(
                diagnostics, workflow.range, "workflow start step", *workflow.start_step
            );
        }
        for (const auto& step : workflow.steps)
        {
            for (const auto& statement : step.statements)
            {
                validate_workflow_statement(
                    system, symbols, steps, child_sets, statement, diagnostics
                );
            }
        }
    }
}

} // namespace statespec::validator_detail
