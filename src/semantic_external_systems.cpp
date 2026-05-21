#include "semantic_external_systems.hpp"

#include <utility>

namespace statespec
{

namespace
{

std::vector<SemanticExternalSystemMetadataMapping> resolve_external_system_metadata_mappings(
    const std::vector<ExternalSystemMetadataMappingDecl>& mappings
)
{
    std::vector<SemanticExternalSystemMetadataMapping> resolved;
    for (const auto& mapping : mappings)
    {
        resolved.push_back(SemanticExternalSystemMetadataMapping{mapping.source, mapping.target});
    }
    return resolved;
}

const EntityDecl* find_entity_decl(
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

} // namespace

void resolve_semantic_external_systems(
    const SystemDecl& system,
    SemanticSystem& resolved
)
{
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
        if (external_system.metadata.has_value())
        {
            const auto entity_name = external_system.metadata->entity.value_or("");
            const auto* metadata_entity = find_entity_decl(system, entity_name);
            resolved_external_system.metadata = SemanticExternalSystemMetadata{
                entity_name,
                system.tenant_scope.has_value()
                    ? std::optional<std::string>{system.tenant_scope->field_name}
                    : std::nullopt,
                external_system.metadata->profile_field.value_or(""),
                metadata_entity != nullptr ? metadata_entity->key_fields
                                           : std::vector<std::string>{},
                external_system.metadata->required_fields,
                resolve_external_system_metadata_mappings(external_system.metadata->mappings),
            };
        }
        resolved.external_systems.push_back(std::move(resolved_external_system));
    }
}

} // namespace statespec
