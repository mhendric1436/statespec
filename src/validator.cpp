#include "statespec/validator.hpp"

#include "validator_helpers.hpp"

#include "statespec/symbol_table.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace statespec
{

namespace
{

using namespace validator_detail;

const EntityDecl* find_entity(
    const SystemDecl& system,
    const std::string& name
)
{
    for (const auto& entity : system.entities)
    {
        if (entity.name == name)
        {
            return &entity;
        }
    }
    return nullptr;
}

bool symbol_is_one_of(
    const Symbol& symbol,
    const std::vector<SymbolKind>& kinds
)
{
    return std::find(kinds.begin(), kinds.end(), symbol.kind) != kinds.end();
}

void validate_symbol_reference(
    const SymbolTable& symbols,
    const SourceRange& range,
    const std::string& kind,
    const std::string& name,
    const std::vector<SymbolKind>& allowed_kinds,
    DiagnosticBag& diagnostics
)
{
    const auto symbol = symbols.find(name);
    if (!symbol.has_value() || !symbol_is_one_of(*symbol, allowed_kinds))
    {
        diagnostics.error(range, "SSPEC3002", "unknown " + kind + " reference '" + name + "'");
    }
}

std::unordered_set<std::string> entity_state_names(const EntityDecl& entity)
{
    std::unordered_set<std::string> states;
    if (!entity.state_machine.has_value())
    {
        return states;
    }
    for (const auto& state : entity.state_machine->states)
    {
        states.insert(state.name);
    }
    return states;
}

void duplicate_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& name
)
{
    diagnostics.error(range, "SSPEC3001", "duplicate declaration '" + name + "'");
}

void unknown_reference_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& kind,
    const std::string& name
)
{
    diagnostics.error(range, "SSPEC3002", "unknown " + kind + " reference '" + name + "'");
}

void required_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
)
{
    diagnostics.error(range, "SSPEC4001", subject + " must declare " + field);
}

void positive_integer_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
)
{
    diagnostics.error(range, "SSPEC4002", subject + " " + field + " must be a positive integer");
}

void non_negative_integer_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
)
{
    diagnostics.error(range, "SSPEC4003", subject + " " + field + " must be non-negative");
}

void duration_error(
    DiagnosticBag& diagnostics,
    const SourceRange& range,
    const std::string& subject,
    const std::string& field
)
{
    diagnostics.error(range, "SSPEC4004", subject + " " + field + " must be an ISO-8601 duration");
}

void add_symbol(
    SymbolTable& symbols,
    DiagnosticBag& diagnostics,
    SymbolKind kind,
    const std::string& name,
    const SourceRange& range
)
{
    if (!symbols.insert(Symbol{kind, name, range}))
    {
        duplicate_error(diagnostics, range, name);
    }
}

void validate_field_duplicates(
    const std::vector<FieldDecl>& fields,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> names;
    for (const auto& field : fields)
    {
        if (!names.insert(field.name).second)
        {
            duplicate_error(diagnostics, field.range, field.name);
        }
    }
}

void validate_field_types(
    const std::vector<FieldDecl>& fields,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& field : fields)
    {
        const auto base_type = base_type_name(field.type);
        if (!is_builtin_type(base_type) && !symbols.find(base_type).has_value())
        {
            unknown_reference_error(diagnostics, field.range, "type", base_type);
        }
    }
}

bool is_known_type_reference(
    const std::string& type,
    const SymbolTable& symbols
)
{
    const auto base_type = base_type_name(type);
    return is_builtin_type(base_type) || symbols.find(base_type).has_value();
}

const FeatureFlagDecl* find_feature_flag(
    const SystemDecl& system,
    const std::string& name
)
{
    for (const auto& flag : system.feature_flags)
    {
        if (flag.name == name)
        {
            return &flag;
        }
    }
    return nullptr;
}

std::vector<std::string> feature_flag_function_arguments(
    const std::string& expression,
    const std::string& function_name
)
{
    std::vector<std::string> arguments;
    std::size_t offset = 0;

    while ((offset = expression.find(function_name, offset)) != std::string::npos)
    {
        auto cursor = offset + function_name.size();
        while (cursor < expression.size() &&
               std::isspace(static_cast<unsigned char>(expression[cursor])) != 0)
        {
            ++cursor;
        }
        if (cursor >= expression.size() || expression[cursor] != '(')
        {
            offset = cursor;
            continue;
        }

        const auto arg_start = cursor + 1;
        const auto arg_end = expression.find(')', arg_start);
        if (arg_end == std::string::npos)
        {
            break;
        }

        auto argument = expression.substr(arg_start, arg_end - arg_start);
        argument.erase(
            std::remove_if(
                argument.begin(), argument.end(),
                [](char ch) { return std::isspace(static_cast<unsigned char>(ch)) != 0; }
            ),
            argument.end()
        );
        arguments.push_back(argument);
        offset = arg_end + 1;
    }

    return arguments;
}

void validate_feature_flag_expression(
    const SystemDecl& system,
    const SourceRange& range,
    const std::string& expression,
    DiagnosticBag& diagnostics
)
{
    for (const auto& flag_name : feature_flag_function_arguments(expression, "feature_enabled"))
    {
        const auto* flag = find_feature_flag(system, flag_name);
        if (flag == nullptr)
        {
            unknown_reference_error(diagnostics, range, "feature flag", flag_name);
            continue;
        }
        if (flag->type.value_or("") != "bool")
        {
            diagnostics.error(
                range, "SSPEC4204", "feature_enabled requires bool feature flag '" + flag_name + "'"
            );
        }
    }

    for (const auto& flag_name : feature_flag_function_arguments(expression, "feature_value"))
    {
        if (find_feature_flag(system, flag_name) == nullptr)
        {
            unknown_reference_error(diagnostics, range, "feature flag", flag_name);
        }
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

void validate_shapes(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& shape : system.shapes)
    {
        if (!is_qualified_pascal_case_name(shape.name))
        {
            diagnostics.error(
                shape.range, "SSPEC3201", "shape '" + shape.name + "' must use PascalCase"
            );
        }
        if (shape.fields.empty())
        {
            required_error(diagnostics, shape.range, "shape '" + shape.name + "'", "fields");
        }
        validate_field_duplicates(shape.fields, diagnostics);
        validate_field_types(shape.fields, symbols, diagnostics);
    }
}

void validate_values(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& value : system.values)
    {
        if (!is_qualified_pascal_case_name(value.name))
        {
            diagnostics.error(
                value.range, "SSPEC4501", "value '" + value.name + "' must use PascalCase"
            );
        }
        if (value.type.empty())
        {
            required_error(diagnostics, value.range, "value '" + value.name + "'", "type");
        }
        else if (!is_known_type_reference(value.type, symbols))
        {
            unknown_reference_error(diagnostics, value.range, "value type", value.type);
        }
        if (value.constraint.has_value())
        {
            validate_feature_flag_expression(system, value.range, *value.constraint, diagnostics);
        }
    }
}

void validate_namespaces(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& namespace_decl : system.namespaces)
    {
        if (!is_qualified_pascal_case_name(namespace_decl.name))
        {
            diagnostics.error(
                namespace_decl.range, "SSPEC4801",
                "namespace '" + namespace_decl.name + "' must use PascalCase segments"
            );
        }
        for (const auto& member : namespace_decl.members)
        {
            if (!symbols.find(member).has_value())
            {
                unknown_reference_error(
                    diagnostics, namespace_decl.range, "namespace member", member
                );
            }
        }
    }
}

void validate_enums(
    DiagnosticBag& diagnostics,
    const SystemDecl& system
)
{
    for (const auto& enum_decl : system.enums)
    {
        if (!is_qualified_pascal_case_name(enum_decl.name))
        {
            diagnostics.error(
                enum_decl.range, "SSPEC4601", "enum '" + enum_decl.name + "' must use PascalCase"
            );
        }
        if (enum_decl.members.empty())
        {
            required_error(diagnostics, enum_decl.range, "enum '" + enum_decl.name + "'", "member");
        }

        std::unordered_set<std::string> members;
        for (const auto& member : enum_decl.members)
        {
            if (!is_pascal_case_name(member.name))
            {
                diagnostics.error(
                    member.range, "SSPEC4602",
                    "enum member '" + enum_decl.name + "." + member.name + "' must use PascalCase"
                );
            }
            if (!members.insert(member.name).second)
            {
                duplicate_error(diagnostics, member.range, enum_decl.name + "." + member.name);
            }
        }
    }
}

void validate_events(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& event : system.events)
    {
        if (!is_qualified_pascal_case_name(event.name))
        {
            diagnostics.error(
                event.range, "SSPEC4701", "event '" + event.name + "' must use PascalCase"
            );
        }
        if (event.fields.empty())
        {
            required_error(diagnostics, event.range, "event '" + event.name + "'", "fields");
        }
        validate_field_duplicates(event.fields, diagnostics);
        validate_field_types(event.fields, symbols, diagnostics);
    }
}

void validate_external_systems(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    for (const auto& external_system : system.external_systems)
    {
        if (!is_qualified_pascal_case_name(external_system.name))
        {
            diagnostics.error(
                external_system.range, "SSPEC4901",
                "external_system '" + external_system.name + "' must use PascalCase segments"
            );
        }

        std::unordered_set<std::string> properties;
        for (const auto& property : external_system.properties)
        {
            if (!properties.insert(property.name).second)
            {
                duplicate_error(diagnostics, property.range, property.name);
            }
        }
    }
}

void validate_feature_flags(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& flag : system.feature_flags)
    {
        if (!is_qualified_pascal_case_name(flag.name))
        {
            diagnostics.error(
                flag.range, "SSPEC4201", "feature flag '" + flag.name + "' must use PascalCase"
            );
        }

        if (!flag.type.has_value())
        {
            required_error(diagnostics, flag.range, "feature_flag '" + flag.name + "'", "type");
        }
        else if (!is_supported_feature_flag_type(*flag.type))
        {
            diagnostics.error(
                flag.range, "SSPEC4202",
                "feature flag '" + flag.name + "' has unsupported type '" + *flag.type + "'"
            );
        }

        if (!flag.default_value.has_value())
        {
            required_error(diagnostics, flag.range, "feature_flag '" + flag.name + "'", "default");
        }
        else if (!feature_flag_default_matches_type(flag))
        {
            diagnostics.error(
                flag.range, "SSPEC4203",
                "feature flag '" + flag.name + "' default must match declared type"
            );
        }

        if (flag.scope.has_value())
        {
            const auto entity = entity_scope_target(*flag.scope);
            if (entity.has_value())
            {
                const auto symbol = symbols.find(*entity);
                if (!symbol.has_value() || symbol->kind != SymbolKind::Entity)
                {
                    unknown_reference_error(
                        diagnostics, flag.range, "feature flag entity scope", *entity
                    );
                }
            }
        }
    }
}

void validate_logs(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> event_names;
    for (const auto& log : system.logs)
    {
        if (!is_qualified_pascal_case_name(log.name))
        {
            diagnostics.error(log.range, "SSPEC4301", "log '" + log.name + "' must use PascalCase");
        }

        if (!log.level.has_value())
        {
            required_error(diagnostics, log.range, "log '" + log.name + "'", "level");
        }
        else if (!is_supported_log_level(*log.level))
        {
            diagnostics.error(
                log.range, "SSPEC4302",
                "log '" + log.name + "' has unsupported level '" + *log.level + "'"
            );
        }

        if (!log.event_name.has_value())
        {
            required_error(diagnostics, log.range, "log '" + log.name + "'", "event_name");
        }
        else if (!event_names.insert(*log.event_name).second)
        {
            diagnostics.error(
                log.range, "SSPEC4303", "duplicate log event_name '" + *log.event_name + "'"
            );
        }

        validate_field_duplicates(log.fields, diagnostics);
        validate_field_types(log.fields, symbols, diagnostics);
    }
}

void validate_metrics(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> backend_names;
    for (const auto& metric : system.metrics)
    {
        if (!is_qualified_pascal_case_name(metric.name))
        {
            diagnostics.error(
                metric.range, "SSPEC4401", "metric '" + metric.name + "' must use PascalCase"
            );
        }

        if (!metric.kind.has_value())
        {
            required_error(diagnostics, metric.range, "metric '" + metric.name + "'", "kind");
        }
        else if (!is_supported_metric_kind(*metric.kind))
        {
            diagnostics.error(
                metric.range, "SSPEC4402",
                "metric '" + metric.name + "' has unsupported kind '" + *metric.kind + "'"
            );
        }

        if (!metric.backend_name.has_value())
        {
            required_error(diagnostics, metric.range, "metric '" + metric.name + "'", "name");
        }
        else if (!backend_names.insert(*metric.backend_name).second)
        {
            diagnostics.error(
                metric.range, "SSPEC4403", "duplicate metric name '" + *metric.backend_name + "'"
            );
        }

        if (!metric.unit.has_value())
        {
            required_error(diagnostics, metric.range, "metric '" + metric.name + "'", "unit");
        }

        validate_field_duplicates(metric.labels, diagnostics);
        validate_field_types(metric.labels, symbols, diagnostics);
        for (const auto& label : metric.labels)
        {
            if (!is_supported_metric_label_type(label.type))
            {
                diagnostics.error(
                    label.range, "SSPEC4404",
                    "metric '" + metric.name + "' label '" + label.name +
                        "' must use low-cardinality type string, bool, or int"
                );
            }
        }
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

void validate_policies(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& policy : system.policies)
    {
        for (const auto& rule : policy.allows)
        {
            if (!symbols.find(rule.action).has_value())
            {
                unknown_reference_error(
                    diagnostics, rule.range, "policy allow action", rule.action
                );
            }
            validate_feature_flag_expression(system, rule.range, rule.condition, diagnostics);
        }
        for (const auto& rule : policy.denies)
        {
            if (!symbols.find(rule.action).has_value())
            {
                unknown_reference_error(diagnostics, rule.range, "policy deny action", rule.action);
            }
            validate_feature_flag_expression(system, rule.range, rule.condition, diagnostics);
        }
        for (const auto& quota : policy.quotas)
        {
            validate_feature_flag_expression(system, quota.range, quota.expression, diagnostics);
        }
        for (const auto& audit : policy.audits)
        {
            if (!symbols.find(audit).has_value())
            {
                unknown_reference_error(diagnostics, policy.range, "policy audit action", audit);
            }
        }
    }
}

SymbolTable build_symbol_table(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    SymbolTable symbols;
    add_symbol(symbols, diagnostics, SymbolKind::System, system.name, system.range);

    for (const auto& namespace_decl : system.namespaces)
    {
        add_symbol(
            symbols, diagnostics, SymbolKind::Namespace, namespace_decl.name, namespace_decl.range
        );
    }
    for (const auto& entity : system.entities)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Entity, entity.name, entity.range);
    }
    for (const auto& value : system.values)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Value, value.name, value.range);
    }
    for (const auto& enum_decl : system.enums)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Enum, enum_decl.name, enum_decl.range);
    }
    for (const auto& event : system.events)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Event, event.name, event.range);
    }
    for (const auto& shape : system.shapes)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Shape, shape.name, shape.range);
    }
    for (const auto& external_system : system.external_systems)
    {
        add_symbol(
            symbols, diagnostics, SymbolKind::ExternalSystem, external_system.name,
            external_system.range
        );
    }
    for (const auto& flag : system.feature_flags)
    {
        add_symbol(symbols, diagnostics, SymbolKind::FeatureFlag, flag.name, flag.range);
    }
    for (const auto& log : system.logs)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Log, log.name, log.range);
    }
    for (const auto& metric : system.metrics)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Metric, metric.name, metric.range);
    }
    for (const auto& queue : system.queues)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Queue, queue.name, queue.range);
        for (const auto& message : queue.messages)
        {
            add_symbol(
                symbols, diagnostics, SymbolKind::Message, queue_message_name(queue, message),
                message.range
            );
        }
    }
    for (const auto& lease : system.leases)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Lease, lease.name, lease.range);
    }
    for (const auto& worker : system.workers)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Worker, worker.name, worker.range);
    }
    for (const auto& api_server : system.api_servers)
    {
        add_symbol(symbols, diagnostics, SymbolKind::ApiServer, api_server.name, api_server.range);
    }
    for (const auto& api : system.apis)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Api, api.name, api.range);
    }
    for (const auto& workflow : system.workflows)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Workflow, workflow.name, workflow.range);
        for (const auto& step : workflow.steps)
        {
            add_symbol(
                symbols, diagnostics, SymbolKind::WorkflowStep, workflow_step_name(workflow, step),
                step.range
            );
        }
    }
    for (const auto& policy : system.policies)
    {
        add_symbol(symbols, diagnostics, SymbolKind::Policy, policy.name, policy.range);
    }

    return symbols;
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

    validate_feature_flags(system, symbols, diagnostics);
    validate_namespaces(system, symbols, diagnostics);
    validate_values(system, symbols, diagnostics);
    validate_enums(diagnostics, system);
    validate_events(system, symbols, diagnostics);
    validate_external_systems(system, diagnostics);
    validate_shapes(system, symbols, diagnostics);
    validate_logs(system, symbols, diagnostics);
    validate_metrics(system, symbols, diagnostics);
    validate_entities(system, symbols, diagnostics);
    validate_queues(system, symbols, diagnostics);
    validate_leases(system, diagnostics);
    validate_workflows(system, symbols, diagnostics);
    validate_workers(system, symbols, diagnostics);
    validate_apis(system, symbols, diagnostics);
    validate_api_servers(system, symbols, diagnostics);
    validate_policies(system, symbols, diagnostics);
}

} // namespace statespec
