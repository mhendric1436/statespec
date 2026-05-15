#include "statespec/ir.hpp"

#include <utility>

namespace statespec
{

namespace
{

std::vector<IrField> lower_fields(const std::vector<FieldDecl>& fields)
{
    std::vector<IrField> lowered;
    for (const auto& field : fields)
    {
        lowered.push_back(IrField{field.name, field.type});
    }
    return lowered;
}

} // namespace

IrSystem lower_to_ir(const Spec& spec)
{
    IrSystem ir;
    if (!spec.system.has_value())
    {
        return ir;
    }

    const auto& system = *spec.system;
    ir.name = system.name;

    if (system.tenant_scope.has_value())
    {
        ir.tenant_scope = IrTenantScope{system.tenant_scope->field_name};
    }

    if (system.system_tenant.has_value())
    {
        ir.system_tenant = IrSystemTenant{"configured", system.system_tenant->config_key};
    }

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
        ir_log.fields = lower_fields(log.fields);
        ir.logs.push_back(std::move(ir_log));
    }

    for (const auto& metric : system.metrics)
    {
        IrMetric ir_metric;
        ir_metric.name = metric.name;
        ir_metric.kind = metric.kind.value_or("");
        ir_metric.backend_name = metric.backend_name.value_or("");
        ir_metric.unit = metric.unit.value_or("");
        ir_metric.labels = lower_fields(metric.labels);
        ir.metrics.push_back(std::move(ir_metric));
    }

    for (const auto& entity : system.entities)
    {
        IrEntity ir_entity;
        ir_entity.name = entity.name;
        ir_entity.key_fields = entity.key_fields;
        ir_entity.fields = lower_fields(entity.fields);
        for (const auto& index : entity.indexes)
        {
            ir_entity.indexes.push_back(IrIndex{index.name, index.fields, index.unique});
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
            for (const auto& transition : state_machine.transitions)
            {
                ir_entity.transitions.push_back(IrTransition{transition.from, transition.to});
            }
        }
        ir.entities.push_back(std::move(ir_entity));
    }

    for (const auto& queue : system.queues)
    {
        IrQueue ir_queue;
        ir_queue.name = queue.name;
        ir_queue.namespace_name = queue.namespace_name;
        ir_queue.channel = queue.channel;
        ir_queue.visibility_timeout = queue.visibility_timeout;
        ir_queue.max_attempts = queue.max_attempts;
        ir_queue.dead_letter = queue.dead_letter;
        for (const auto& message : queue.messages)
        {
            IrMessage ir_message;
            ir_message.name = message.name;
            ir_message.idempotency_key = message.idempotency_key;
            ir_message.payload_fields = lower_fields(message.payload_fields);
            ir_queue.messages.push_back(std::move(ir_message));
        }
        ir.queues.push_back(std::move(ir_queue));
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

    for (const auto& worker : system.workers)
    {
        ir.workers.push_back(
            IrWorker{
                worker.name,
                worker.singleton,
                worker.lease,
                worker.polls,
                worker.executes,
                worker.concurrency,
            }
        );
    }

    for (const auto& api : system.apis)
    {
        ir.apis.push_back(
            IrApi{
                api.name,
                api.method,
                api.path,
                api.input,
                api.output,
                api.error,
                api.starts_workflow,
                api.enqueues,
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

    for (const auto& policy : system.policies)
    {
        IrPolicy ir_policy;
        ir_policy.name = policy.name;
        ir_policy.tenant_scoped_by = policy.tenant_scoped_by;
        for (const auto& allow : policy.allows)
        {
            ir_policy.allows.push_back(IrPolicyRule{allow.action, allow.condition});
        }
        for (const auto& deny : policy.denies)
        {
            ir_policy.denies.push_back(IrPolicyRule{deny.action, deny.condition});
        }
        for (const auto& quota : policy.quotas)
        {
            ir_policy.quotas.push_back(IrQuota{quota.name, quota.expression});
        }
        ir_policy.audits = policy.audits;
        ir.policies.push_back(std::move(ir_policy));
    }

    return ir;
}

} // namespace statespec
