#include "validator_entities.hpp"

#include "validator_declarations.hpp"
#include "validator_helpers.hpp"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

namespace statespec::validator_detail
{

namespace
{

int entity_member_order_index(const std::string& kind)
{
    if (kind == "key")
    {
        return 0;
    }
    if (kind == "ownership")
    {
        return 1;
    }
    if (kind == "version")
    {
        return 2;
    }
    if (kind == "fields")
    {
        return 3;
    }
    if (kind == "state_machine")
    {
        return 4;
    }
    if (kind == "relations")
    {
        return 5;
    }
    if (kind == "children")
    {
        return 6;
    }
    if (kind == "invariants")
    {
        return 7;
    }
    if (kind == "indexes")
    {
        return 8;
    }
    return 9;
}

void validate_entity_member_order(
    const EntityDecl& entity,
    DiagnosticBag& diagnostics
)
{
    int previous_order = -1;
    for (const auto& member : entity.member_order)
    {
        const auto order = entity_member_order_index(member.kind);
        if (order < previous_order)
        {
            diagnostics.warning(
                member.range, "SSPEC6101",
                "entity '" + entity.name +
                    "' members should use canonical order: key, ownership, version, fields, "
                    "state_machine, relations, children, invariants, indexes"
            );
            return;
        }
        previous_order = order;
    }
}

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

void validate_entity_tenant_field(
    const SystemDecl& system,
    const EntityDecl& entity,
    const std::unordered_set<std::string>& fields,
    DiagnosticBag& diagnostics
)
{
    if (!system.tenant_scope.has_value())
    {
        return;
    }

    const auto& tenant_field = system.tenant_scope->field_name;
    if (!contains(fields, tenant_field))
    {
        diagnostics.error(
            entity.range, "SSPEC3401",
            "entity '" + entity.name + "' must declare tenant field '" + tenant_field + "'"
        );
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
        required_error(diagnostics, entity.range, "entity '" + entity.name + "'", "ownership");
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

} // namespace

void validate_entities(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& entity : system.entities)
    {
        validate_entity_member_order(entity, diagnostics);

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
        validate_entity_tenant_field(system, entity, fields, diagnostics);
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

} // namespace statespec::validator_detail
