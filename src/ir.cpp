#include "statespec/ir.hpp"

#include <utility>

namespace statespec
{

namespace
{

std::vector<IrField> lower_fields(const std::vector<SemanticField>& fields)
{
    std::vector<IrField> lowered;
    for (const auto& field : fields)
    {
        lowered.push_back(IrField{field.name, field.type});
    }
    return lowered;
}

std::optional<std::string> reference_name(const std::optional<SemanticReference>& reference)
{
    if (!reference.has_value())
    {
        return std::nullopt;
    }
    return reference->name;
}

std::vector<std::string> reference_names(const std::vector<SemanticReference>& references)
{
    std::vector<std::string> names;
    for (const auto& reference : references)
    {
        names.push_back(reference.name);
    }
    return names;
}

IrWorkflowStatement lower_workflow_statement(const SemanticWorkflowStatement& statement)
{
    IrWorkflowStatement lowered;
    lowered.kind = statement.kind;
    lowered.target = statement.target;
    lowered.expression = statement.expression;
    lowered.assignable = statement.assignable;
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
    return lowered;
}

} // namespace

IrSystem lower_to_ir(const SemanticSystem& system)
{
    IrSystem ir;
    ir.name = system.name;

    if (system.tenant_scope.has_value())
    {
        ir.tenant_scope = IrTenantScope{system.tenant_scope->field_name};
    }

    if (system.system_tenant.has_value())
    {
        ir.system_tenant =
            IrSystemTenant{system.system_tenant->source, system.system_tenant->config_key};
    }

    for (const auto& namespace_decl : system.namespaces)
    {
        IrNamespace ir_namespace;
        ir_namespace.name = namespace_decl.name;
        for (const auto& member : namespace_decl.members)
        {
            ir_namespace.members.push_back(member.name);
        }
        ir.namespaces.push_back(std::move(ir_namespace));
    }

    for (const auto& value : system.values)
    {
        ir.values.push_back(IrValue{value.name, value.type, value.constraint});
    }

    for (const auto& enum_decl : system.enums)
    {
        IrEnum ir_enum;
        ir_enum.name = enum_decl.name;
        for (const auto& member : enum_decl.members)
        {
            ir_enum.members.push_back(IrEnumMember{member.name, member.value});
        }
        ir.enums.push_back(std::move(ir_enum));
    }

    for (const auto& event : system.events)
    {
        ir.events.push_back(IrEvent{event.name, lower_fields(event.fields)});
    }

    for (const auto& external_system : system.external_systems)
    {
        IrExternalSystem ir_external_system;
        ir_external_system.name = external_system.name;
        for (const auto& property : external_system.properties)
        {
            ir_external_system.properties.push_back(
                IrExternalSystemProperty{property.name, property.value}
            );
        }
        if (external_system.metadata.has_value())
        {
            ir_external_system.metadata = IrExternalSystemMetadata{
                external_system.metadata->entity,          external_system.metadata->tenant_field,
                external_system.metadata->profile_field,   external_system.metadata->key_fields,
                external_system.metadata->required_fields,
            };
        }
        ir.external_systems.push_back(std::move(ir_external_system));
    }

    for (const auto& shape : system.shapes)
    {
        ir.shapes.push_back(IrShape{shape.name, lower_fields(shape.fields)});
    }

    for (const auto& flag : system.feature_flags)
    {
        ir.feature_flags.push_back(
            IrFeatureFlag{
                flag.name,
                flag.type,
                flag.default_value,
                flag.scope,
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
        ir_log.level = log.level;
        ir_log.event_name = log.event_name;
        ir_log.fields = lower_fields(log.fields);
        ir.logs.push_back(std::move(ir_log));
    }

    for (const auto& metric : system.metrics)
    {
        IrMetric ir_metric;
        ir_metric.name = metric.name;
        ir_metric.kind = metric.kind;
        ir_metric.backend_name = metric.backend_name;
        ir_metric.unit = metric.unit;
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
        if (entity.ownership.has_value())
        {
            ir_entity.ownership = IrOwnership{
                entity.ownership->authority,
                entity.ownership->system_of_record,
                entity.ownership->lifecycle,
            };
        }
        for (const auto& relation : entity.relations)
        {
            ir_entity.relations.push_back(
                IrRelation{
                    relation.kind,
                    relation.name,
                    relation.target.name,
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
            ir_entity.children.push_back(
                IrChild{child.name, child.target_entity.name, child.relation}
            );
        }
        for (const auto& invariant : entity.invariants)
        {
            ir_entity.invariants.push_back(IrInvariant{invariant.name, invariant.expression});
        }
        ir_entity.initial_state = entity.initial_state;
        ir_entity.terminal_states = entity.terminal_states;
        for (const auto& state : entity.states)
        {
            IrState ir_state;
            ir_state.name = state.name;
            ir_state.terminal = state.terminal;
            if (state.garbage_collection.has_value())
            {
                ir_state.garbage_collection = IrGarbageCollectionPolicy{
                    state.garbage_collection->after,
                    state.garbage_collection->mode,
                };
            }
            ir_entity.states.push_back(std::move(ir_state));
        }
        for (const auto& transition : entity.transitions)
        {
            ir_entity.transitions.push_back(IrTransition{transition.from, transition.to});
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
        ir_queue.dead_letter = reference_name(queue.dead_letter);
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
                reference_name(worker.lease),
                reference_name(worker.polls),
                reference_name(worker.executes),
                worker.concurrency,
            }
        );
    }

    for (const auto& api_server : system.api_servers)
    {
        ir.api_servers.push_back(
            IrApiServer{
                api_server.name,
                reference_names(api_server.serves),
                api_server.concurrency,
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
                reference_name(api.input),
                reference_name(api.output),
                reference_name(api.error),
                reference_name(api.starts_workflow),
                reference_name(api.enqueues),
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

    for (const auto& policy : system.policies)
    {
        IrPolicy ir_policy;
        ir_policy.name = policy.name;
        ir_policy.tenant_scoped_by = policy.tenant_scoped_by;
        for (const auto& allow : policy.allows)
        {
            ir_policy.allows.push_back(IrPolicyRule{allow.action.name, allow.condition});
        }
        for (const auto& deny : policy.denies)
        {
            ir_policy.denies.push_back(IrPolicyRule{deny.action.name, deny.condition});
        }
        for (const auto& quota : policy.quotas)
        {
            ir_policy.quotas.push_back(IrQuota{quota.name, quota.expression});
        }
        for (const auto& audit : policy.audits)
        {
            ir_policy.audits.push_back(audit.name);
        }
        ir.policies.push_back(std::move(ir_policy));
    }

    return ir;
}

IrSystem lower_to_ir(const Spec& spec)
{
    return lower_to_ir(resolve_semantics(spec));
}

} // namespace statespec
