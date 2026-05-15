#include "statespec/ir.hpp"

#include <utility>

namespace statespec
{

IrSystem lower_to_ir(const Spec& spec)
{
    IrSystem ir;
    if (!spec.system.has_value())
    {
        return ir;
    }

    const auto& system = *spec.system;
    ir.name = system.name;

    for (const auto& flag : system.feature_flags)
    {
        ir.feature_flags.push_back(
            IrFeatureFlag{
                flag.name,
                flag.type.value_or(""),
                flag.default_value.value_or(""),
                flag.scope.value_or("system"),
                flag.owner,
                flag.description,
                flag.expires,
            }
        );
    }

    for (const auto& log : system.logs)
    {
        IrLog ir_log;
        ir_log.name = log.name;
        ir_log.level = log.level.value_or("");
        ir_log.event_name = log.event_name.value_or("");
        for (const auto& field : log.fields)
        {
            ir_log.fields.push_back(IrField{field.name, field.type});
        }
        ir.logs.push_back(std::move(ir_log));
    }

    for (const auto& metric : system.metrics)
    {
        IrMetric ir_metric;
        ir_metric.name = metric.name;
        ir_metric.kind = metric.kind.value_or("");
        ir_metric.backend_name = metric.backend_name.value_or("");
        ir_metric.unit = metric.unit.value_or("");
        for (const auto& label : metric.labels)
        {
            ir_metric.labels.push_back(IrField{label.name, label.type});
        }
        ir.metrics.push_back(std::move(ir_metric));
    }

    for (const auto& entity : system.entities)
    {
        IrEntity ir_entity;
        ir_entity.name = entity.name;
        ir_entity.key_fields = entity.key_fields;
        for (const auto& field : entity.fields)
        {
            ir_entity.fields.push_back(IrField{field.name, field.type});
        }
        if (entity.state_machine.has_value())
        {
            const auto& state_machine = *entity.state_machine;
            ir_entity.initial_state = state_machine.initial_state;
            ir_entity.terminal_states = state_machine.terminal_states;
            for (const auto& state : state_machine.states)
            {
                IrState ir_state;
                ir_state.name = state.name;
                ir_state.terminal = state.terminal;
                if (state.garbage_collection.has_value())
                {
                    ir_state.garbage_collection = IrGarbageCollectionPolicy{
                        state.garbage_collection->after.value_or(""),
                        state.garbage_collection->mode.value_or(""),
                    };
                }
                ir_entity.states.push_back(std::move(ir_state));
            }
        }
        ir.entities.push_back(std::move(ir_entity));
    }

    for (const auto& queue : system.queues)
    {
        ir.queues.push_back(
            IrQueue{
                queue.name,
                queue.channel,
                queue.visibility_timeout,
                queue.max_attempts,
                queue.dead_letter,
            }
        );
    }

    for (const auto& lease : system.leases)
    {
        ir.leases.push_back(
            IrLease{
                lease.name,
                lease.resource,
                lease.ttl,
                lease.renew_every,
                lease.holder,
                lease.fencing_token,
                lease.max_ttl,
            }
        );
    }

    for (const auto& workflow : system.workflows)
    {
        IrWorkflow ir_workflow;
        ir_workflow.name = workflow.name;
        ir_workflow.version = workflow.version;
        ir_workflow.singleton = workflow.singleton;
        ir_workflow.expected_execution_time = workflow.expected_execution_time;
        ir_workflow.start_step = workflow.start_step;
        for (const auto& step : workflow.steps)
        {
            ir_workflow.steps.push_back(
                IrWorkflowStep{step.name, step.expected_execution_time, step.max_retries}
            );
        }
        ir.workflows.push_back(std::move(ir_workflow));
    }

    return ir;
}

} // namespace statespec
