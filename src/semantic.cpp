#include "statespec/semantic.hpp"

#include <utility>

namespace statespec
{

namespace
{

std::vector<SemanticField> resolve_fields(const std::vector<FieldDecl>& fields)
{
    std::vector<SemanticField> resolved;
    for (const auto& field : fields)
    {
        resolved.push_back(SemanticField{field.name, field.type});
    }
    return resolved;
}

void insert_symbol(
    SymbolTable& symbols,
    SymbolKind kind,
    const std::string& name,
    const SourceRange& range
)
{
    symbols.insert(Symbol{kind, name, range});
}

SymbolTable build_symbols(const SystemDecl& system)
{
    SymbolTable symbols;
    insert_symbol(symbols, SymbolKind::System, system.name, system.range);

    for (const auto& flag : system.feature_flags)
    {
        insert_symbol(symbols, SymbolKind::FeatureFlag, flag.name, flag.range);
    }
    for (const auto& log : system.logs)
    {
        insert_symbol(symbols, SymbolKind::Log, log.name, log.range);
    }
    for (const auto& metric : system.metrics)
    {
        insert_symbol(symbols, SymbolKind::Metric, metric.name, metric.range);
    }
    for (const auto& entity : system.entities)
    {
        insert_symbol(symbols, SymbolKind::Entity, entity.name, entity.range);
    }
    for (const auto& queue : system.queues)
    {
        insert_symbol(symbols, SymbolKind::Queue, queue.name, queue.range);
        for (const auto& message : queue.messages)
        {
            insert_symbol(
                symbols, SymbolKind::Message, queue.name + "." + message.name, message.range
            );
        }
    }
    for (const auto& lease : system.leases)
    {
        insert_symbol(symbols, SymbolKind::Lease, lease.name, lease.range);
    }
    for (const auto& worker : system.workers)
    {
        insert_symbol(symbols, SymbolKind::Worker, worker.name, worker.range);
    }
    for (const auto& api : system.apis)
    {
        insert_symbol(symbols, SymbolKind::Api, api.name, api.range);
    }
    for (const auto& workflow : system.workflows)
    {
        insert_symbol(symbols, SymbolKind::Workflow, workflow.name, workflow.range);
        for (const auto& step : workflow.steps)
        {
            insert_symbol(
                symbols, SymbolKind::WorkflowStep, workflow.name + "." + step.name, step.range
            );
        }
    }
    for (const auto& policy : system.policies)
    {
        insert_symbol(symbols, SymbolKind::Policy, policy.name, policy.range);
    }

    return symbols;
}

SemanticReference resolve_reference(
    const SymbolTable& symbols,
    const std::string& name
)
{
    SemanticReference reference;
    reference.name = name;
    if (const auto symbol = symbols.find(name); symbol.has_value())
    {
        reference.kind = symbol->kind;
    }
    return reference;
}

std::optional<SemanticReference> resolve_optional_reference(
    const SymbolTable& symbols,
    const std::optional<std::string>& name
)
{
    if (!name.has_value())
    {
        return std::nullopt;
    }
    return resolve_reference(symbols, *name);
}

std::optional<SemanticReference> resolve_feature_flag_scope_target(
    const SymbolTable& symbols,
    const std::optional<std::string>& scope
)
{
    if (!scope.has_value())
    {
        return std::nullopt;
    }

    constexpr std::string_view prefix{"entity "};
    if (scope->rfind(std::string{prefix}, 0) != 0)
    {
        return std::nullopt;
    }

    return resolve_reference(symbols, scope->substr(prefix.size()));
}

SemanticEntity resolve_entity(const EntityDecl& entity)
{
    SemanticEntity resolved;
    resolved.name = entity.name;
    resolved.key_fields = entity.key_fields;
    resolved.fields = resolve_fields(entity.fields);
    for (const auto& index : entity.indexes)
    {
        resolved.indexes.push_back(SemanticIndex{index.name, index.fields, index.unique});
    }

    if (entity.state_machine.has_value())
    {
        const auto& state_machine = *entity.state_machine;
        resolved.initial_state = state_machine.initial_state;
        resolved.terminal_states = state_machine.terminal_states;
        for (const auto& state : state_machine.states)
        {
            SemanticState resolved_state;
            resolved_state.name = state.name;
            resolved_state.terminal = state.terminal;
            if (state.garbage_collection.has_value())
            {
                resolved_state.garbage_collection = SemanticGarbageCollectionPolicy{
                    state.garbage_collection->after.value_or(""),
                    state.garbage_collection->mode.value_or(""),
                };
            }
            resolved.states.push_back(std::move(resolved_state));
        }
        for (const auto& transition : state_machine.transitions)
        {
            resolved.transitions.push_back(SemanticTransition{transition.from, transition.to});
        }
    }

    return resolved;
}

} // namespace

SemanticSystem resolve_semantics(const Spec& spec)
{
    SemanticSystem resolved;
    if (!spec.system.has_value())
    {
        return resolved;
    }

    const auto& system = *spec.system;
    const auto symbols = build_symbols(system);

    resolved.name = system.name;
    if (system.tenant_scope.has_value())
    {
        resolved.tenant_scope = SemanticTenantScope{system.tenant_scope->field_name};
    }
    if (system.system_tenant.has_value())
    {
        resolved.system_tenant =
            SemanticSystemTenant{"configured", system.system_tenant->config_key};
    }

    for (const auto& flag : system.feature_flags)
    {
        resolved.feature_flags.push_back(
            SemanticFeatureFlag{
                flag.name,
                flag.type.value_or(""),
                flag.default_value.value_or(""),
                flag.scope.value_or("system"),
                resolve_feature_flag_scope_target(symbols, flag.scope),
                flag.owner,
                flag.description,
                flag.expires,
            }
        );
    }

    for (const auto& log : system.logs)
    {
        resolved.logs.push_back(
            SemanticLog{
                log.name,
                log.level.value_or(""),
                log.event_name.value_or(""),
                resolve_fields(log.fields),
            }
        );
    }

    for (const auto& metric : system.metrics)
    {
        resolved.metrics.push_back(
            SemanticMetric{
                metric.name,
                metric.kind.value_or(""),
                metric.backend_name.value_or(""),
                metric.unit.value_or(""),
                resolve_fields(metric.labels),
            }
        );
    }

    for (const auto& entity : system.entities)
    {
        resolved.entities.push_back(resolve_entity(entity));
    }

    for (const auto& queue : system.queues)
    {
        SemanticQueue resolved_queue;
        resolved_queue.name = queue.name;
        resolved_queue.namespace_name = queue.namespace_name;
        resolved_queue.channel = queue.channel;
        resolved_queue.visibility_timeout = queue.visibility_timeout;
        resolved_queue.max_attempts = queue.max_attempts;
        resolved_queue.dead_letter = resolve_optional_reference(symbols, queue.dead_letter);
        for (const auto& message : queue.messages)
        {
            resolved_queue.messages.push_back(
                SemanticMessage{
                    message.name,
                    message.idempotency_key,
                    resolve_fields(message.payload_fields),
                }
            );
        }
        resolved.queues.push_back(std::move(resolved_queue));
    }

    for (const auto& lease : system.leases)
    {
        resolved.leases.push_back(
            SemanticLease{
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
        resolved.workers.push_back(
            SemanticWorker{
                worker.name,
                worker.singleton,
                resolve_optional_reference(symbols, worker.lease),
                resolve_optional_reference(symbols, worker.polls),
                resolve_optional_reference(symbols, worker.executes),
                worker.concurrency,
            }
        );
    }

    for (const auto& api : system.apis)
    {
        resolved.apis.push_back(
            SemanticApi{
                api.name,
                api.method,
                api.path,
                resolve_optional_reference(symbols, api.input),
                resolve_optional_reference(symbols, api.output),
                resolve_optional_reference(symbols, api.error),
                resolve_optional_reference(symbols, api.starts_workflow),
                resolve_optional_reference(symbols, api.enqueues),
            }
        );
    }

    for (const auto& workflow : system.workflows)
    {
        SemanticWorkflow resolved_workflow;
        resolved_workflow.name = workflow.name;
        resolved_workflow.version = workflow.version;
        resolved_workflow.singleton = workflow.singleton;
        resolved_workflow.expected_execution_time = workflow.expected_execution_time;
        resolved_workflow.start_step = workflow.start_step;
        for (const auto& step : workflow.steps)
        {
            resolved_workflow.steps.push_back(
                SemanticWorkflowStep{step.name, step.expected_execution_time, step.max_retries}
            );
        }
        resolved.workflows.push_back(std::move(resolved_workflow));
    }

    for (const auto& policy : system.policies)
    {
        SemanticPolicy resolved_policy;
        resolved_policy.name = policy.name;
        resolved_policy.tenant_scoped_by = policy.tenant_scoped_by;
        for (const auto& allow : policy.allows)
        {
            resolved_policy.allows.push_back(
                SemanticPolicyRule{resolve_reference(symbols, allow.action), allow.condition}
            );
        }
        for (const auto& deny : policy.denies)
        {
            resolved_policy.denies.push_back(
                SemanticPolicyRule{resolve_reference(symbols, deny.action), deny.condition}
            );
        }
        for (const auto& quota : policy.quotas)
        {
            resolved_policy.quotas.push_back(SemanticQuota{quota.name, quota.expression});
        }
        for (const auto& audit : policy.audits)
        {
            resolved_policy.audits.push_back(resolve_reference(symbols, audit));
        }
        resolved.policies.push_back(std::move(resolved_policy));
    }

    return resolved;
}

} // namespace statespec
