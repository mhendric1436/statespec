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

    for (const auto& entity : system.entities)
    {
        IrEntity ir_entity;
        ir_entity.name = entity.name;
        ir_entity.key_fields = entity.key_fields;
        for (const auto& field : entity.fields)
        {
            ir_entity.fields.push_back(IrField{field.name, field.type});
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
