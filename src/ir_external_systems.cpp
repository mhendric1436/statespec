#include "ir_external_systems.hpp"

#include <utility>

namespace statespec
{

namespace
{

std::vector<IrExternalSystemMetadataMapping> lower_external_system_metadata_mappings(
    const std::vector<SemanticExternalSystemMetadataMapping>& mappings
)
{
    std::vector<IrExternalSystemMetadataMapping> lowered;
    for (const auto& mapping : mappings)
    {
        lowered.push_back(IrExternalSystemMetadataMapping{mapping.source, mapping.target});
    }
    return lowered;
}

} // namespace

void lower_ir_external_systems(
    const SemanticSystem& system,
    IrSystem& ir
)
{
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
                external_system.metadata->entity,
                external_system.metadata->tenant_field,
                external_system.metadata->profile_field,
                external_system.metadata->key_fields,
                external_system.metadata->required_fields,
                lower_external_system_metadata_mappings(external_system.metadata->mappings),
            };
        }
        ir.external_systems.push_back(std::move(ir_external_system));
    }
}

} // namespace statespec
