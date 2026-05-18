#include "statespec/validator.hpp"

#include "validator_context.hpp"
#include "validator_declarations.hpp"
#include "validator_helpers.hpp"

#include "statespec/symbol_table.hpp"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

namespace statespec
{

using namespace validator_detail;

namespace
{

void validate_state_machine(
    const EntityDecl& entity,
    DiagnosticBag& diagnostics
)
{
    if (!entity.state_machine.has_value())
    {
        required_error(diagnostics, entity.range, "entity '" + entity.name + "'", "state_machine");
        return;
    }

    const auto& state_machine = *entity.state_machine;
    std::unordered_set<std::string> states;
    std::unordered_set<std::string> terminal_states;
    std::unordered_set<std::string> garbage_collected_states;
    for (const auto& state : state_machine.states)
    {
        if (!states.insert(state.name).second)
        {
            duplicate_error(diagnostics, state.range, state.name);
        }
        if (state.terminal)
        {
            terminal_states.insert(state.name);
        }
    }

    if (state_machine.states.empty())
    {
        required_error(
            diagnostics, state_machine.range, "state_machine for entity '" + entity.name + "'",
            "at least one state"
        );
    }

    if (!state_machine.initial_state.has_value())
    {
        required_error(
            diagnostics, state_machine.range, "state_machine for entity '" + entity.name + "'",
            "an initial state"
        );
    }
    else if (!contains(states, *state_machine.initial_state))
    {
        unknown_reference_error(
            diagnostics, state_machine.range, "initial state", *state_machine.initial_state
        );
    }

    for (const auto& terminal : state_machine.terminal_states)
    {
        if (!contains(states, terminal))
        {
            unknown_reference_error(diagnostics, state_machine.range, "terminal state", terminal);
        }
        terminal_states.insert(terminal);
    }

    for (const auto& state : state_machine.states)
    {
        if (!state.garbage_collection.has_value())
        {
            continue;
        }

        garbage_collected_states.insert(state.name);

        if (!contains(terminal_states, state.name))
        {
            diagnostics.error(
                state.garbage_collection->range, "SSPEC3103",
                "garbage_collection for state '" + state.name + "' is valid only on terminal states"
            );
        }

        if (!state.garbage_collection->after.has_value())
        {
            required_error(
                diagnostics, state.garbage_collection->range,
                "garbage_collection for state '" + state.name + "'", "after"
            );
        }
        else if (!is_duration_literal(*state.garbage_collection->after))
        {
            duration_error(
                diagnostics, state.garbage_collection->range,
                "garbage_collection for state '" + state.name + "'", "after"
            );
        }

        if (!state.garbage_collection->mode.has_value())
        {
            required_error(
                diagnostics, state.garbage_collection->range,
                "garbage_collection for state '" + state.name + "'", "mode"
            );
        }
        else if (!is_garbage_collection_mode(*state.garbage_collection->mode))
        {
            diagnostics.error(
                state.garbage_collection->range, "SSPEC3104",
                "garbage_collection for state '" + state.name +
                    "' mode must be delete, tombstone, or archive"
            );
        }
    }

    for (const auto& state : state_machine.states)
    {
        if (state.duplicate_garbage_collection_range.has_value())
        {
            diagnostics.error(
                *state.duplicate_garbage_collection_range, "SSPEC3001",
                "duplicate declaration 'garbage_collection'"
            );
        }
    }

    for (const auto& transition : state_machine.transitions)
    {
        if (!contains(states, transition.from))
        {
            unknown_reference_error(
                diagnostics, transition.range, "transition source state", transition.from
            );
        }
        if (!contains(states, transition.to))
        {
            unknown_reference_error(
                diagnostics, transition.range, "transition target state", transition.to
            );
        }
        if (contains(garbage_collected_states, transition.from))
        {
            diagnostics.error(
                transition.range, "SSPEC3105",
                "garbage-collected terminal state '" + transition.from +
                    "' must not have outgoing transitions"
            );
        }
    }
}

const FieldDecl* find_field(
    const std::vector<FieldDecl>& fields,
    const std::string& name
)
{
    for (const auto& field : fields)
    {
        if (field.name == name)
        {
            return &field;
        }
    }
    return nullptr;
}

void validate_required_entity_field(
    const EntityDecl& entity,
    const std::string& field_name,
    const std::string& expected_type,
    DiagnosticBag& diagnostics
)
{
    const auto* field = find_field(entity.fields, field_name);
    if (field == nullptr)
    {
        required_error(
            diagnostics, entity.range, "entity '" + entity.name + "'", "field '" + field_name + "'"
        );
        return;
    }

    if (field->type != expected_type)
    {
        diagnostics.error(
            field->range, "SSPEC3102",
            "entity '" + entity.name + "' field '" + field_name + "' must have type " +
                expected_type
        );
    }
}

void validate_entity_management_fields(
    const EntityDecl& entity,
    DiagnosticBag& diagnostics
)
{
    validate_required_entity_field(entity, "created_at", "timestamp", diagnostics);
    validate_required_entity_field(entity, "updated_at", "timestamp", diagnostics);
    validate_required_entity_field(entity, "status", "string", diagnostics);

    static const std::vector<std::string> canonical_fields{
        "created_at",
        "updated_at",
        "status",
    };
    if (entity.fields.size() < canonical_fields.size())
    {
        return;
    }

    for (std::size_t i = 0; i < canonical_fields.size(); ++i)
    {
        if (entity.fields[i].name != canonical_fields[i])
        {
            diagnostics.error(
                entity.fields[i].range, "SSPEC3106",
                "entity '" + entity.name +
                    "' fields must begin with canonical management fields: created_at "
                    "timestamp, updated_at timestamp, status string"
            );
            return;
        }
    }
}

void validate_indexes(
    const EntityDecl& entity,
    const std::unordered_set<std::string>& fields,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> index_names;
    for (const auto& index : entity.indexes)
    {
        if (!index_names.insert(index.name).second)
        {
            duplicate_error(diagnostics, index.range, index.name);
        }

        if (index.fields.empty())
        {
            required_error(
                diagnostics, index.range, "index '" + index.name + "'", "at least one field"
            );
        }

        std::unordered_set<std::string> index_fields;
        for (const auto& field : index.fields)
        {
            if (!contains(fields, field))
            {
                unknown_reference_error(diagnostics, index.range, "index field", field);
            }
            if (!index_fields.insert(field).second)
            {
                duplicate_error(diagnostics, index.range, field);
            }
        }
    }
}

void validate_ownership(
    const EntityDecl& entity,
    DiagnosticBag& diagnostics
)
{
    if (!entity.ownership.has_value())
    {
        return;
    }
    const auto& ownership = *entity.ownership;
    if (!ownership.authority.has_value())
    {
        required_error(
            diagnostics, ownership.range, "ownership for entity '" + entity.name + "'", "authority"
        );
    }
    else if (!is_ownership_authority(*ownership.authority))
    {
        diagnostics.error(
            ownership.range, "SSPEC3301", "ownership authority must be system or external"
        );
    }
    if (!ownership.system_of_record.has_value())
    {
        required_error(
            diagnostics, ownership.range, "ownership for entity '" + entity.name + "'",
            "system_of_record"
        );
    }
    if (!ownership.lifecycle.has_value())
    {
        required_error(
            diagnostics, ownership.range, "ownership for entity '" + entity.name + "'", "lifecycle"
        );
    }
    else if (!is_lifecycle_mode(*ownership.lifecycle))
    {
        diagnostics.error(
            ownership.range, "SSPEC3302",
            "ownership lifecycle must be authoritative, managed, observed, or projected"
        );
    }
}

void validate_relations(
    const SystemDecl& system,
    const EntityDecl& entity,
    const std::unordered_set<std::string>& fields,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> relation_names;
    for (const auto& relation : entity.relations)
    {
        if (!relation_names.insert(relation.name).second)
        {
            duplicate_error(diagnostics, relation.range, relation.name);
        }
        if (!contains(fields, relation.name))
        {
            unknown_reference_error(diagnostics, relation.range, "relation field", relation.name);
        }

        const auto target_name = relation_target_name(relation.target);
        const auto* target_entity = find_entity(system, target_name);
        if (target_entity == nullptr)
        {
            unknown_reference_error(
                diagnostics, relation.range, "relation target entity", target_name
            );
        }

        if (relation.relation_kind.has_value() && !is_relation_kind(*relation.relation_kind))
        {
            diagnostics.error(
                relation.range, "SSPEC3303",
                "relation kind must be composition, aggregation, or reference"
            );
        }
        if (relation.on_parent_delete.has_value() &&
            !is_parent_delete_behavior(*relation.on_parent_delete))
        {
            diagnostics.error(
                relation.range, "SSPEC3304",
                "on_parent_delete must be cascade, block, detach, or fail"
            );
        }
        if (relation.on_parent_delete.has_value() && *relation.on_parent_delete == "detach" &&
            !relation.optional)
        {
            diagnostics.error(
                relation.range, "SSPEC3305",
                "detach parent delete behavior requires an optional parent relation"
            );
        }
        for (const auto& field : relation.unique_within_parent)
        {
            if (!contains(fields, field))
            {
                unknown_reference_error(
                    diagnostics, relation.range, "unique_within_parent field", field
                );
            }
        }
        if (target_entity != nullptr)
        {
            const auto states = entity_state_names(*target_entity);
            for (const auto& state : relation.parent_must_be_in)
            {
                if (!contains(states, state))
                {
                    unknown_reference_error(diagnostics, relation.range, "parent state", state);
                }
            }
        }
    }
}

void validate_children(
    const SystemDecl& system,
    const EntityDecl& entity,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> child_names;
    for (const auto& child : entity.children)
    {
        if (!child_names.insert(child.name).second)
        {
            duplicate_error(diagnostics, child.range, child.name);
        }
        const auto* target_entity = find_entity(system, child.target_entity);
        if (target_entity == nullptr)
        {
            unknown_reference_error(diagnostics, child.range, "child entity", child.target_entity);
            continue;
        }
        const auto relation = std::find_if(
            target_entity->relations.begin(), target_entity->relations.end(),
            [&](const RelationDecl& candidate)
            { return candidate.kind == "parent" && candidate.name == child.relation; }
        );
        if (relation == target_entity->relations.end())
        {
            unknown_reference_error(
                diagnostics, child.range, "child parent relation", child.relation
            );
        }
        else if (relation_target_name(relation->target) != entity.name)
        {
            diagnostics.error(
                child.range, "SSPEC3306",
                "child collection '" + child.name +
                    "' must reference a parent relation to entity '" + entity.name + "'"
            );
        }
    }
}

void validate_invariants(
    const EntityDecl& entity,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> invariant_names;
    for (const auto& invariant : entity.invariants)
    {
        if (!invariant_names.insert(invariant.name).second)
        {
            duplicate_error(diagnostics, invariant.range, invariant.name);
        }
        if (invariant.expression.empty())
        {
            required_error(
                diagnostics, invariant.range, "invariant '" + invariant.name + "'", "expression"
            );
        }
    }
}

void validate_entities(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& entity : system.entities)
    {
        if (entity.key_fields.empty())
        {
            diagnostics.error(
                entity.range, "SSPEC3101", "entity '" + entity.name + "' must declare a key"
            );
        }
        if (entity.fields.empty())
        {
            required_error(diagnostics, entity.range, "entity '" + entity.name + "'", "fields");
        }

        validate_field_duplicates(entity.fields, diagnostics);
        validate_field_types(entity.fields, symbols, diagnostics);
        validate_entity_management_fields(entity, diagnostics);

        const auto fields = field_names(entity.fields);
        for (const auto& key_field : entity.key_fields)
        {
            if (!contains(fields, key_field))
            {
                unknown_reference_error(diagnostics, entity.range, "entity key field", key_field);
            }
        }

        validate_ownership(entity, diagnostics);
        validate_relations(system, entity, fields, diagnostics);
        validate_children(system, entity, diagnostics);
        validate_invariants(entity, diagnostics);
        validate_indexes(entity, fields, diagnostics);
        validate_state_machine(entity, diagnostics);
    }
}

void validate_queues(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& queue : system.queues)
    {
        if (!queue.namespace_name.has_value())
        {
            required_error(diagnostics, queue.range, "queue '" + queue.name + "'", "namespace");
        }
        if (!queue.channel.has_value())
        {
            required_error(diagnostics, queue.range, "queue '" + queue.name + "'", "channel");
        }
        if (queue.visibility_timeout.has_value() && !is_duration_literal(*queue.visibility_timeout))
        {
            duration_error(
                diagnostics, queue.range, "queue '" + queue.name + "'", "visibility_timeout"
            );
        }
        if (queue.max_attempts.has_value() && !is_positive_integer(*queue.max_attempts))
        {
            positive_integer_error(
                diagnostics, queue.range, "queue '" + queue.name + "'", "max_attempts"
            );
        }
        if (queue.messages.empty())
        {
            required_error(
                diagnostics, queue.range, "queue '" + queue.name + "'", "at least one message"
            );
        }

        std::unordered_set<std::string> message_names;
        for (const auto& message : queue.messages)
        {
            if (!message_names.insert(message.name).second)
            {
                duplicate_error(diagnostics, message.range, queue_message_name(queue, message));
            }
            if (!message.idempotency_key.has_value())
            {
                required_error(
                    diagnostics, message.range,
                    "message '" + queue_message_name(queue, message) + "'", "idempotency_key"
                );
            }
            if (message.payload_fields.empty())
            {
                required_error(
                    diagnostics, message.range,
                    "message '" + queue_message_name(queue, message) + "'", "payload"
                );
            }

            validate_field_duplicates(message.payload_fields, diagnostics);
            validate_field_types(message.payload_fields, symbols, diagnostics);

            if (message.idempotency_key.has_value())
            {
                const auto payload_fields = field_names(message.payload_fields);
                if (!contains(payload_fields, *message.idempotency_key))
                {
                    unknown_reference_error(
                        diagnostics, message.range, "message idempotency_key field",
                        *message.idempotency_key
                    );
                }
            }
        }

        if (queue.dead_letter.has_value() && !symbols.find(*queue.dead_letter).has_value())
        {
            unknown_reference_error(diagnostics, queue.range, "dead_letter", *queue.dead_letter);
        }
    }
}

void validate_leases(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    for (const auto& lease : system.leases)
    {
        if (!lease.resource.has_value())
        {
            required_error(diagnostics, lease.range, "lease '" + lease.name + "'", "resource");
        }
        if (!lease.ttl.has_value())
        {
            required_error(diagnostics, lease.range, "lease '" + lease.name + "'", "ttl");
        }
        else if (!is_duration_literal(*lease.ttl))
        {
            duration_error(diagnostics, lease.range, "lease '" + lease.name + "'", "ttl");
        }
        if (lease.renew_every.has_value() && !is_duration_literal(*lease.renew_every))
        {
            duration_error(diagnostics, lease.range, "lease '" + lease.name + "'", "renew_every");
        }
        if (lease.max_ttl.has_value() && !is_duration_literal(*lease.max_ttl))
        {
            duration_error(diagnostics, lease.range, "lease '" + lease.name + "'", "max_ttl");
        }
    }
}

void validate_workflows(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& workflow : system.workflows)
    {
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
        if (workflow.steps.empty())
        {
            required_error(
                diagnostics, workflow.range, "workflow '" + workflow.name + "'", "at least one step"
            );
        }
        if (workflow.expected_execution_time.has_value() &&
            !is_duration_literal(*workflow.expected_execution_time))
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
            if (step.max_retries.has_value() && !is_non_negative_integer(*step.max_retries))
            {
                non_negative_integer_error(
                    diagnostics, step.range,
                    "workflow step '" + workflow_step_name(workflow, step) + "'", "max_retries"
                );
            }

            for (const auto& statement : step.statements)
            {
                if (statement.expression.has_value())
                {
                    validate_feature_flag_expression(
                        system, statement.range, *statement.expression, diagnostics
                    );
                }
                for (const auto& assignment : statement.payload)
                {
                    validate_feature_flag_expression(
                        system, assignment.range, assignment.expression, diagnostics
                    );
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
                if (statement.kind == "transition_to" && statement.target.has_value() &&
                    !contains(steps, *statement.target))
                {
                    unknown_reference_error(
                        diagnostics, statement.range, "workflow transition target",
                        *statement.target
                    );
                }
            }
        }
    }
}

void validate_workers(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& worker : system.workers)
    {
        if (worker.singleton.value_or(false) && !worker.lease.has_value())
        {
            diagnostics.error(
                worker.range, "SSPEC3301",
                "singleton worker '" + worker.name + "' must declare a lease"
            );
        }
        if (worker.concurrency.has_value() && !is_positive_integer(*worker.concurrency))
        {
            positive_integer_error(
                diagnostics, worker.range, "worker '" + worker.name + "'", "concurrency"
            );
        }

        if (worker.lease.has_value() && !symbols.find(*worker.lease).has_value())
        {
            unknown_reference_error(diagnostics, worker.range, "worker lease", *worker.lease);
        }
        if (worker.polls.has_value() && !symbols.find(*worker.polls).has_value())
        {
            unknown_reference_error(
                diagnostics, worker.range, "worker polls target", *worker.polls
            );
        }
        if (worker.executes.has_value() && !symbols.find(*worker.executes).has_value())
        {
            unknown_reference_error(
                diagnostics, worker.range, "worker executes target", *worker.executes
            );
        }
    }
}

void validate_apis(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& api : system.apis)
    {
        if (!api.method.has_value())
        {
            required_error(diagnostics, api.range, "api '" + api.name + "'", "method");
        }
        if (!api.path.has_value())
        {
            required_error(diagnostics, api.range, "api '" + api.name + "'", "path");
        }
        if (api.input.has_value() && !is_known_type_reference(*api.input, symbols))
        {
            unknown_reference_error(diagnostics, api.range, "API input", *api.input);
        }
        if (api.output.has_value() && !is_known_type_reference(*api.output, symbols))
        {
            unknown_reference_error(diagnostics, api.range, "API output", *api.output);
        }
        if (api.error.has_value() && !is_known_type_reference(*api.error, symbols))
        {
            unknown_reference_error(diagnostics, api.range, "API error", *api.error);
        }
        if (api.starts_workflow.has_value() && !symbols.find(*api.starts_workflow).has_value())
        {
            unknown_reference_error(
                diagnostics, api.range, "API starts workflow", *api.starts_workflow
            );
        }
        if (api.enqueues.has_value() && !symbols.find(*api.enqueues).has_value())
        {
            unknown_reference_error(diagnostics, api.range, "API enqueues target", *api.enqueues);
        }
    }
}

void validate_api_servers(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& api_server : system.api_servers)
    {
        if (api_server.concurrency.has_value() && !is_positive_integer(*api_server.concurrency))
        {
            positive_integer_error(
                diagnostics, api_server.range, "api_server '" + api_server.name + "'", "concurrency"
            );
        }

        for (const auto& served_api : api_server.serves)
        {
            validate_symbol_reference(
                symbols, api_server.range, "api_server served API", served_api, {SymbolKind::Api},
                diagnostics
            );
        }
    }
}

} // namespace

void Validator::validate(
    const Spec& spec,
    DiagnosticBag& diagnostics
)
{
    if (!spec.system.has_value())
    {
        diagnostics.error(SourceRange{}, "SSPEC1001", "spec must contain a system declaration");
        return;
    }

    const auto& system = *spec.system;
    auto symbols = build_symbol_table(system, diagnostics);
    const ValidatorContext context{spec, system, symbols, diagnostics};

    validate_feature_flags(context.system, context.symbols, context.diagnostics);
    validate_namespaces(context.system, context.symbols, context.diagnostics);
    validate_values(context.system, context.symbols, context.diagnostics);
    validate_enums(context.diagnostics, context.system);
    validate_events(context.system, context.symbols, context.diagnostics);
    validate_external_systems(context.system, context.diagnostics);
    validate_shapes(context.system, context.symbols, context.diagnostics);
    validate_logs(context.system, context.symbols, context.diagnostics);
    validate_metrics(context.system, context.symbols, context.diagnostics);
    validate_entities(context.system, context.symbols, context.diagnostics);
    validate_queues(context.system, context.symbols, context.diagnostics);
    validate_leases(context.system, context.diagnostics);
    validate_workflows(context.system, context.symbols, context.diagnostics);
    validate_workers(context.system, context.symbols, context.diagnostics);
    validate_apis(context.system, context.symbols, context.diagnostics);
    validate_api_servers(context.system, context.symbols, context.diagnostics);
    validate_policies(context.system, context.symbols, context.diagnostics);
}

} // namespace statespec
