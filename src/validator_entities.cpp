#include "validator_entities.hpp"

#include "statespec/language_constants.hpp"
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
    if (kind == "api")
    {
        return 9;
    }
    return 10;
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
                member.range, diagnostic_codes::NoncanonicalEntityOrder,
                "entity '" + entity.name +
                    "' members should use canonical order: key, ownership, version, fields, "
                    "state_machine, relations, children, invariants, indexes, api"
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
    std::unordered_set<std::string> inline_terminal_states;
    std::unordered_set<std::string> declared_terminal_states;
    std::unordered_set<std::string> garbage_collected_states;
    for (const auto& state : state_machine.states)
    {
        if (!states.insert(state.name).second)
        {
            duplicate_error(diagnostics, state.range, state.name);
        }
        if (state.terminal)
        {
            inline_terminal_states.insert(state.name);
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
            continue;
        }
        declared_terminal_states.insert(terminal);
    }

    for (const auto& state : state_machine.states)
    {
        if (contains(inline_terminal_states, state.name) &&
            !contains(declared_terminal_states, state.name))
        {
            diagnostics.error(
                state.range, diagnostic_codes::EntityDuplicateState,
                "terminal state '" + state.name + "' must also appear in the terminal list"
            );
        }
    }

    for (const auto& terminal : declared_terminal_states)
    {
        if (!contains(inline_terminal_states, terminal))
        {
            diagnostics.error(
                state_machine.range, diagnostic_codes::EntityMissingInitialState,
                "terminal state '" + terminal + "' must declare terminal: true"
            );
        }
    }

    for (const auto& state : state_machine.states)
    {
        const auto is_terminal_state = contains(inline_terminal_states, state.name) ||
                                       contains(declared_terminal_states, state.name);
        if (is_terminal_state && !state.garbage_collection.has_value())
        {
            diagnostics.error(
                state.range, diagnostic_codes::EntityStateTransitionGcConflict,
                "terminal state '" + state.name + "' must declare garbage_collection"
            );
        }

        if (!state.garbage_collection.has_value())
        {
            continue;
        }

        if (!is_terminal_state)
        {
            diagnostics.error(
                state.garbage_collection->range,
                diagnostic_codes::EntityTerminalGcRequiresRetention,
                "garbage_collection for state '" + state.name + "' is valid only on terminal states"
            );
        }
        else
        {
            garbage_collected_states.insert(state.name);
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
                state.garbage_collection->range, diagnostic_codes::EntityGcRequiresTerminalState,
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
                *state.duplicate_garbage_collection_range, diagnostic_codes::DuplicateDeclaration,
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
                transition.range, diagnostic_codes::EntityUnknownTransitionState,
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
            field->range, diagnostic_codes::EntityInvalidStateType,
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
    validate_required_entity_field(
        entity, std::string{EntityCreatedAtFieldName}, std::string{EntityCreatedAtFieldType},
        diagnostics
    );
    validate_required_entity_field(
        entity, std::string{EntityUpdatedAtFieldName}, std::string{EntityUpdatedAtFieldType},
        diagnostics
    );
    validate_required_entity_field(
        entity, std::string{EntityStatusFieldName}, std::string{EntityStatusFieldType}, diagnostics
    );

    static const std::vector<std::string> canonical_fields{
        std::string{EntityCreatedAtFieldName},
        std::string{EntityUpdatedAtFieldName},
        std::string{EntityStatusFieldName},
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
                entity.fields[i].range, diagnostic_codes::EntityDuplicateFieldName,
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
            entity.range, diagnostic_codes::TenantEntityMissingTenantField,
            "entity '" + entity.name + "' must declare tenant field '" + tenant_field + "'"
        );
    }
}

void validate_entity_tenant_key(
    const SystemDecl& system,
    const EntityDecl& entity,
    DiagnosticBag& diagnostics
)
{
    if (!system.tenant_scope.has_value())
    {
        return;
    }

    const auto& tenant_field = system.tenant_scope->field_name;
    if (std::find(entity.key_fields.begin(), entity.key_fields.end(), tenant_field) ==
        entity.key_fields.end())
    {
        diagnostics.error(
            entity.range, diagnostic_codes::TenantEntityKeyMissingTenantField,
            "entity '" + entity.name + "' key must include tenant field '" + tenant_field + "'"
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

bool is_prefix(
    const std::vector<std::string>& prefix,
    const std::vector<std::string>& values
)
{
    return !prefix.empty() && prefix.size() <= values.size() &&
           std::equal(prefix.begin(), prefix.end(), values.begin());
}

std::vector<std::string> path_parameters(const std::string& path)
{
    std::vector<std::string> parameters;
    std::size_t cursor = 0;
    while (cursor < path.size())
    {
        const auto open = path.find('{', cursor);
        if (open == std::string::npos)
        {
            break;
        }
        const auto close = path.find('}', open + 1);
        if (close == std::string::npos)
        {
            break;
        }
        if (close > open + 1)
        {
            parameters.push_back(path.substr(open + 1, close - open - 1));
        }
        cursor = close + 1;
    }
    return parameters;
}

std::unordered_set<std::string> path_parameter_set(const std::optional<std::string>& path)
{
    std::unordered_set<std::string> parameters;
    if (!path.has_value())
    {
        return parameters;
    }
    for (const auto& parameter : path_parameters(*path))
    {
        parameters.insert(parameter);
    }
    return parameters;
}

bool has_all_key_fields(
    const std::unordered_set<std::string>& path_fields,
    const EntityDecl& entity
)
{
    return std::all_of(
        entity.key_fields.begin(), entity.key_fields.end(),
        [&](const std::string& key_field) { return contains(path_fields, key_field); }
    );
}

bool entity_api_list_selector_is_allowed(
    const EntityDecl& entity,
    const EntityApiListDecl& list
)
{
    if (is_prefix(list.by, entity.key_fields))
    {
        return true;
    }

    for (const auto& index : entity.indexes)
    {
        if (list.by.size() == 1 && list.by[0] == index.name)
        {
            return true;
        }
        if (is_prefix(list.by, index.fields))
        {
            return true;
        }
    }

    return false;
}

void validate_entity_api_path_parameters(
    const EntityDecl& entity,
    const std::optional<std::string>& path,
    const SourceRange& range,
    DiagnosticBag& diagnostics
)
{
    if (!path.has_value())
    {
        return;
    }

    const auto fields = field_names(entity.fields);
    for (const auto& parameter : path_parameters(*path))
    {
        if (!contains(fields, parameter))
        {
            unknown_reference_error(diagnostics, range, "entity api path parameter", parameter);
        }
    }
}

void validate_entity_api_resource_path(
    const EntityDecl& entity,
    const EntityApiDecl& api,
    DiagnosticBag& diagnostics
)
{
    validate_entity_api_path_parameters(entity, api.resource, api.range, diagnostics);

    const auto resource_parameters = path_parameter_set(api.resource);
    if (!api.resource.has_value())
    {
        if (api.get.has_value() || api.update_status.has_value() || api.delete_.has_value())
        {
            required_error(diagnostics, api.range, "entity api", "resource");
        }
        return;
    }

    if (!has_all_key_fields(resource_parameters, entity))
    {
        diagnostics.error(
            api.range, diagnostic_codes::UnknownReference,
            "entity api resource path for entity '" + entity.name +
                "' must include all key fields"
        );
    }

    if ((api.get.has_value() || api.update_status.has_value() || api.delete_.has_value()) &&
        !has_all_key_fields(resource_parameters, entity))
    {
        diagnostics.error(
            api.range, diagnostic_codes::UnknownReference,
            "entity api get, update_status, and delete require key fields resolvable from resource "
            "path"
        );
    }
}

void validate_entity_api_create(
    const EntityDecl& entity,
    const EntityApiCreateDecl& create,
    DiagnosticBag& diagnostics
)
{
    const auto fields = field_names(entity.fields);
    static const std::unordered_set<std::string> disallowed_fields{
        std::string{EntityCreatedAtFieldName},
        std::string{EntityUpdatedAtFieldName},
        std::string{EntityStatusFieldName},
    };

    for (const auto& field : create.fields)
    {
        if (!contains(fields, field))
        {
            unknown_reference_error(diagnostics, create.range, "entity api create field", field);
        }
        if (contains(disallowed_fields, field))
        {
            diagnostics.error(
                create.range, diagnostic_codes::EntityDuplicateFieldName,
                "entity api create for entity '" + entity.name +
                    "' must not supply created_at, updated_at, or status"
            );
        }
    }
}

bool has_conventional_delete_terminal_state(const EntityDecl& entity)
{
    if (!entity.state_machine.has_value())
    {
        return false;
    }

    for (const auto& state : entity.state_machine->states)
    {
        if (state.name == ConventionalSoftDeleteTerminalStateName && state.terminal &&
            state.garbage_collection.has_value())
        {
            return true;
        }
    }
    return false;
}

void validate_entity_api(
    const EntityDecl& entity,
    DiagnosticBag& diagnostics
)
{
    if (!entity.api.has_value())
    {
        return;
    }

    const auto& api = *entity.api;
    validate_entity_api_resource_path(entity, api, diagnostics);

    if (api.create.has_value())
    {
        validate_entity_api_create(entity, *api.create, diagnostics);
    }

    for (const auto& list : entity.api->lists)
    {
        validate_entity_api_path_parameters(entity, list.path, list.range, diagnostics);
        if (list.by.empty())
        {
            required_error(diagnostics, list.range, "entity api list", "by");
            continue;
        }
        if (!entity_api_list_selector_is_allowed(entity, list))
        {
            unknown_reference_error(
                diagnostics, list.range, "entity api list selector", list.by.front()
            );
        }
    }

    if (api.update_status.has_value())
    {
        if (find_field(entity.fields, std::string{EntityStatusFieldName}) == nullptr)
        {
            required_error(
                diagnostics, api.update_status->range,
                "entity api update_status for entity '" + entity.name + "'", "status field"
            );
        }
        if (!entity.state_machine.has_value())
        {
            required_error(
                diagnostics, api.update_status->range,
                "entity api update_status for entity '" + entity.name + "'", "state_machine"
            );
        }
    }

    if (api.delete_.has_value() && !has_conventional_delete_terminal_state(entity))
    {
        diagnostics.error(
            api.delete_->range, diagnostic_codes::EntityStateTransitionGcConflict,
            "entity api delete for entity '" + entity.name +
                "' requires terminal state Deleted with garbage_collection"
        );
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
            ownership.range, diagnostic_codes::OwnershipInvalidAuthority,
            "ownership authority must be system or external"
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
            ownership.range, diagnostic_codes::OwnershipInvalidLifecycle,
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
                relation.range, diagnostic_codes::RelationInvalidParentDelete,
                "relation kind must be composition, aggregation, or reference"
            );
        }
        if (relation.on_parent_delete.has_value() &&
            !is_parent_delete_behavior(*relation.on_parent_delete))
        {
            diagnostics.error(
                relation.range, diagnostic_codes::RelationDetachRequiresOptionalParent,
                "on_parent_delete must be cascade, block, detach, or fail"
            );
        }
        if (relation.on_parent_delete.has_value() && *relation.on_parent_delete == "detach" &&
            !relation.optional)
        {
            diagnostics.error(
                relation.range, diagnostic_codes::RelationUniqueFieldMissing,
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
                child.range, diagnostic_codes::ChildUnknownRelation,
                "child collection '" + child.name +
                    "' must reference a parent relation to entity '" + entity.name + "'"
            );
        }
    }
}

void validate_invariants(
    const SystemDecl& system,
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
        else
        {
            validate_expression(system, invariant.range, invariant.expression, diagnostics);
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
                entity.range, diagnostic_codes::EntityMissingKey,
                "entity '" + entity.name + "' must declare a key"
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
        validate_entity_tenant_key(system, entity, diagnostics);
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
        validate_invariants(system, entity, diagnostics);
        validate_indexes(entity, fields, diagnostics);
        validate_entity_api(entity, diagnostics);
        validate_state_machine(entity, diagnostics);
    }
}

} // namespace statespec::validator_detail
