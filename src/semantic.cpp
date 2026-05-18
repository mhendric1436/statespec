#include "statespec/semantic.hpp"

#include <string_view>
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
    for (const auto& value : system.values)
    {
        insert_symbol(symbols, SymbolKind::Value, value.name, value.range);
    }
    for (const auto& enum_decl : system.enums)
    {
        insert_symbol(symbols, SymbolKind::Enum, enum_decl.name, enum_decl.range);
    }
    for (const auto& namespace_decl : system.namespaces)
    {
        insert_symbol(symbols, SymbolKind::Namespace, namespace_decl.name, namespace_decl.range);
    }
    for (const auto& event : system.events)
    {
        insert_symbol(symbols, SymbolKind::Event, event.name, event.range);
    }
    for (const auto& shape : system.shapes)
    {
        insert_symbol(symbols, SymbolKind::Shape, shape.name, shape.range);
    }
    for (const auto& external_system : system.external_systems)
    {
        insert_symbol(
            symbols, SymbolKind::ExternalSystem, external_system.name, external_system.range
        );
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
    for (const auto& api_server : system.api_servers)
    {
        insert_symbol(symbols, SymbolKind::ApiServer, api_server.name, api_server.range);
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

std::string relation_target_name(std::string target)
{
    if (!target.empty() && target.back() == '?')
    {
        target.pop_back();
    }
    constexpr std::string_view optional_prefix{"optional "};
    if (target.rfind(std::string{optional_prefix}, 0) == 0)
    {
        target = target.substr(optional_prefix.size());
    }
    constexpr std::string_view ref_prefix{"ref<"};
    if (target.rfind(std::string{ref_prefix}, 0) == 0 && target.back() == '>')
    {
        return target.substr(ref_prefix.size(), target.size() - ref_prefix.size() - 1);
    }
    return target;
}

SemanticEntity resolve_entity(
    const SymbolTable& symbols,
    const EntityDecl& entity
)
{
    SemanticEntity resolved;
    resolved.name = entity.name;
    resolved.key_fields = entity.key_fields;
    resolved.fields = resolve_fields(entity.fields);
    for (const auto& index : entity.indexes)
    {
        resolved.indexes.push_back(SemanticIndex{index.name, index.fields, index.unique});
    }
    if (entity.ownership.has_value())
    {
        resolved.ownership = SemanticOwnership{
            entity.ownership->authority.value_or(""),
            entity.ownership->system_of_record.value_or(""),
            entity.ownership->lifecycle.value_or(""),
        };
    }
    for (const auto& relation : entity.relations)
    {
        resolved.relations.push_back(
            SemanticRelation{
                relation.kind,
                relation.name,
                resolve_reference(symbols, relation_target_name(relation.target)),
                relation.optional,
                relation.relation_kind,
                relation.on_parent_delete,
                relation.parent_must_be_in,
                relation.unique_within_parent,
            }
        );
    }
    for (const auto& child : entity.children)
    {
        resolved.children.push_back(
            SemanticChild{
                child.name,
                resolve_reference(symbols, child.target_entity),
                child.relation,
            }
        );
    }
    for (const auto& invariant : entity.invariants)
    {
        resolved.invariants.push_back(SemanticInvariant{invariant.name, invariant.expression});
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

    for (const auto& shape : system.shapes)
    {
        resolved.shapes.push_back(SemanticShape{shape.name, resolve_fields(shape.fields)});
    }

    for (const auto& value : system.values)
    {
        resolved.values.push_back(SemanticValue{value.name, value.type, value.constraint});
    }

    for (const auto& namespace_decl : system.namespaces)
    {
        SemanticNamespace resolved_namespace;
        resolved_namespace.name = namespace_decl.name;
        for (const auto& member : namespace_decl.members)
        {
            resolved_namespace.members.push_back(resolve_reference(symbols, member));
        }
        resolved.namespaces.push_back(std::move(resolved_namespace));
    }

    for (const auto& enum_decl : system.enums)
    {
        SemanticEnum resolved_enum;
        resolved_enum.name = enum_decl.name;
        for (const auto& member : enum_decl.members)
        {
            resolved_enum.members.push_back(SemanticEnumMember{member.name, member.value});
        }
        resolved.enums.push_back(std::move(resolved_enum));
    }

    for (const auto& event : system.events)
    {
        resolved.events.push_back(SemanticEvent{event.name, resolve_fields(event.fields)});
    }

    for (const auto& external_system : system.external_systems)
    {
        SemanticExternalSystem resolved_external_system;
        resolved_external_system.name = external_system.name;
        for (const auto& property : external_system.properties)
        {
            resolved_external_system.properties.push_back(
                SemanticExternalSystemProperty{property.name, property.value}
            );
        }
        resolved.external_systems.push_back(std::move(resolved_external_system));
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
        resolved.entities.push_back(resolve_entity(symbols, entity));
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

    for (const auto& api_server : system.api_servers)
    {
        SemanticApiServer resolved_api_server;
        resolved_api_server.name = api_server.name;
        resolved_api_server.concurrency = api_server.concurrency;
        for (const auto& served_api : api_server.serves)
        {
            resolved_api_server.serves.push_back(resolve_reference(symbols, served_api));
        }
        resolved.api_servers.push_back(std::move(resolved_api_server));
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
