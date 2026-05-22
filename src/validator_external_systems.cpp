#include "validator_declarations.hpp"

#include "statespec/language_constants.hpp"
#include "validator_helpers.hpp"

#include <algorithm>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace statespec::validator_detail
{

namespace
{

constexpr std::string_view MetadataStatusActive = "Active";
constexpr std::string_view MetadataStatusDisabled = "Disabled";
constexpr std::string_view MetadataStatusDeleted = "Deleted";

std::unordered_set<std::string> entity_field_names(const EntityDecl& entity)
{
    std::unordered_set<std::string> fields;
    for (const auto& field : entity.fields)
    {
        fields.insert(field.name);
    }
    return fields;
}

std::vector<std::string> split_path(const std::string& path)
{
    std::vector<std::string> segments;
    std::string current;
    for (const auto ch : path)
    {
        if (ch == '.')
        {
            segments.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(ch);
        }
    }
    segments.push_back(current);
    return segments;
}

bool contains_name(
    const std::unordered_set<std::string>& names,
    const std::string& name
)
{
    return names.find(name) != names.end();
}

const FieldDecl* find_field(
    const EntityDecl& entity,
    const std::string& name
)
{
    for (const auto& field : entity.fields)
    {
        if (field.name == name)
        {
            return &field;
        }
    }
    return nullptr;
}

bool has_state(
    const StateMachineDecl& state_machine,
    const std::string& name
)
{
    return std::any_of(
        state_machine.states.begin(), state_machine.states.end(),
        [&](const StateDecl& state) { return state.name == name; }
    );
}

const StateDecl* find_state(
    const StateMachineDecl& state_machine,
    const std::string& name
)
{
    for (const auto& state : state_machine.states)
    {
        if (state.name == name)
        {
            return &state;
        }
    }
    return nullptr;
}

bool has_transition(
    const StateMachineDecl& state_machine,
    const std::string& from,
    const std::string& to
)
{
    return std::any_of(
        state_machine.transitions.begin(), state_machine.transitions.end(),
        [&](const TransitionDecl& transition)
        { return transition.from == from && transition.to == to; }
    );
}

bool has_unique_index_on_fields(
    const EntityDecl& entity,
    const std::vector<std::string>& fields
)
{
    return std::any_of(
        entity.indexes.begin(), entity.indexes.end(),
        [&](const IndexDecl& index) { return index.unique && index.fields == fields; }
    );
}

void validate_metadata_mapping_path_shape(
    const ExternalSystemDecl& external_system,
    const ExternalSystemMetadataMappingDecl& mapping,
    std::string_view side,
    const std::string& path,
    const std::unordered_set<std::string>& allowed_roots,
    DiagnosticBag& diagnostics
)
{
    const auto segments = split_path(path);
    if (segments.size() < 2)
    {
        diagnostics.error(
            mapping.range, "SSPEC4906",
            "external_system '" + external_system.name + "' metadata mapping " + std::string(side) +
                " '" + path + "' must include a root and field"
        );
        return;
    }

    if (!contains_name(allowed_roots, segments.front()))
    {
        diagnostics.error(
            mapping.range, "SSPEC4907",
            "external_system '" + external_system.name + "' metadata mapping " + std::string(side) +
                " '" + path + "' uses unsupported root '" + segments.front() + "'"
        );
    }
}

void validate_external_system_metadata_entity_lifecycle(
    const ExternalSystemDecl& external_system,
    const ExternalSystemMetadataDecl& metadata,
    const EntityDecl& entity,
    DiagnosticBag& diagnostics
)
{
    if (!entity.ownership.has_value() ||
        entity.ownership->authority.value_or("") != OwnershipAuthoritySystem ||
        entity.ownership->system_of_record.value_or("") != OwnershipSystemOfRecordSelf ||
        entity.ownership->lifecycle.value_or("") != OwnershipLifecycleAuthoritative)
    {
        diagnostics.error(
            metadata.range, "SSPEC4911",
            "external_system '" + external_system.name + "' metadata entity '" + entity.name +
                "' must use authoritative system ownership"
        );
    }

    const auto* created_at = find_field(entity, std::string{EntityCreatedAtFieldName});
    const auto* updated_at = find_field(entity, std::string{EntityUpdatedAtFieldName});
    const auto* status = find_field(entity, std::string{EntityStatusFieldName});
    if (created_at == nullptr || created_at->type != EntityCreatedAtFieldType ||
        updated_at == nullptr || updated_at->type != EntityUpdatedAtFieldType ||
        status == nullptr || status->type != EntityStatusFieldType)
    {
        diagnostics.error(
            metadata.range, "SSPEC4912",
            "external_system '" + external_system.name + "' metadata entity '" + entity.name +
                "' must declare created_at timestamp, updated_at timestamp, and status string"
        );
    }

    if (metadata.profile_field.has_value() &&
        std::find(entity.key_fields.begin(), entity.key_fields.end(), *metadata.profile_field) ==
            entity.key_fields.end())
    {
        diagnostics.error(
            metadata.range, "SSPEC4913",
            "external_system '" + external_system.name + "' metadata profile_field '" +
                *metadata.profile_field + "' must be part of metadata entity '" + entity.name +
                "' key"
        );
    }

    if (!has_unique_index_on_fields(entity, entity.key_fields))
    {
        diagnostics.error(
            metadata.range, "SSPEC4914",
            "external_system '" + external_system.name + "' metadata entity '" + entity.name +
                "' must declare a unique index on its key fields"
        );
    }

    if (!entity.state_machine.has_value())
    {
        diagnostics.error(
            metadata.range, "SSPEC4915",
            "external_system '" + external_system.name + "' metadata entity '" + entity.name +
                "' must declare states Active, Disabled, and Deleted"
        );
        return;
    }

    const auto& state_machine = *entity.state_machine;
    for (const auto* required_state : {
             MetadataStatusActive.data(),
             MetadataStatusDisabled.data(),
             MetadataStatusDeleted.data(),
         })
    {
        if (!has_state(state_machine, required_state))
        {
            diagnostics.error(
                metadata.range, "SSPEC4915",
                "external_system '" + external_system.name + "' metadata entity '" + entity.name +
                    "' must declare state " + std::string(required_state)
            );
        }
    }

    const auto* deleted = find_state(state_machine, std::string{MetadataStatusDeleted});
    const auto deleted_declared_terminal =
        std::find(
            state_machine.terminal_states.begin(), state_machine.terminal_states.end(),
            std::string{MetadataStatusDeleted}
        ) != state_machine.terminal_states.end();
    if (deleted == nullptr || !deleted->terminal || !deleted_declared_terminal ||
        !deleted->garbage_collection.has_value())
    {
        diagnostics.error(
            metadata.range, "SSPEC4916",
            "external_system '" + external_system.name + "' metadata entity '" + entity.name +
                "' state Deleted must be terminal and declare garbage_collection"
        );
    }

    const std::vector<std::pair<std::string, std::string>> required_transitions{
        {std::string{MetadataStatusActive}, std::string{MetadataStatusDisabled}},
        {std::string{MetadataStatusDisabled}, std::string{MetadataStatusActive}},
        {std::string{MetadataStatusActive}, std::string{MetadataStatusDeleted}},
        {std::string{MetadataStatusDisabled}, std::string{MetadataStatusDeleted}},
    };
    for (const auto& [from, to] : required_transitions)
    {
        if (!has_transition(state_machine, from, to))
        {
            diagnostics.error(
                metadata.range, "SSPEC4917",
                "external_system '" + external_system.name + "' metadata entity '" + entity.name +
                    "' state_machine must declare transition " + from + " -> " + to
            );
        }
    }
}
} // namespace

void validate_external_systems(
    const SystemDecl& system,
    const SymbolTable& symbols,
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

        if (external_system.metadata.has_value())
        {
            const auto& metadata = *external_system.metadata;
            if (!metadata.entity.has_value())
            {
                required_error(diagnostics, metadata.range, "external_system metadata", "entity");
            }
            if (!metadata.profile_field.has_value())
            {
                required_error(
                    diagnostics, metadata.range, "external_system metadata", "profile_field"
                );
            }
            if (metadata.required_fields.empty())
            {
                required_error(
                    diagnostics, metadata.range, "external_system metadata", "required_fields"
                );
            }

            std::unordered_set<std::string> required_fields;
            for (const auto& field : metadata.required_fields)
            {
                if (!required_fields.insert(field).second)
                {
                    duplicate_error(diagnostics, metadata.range, field);
                }
            }

            const std::unordered_set<std::string> allowed_source_roots{
                "input", "entity", "workflow", "metadata"
            };
            const std::unordered_set<std::string> allowed_target_roots{"client", "request"};
            std::unordered_set<std::string> mapping_targets;
            for (const auto& mapping : metadata.mappings)
            {
                validate_metadata_mapping_path_shape(
                    external_system, mapping, "source", mapping.source, allowed_source_roots,
                    diagnostics
                );
                validate_metadata_mapping_path_shape(
                    external_system, mapping, "target", mapping.target, allowed_target_roots,
                    diagnostics
                );
                if (!mapping.target.empty() && !mapping_targets.insert(mapping.target).second)
                {
                    diagnostics.error(
                        mapping.range, "SSPEC4908",
                        "external_system '" + external_system.name + "' metadata mapping target '" +
                            mapping.target + "' is mapped more than once"
                    );
                }
            }

            if (!metadata.entity.has_value())
            {
                continue;
            }

            validate_symbol_reference(
                symbols, metadata.range, "external_system metadata entity", *metadata.entity,
                {SymbolKind::Entity}, diagnostics
            );

            const auto* entity = find_entity(system, *metadata.entity);
            if (entity == nullptr)
            {
                continue;
            }

            const auto fields = entity_field_names(*entity);
            if (metadata.profile_field.has_value() &&
                fields.find(*metadata.profile_field) == fields.end())
            {
                diagnostics.error(
                    metadata.range, "SSPEC4902",
                    "external_system '" + external_system.name + "' metadata profile_field '" +
                        *metadata.profile_field + "' must exist on entity '" + entity->name + "'"
                );
            }

            for (const auto& field : metadata.required_fields)
            {
                if (fields.find(field) == fields.end())
                {
                    diagnostics.error(
                        metadata.range, "SSPEC4903",
                        "external_system '" + external_system.name + "' metadata required field '" +
                            field + "' must exist on entity '" + entity->name + "'"
                    );
                }
            }

            for (const auto& mapping : metadata.mappings)
            {
                const auto source_segments = split_path(mapping.source);
                if (source_segments.size() >= 2 && source_segments.front() == "metadata")
                {
                    const auto& field = source_segments[1];
                    if (fields.find(field) == fields.end())
                    {
                        diagnostics.error(
                            mapping.range, "SSPEC4909",
                            "external_system '" + external_system.name +
                                "' metadata mapping source field '" + field +
                                "' must exist on entity '" + entity->name + "'"
                        );
                    }
                    if (required_fields.find(field) == required_fields.end())
                    {
                        diagnostics.error(
                            mapping.range, "SSPEC4910",
                            "external_system '" + external_system.name +
                                "' metadata mapping source field '" + field +
                                "' must be listed in required_fields"
                        );
                    }
                }
            }

            if (system.tenant_scope.has_value())
            {
                const auto& tenant_field = system.tenant_scope->field_name;
                if (fields.find(tenant_field) == fields.end())
                {
                    diagnostics.error(
                        metadata.range, "SSPEC4904",
                        "external_system '" + external_system.name + "' metadata entity '" +
                            entity->name + "' must declare tenant field '" + tenant_field + "'"
                    );
                }
                if (std::find(entity->key_fields.begin(), entity->key_fields.end(), tenant_field) ==
                    entity->key_fields.end())
                {
                    diagnostics.error(
                        metadata.range, "SSPEC4905",
                        "external_system '" + external_system.name + "' metadata entity '" +
                            entity->name + "' key must include tenant field '" + tenant_field + "'"
                    );
                }
            }

            validate_external_system_metadata_entity_lifecycle(
                external_system, metadata, *entity, diagnostics
            );
        }
    }
}
} // namespace statespec::validator_detail
