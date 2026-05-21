#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_go_external_system_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "func ExternalSystemDescriptors() []ExternalSystemDescriptor {\n";
    out << "\treturn []ExternalSystemDescriptor{\n";
    for (const auto& external_system : system.external_systems)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(external_system.name) << ",\n";
        out << "\t\t\tProperties: []ExternalSystemPropertyDescriptor{\n";
        for (const auto& property : external_system.properties)
        {
            out << "\t\t\t\t{Name: " << go_string(property.name)
                << ", Value: " << go_string(property.value) << "},\n";
        }
        out << "\t\t\t},\n";
        if (external_system.metadata.has_value())
        {
            out << "\t\t\tMetadata: &ExternalSystemMetadataDescriptor{\n";
            out << "\t\t\t\tEntity: " << go_string(external_system.metadata->entity) << ",\n";
            if (external_system.metadata->tenant_field.has_value())
            {
                out << "\t\t\t\tTenantField: stringPtr("
                    << go_string(*external_system.metadata->tenant_field) << "),\n";
            }
            out << "\t\t\t\tProfileField: " << go_string(external_system.metadata->profile_field)
                << ",\n";
            out << "\t\t\t\tKeyFields: []string{";
            for (std::size_t i = 0; i < external_system.metadata->key_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << go_string(external_system.metadata->key_fields[i]);
            }
            out << "},\n";
            out << "\t\t\t\tRequiredFields: []string{";
            for (std::size_t i = 0; i < external_system.metadata->required_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << go_string(external_system.metadata->required_fields[i]);
            }
            out << "},\n";
            out << "\t\t\t\tMappings: []ExternalSystemMetadataMappingDescriptor{\n";
            for (const auto& mapping : external_system.metadata->mappings)
            {
                out << "\t\t\t\t\t{Source: " << go_string(mapping.source)
                    << ", Target: " << go_string(mapping.target) << "},\n";
            }
            out << "\t\t\t\t},\n";
            out << "\t\t\t},\n";
        }
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func BuildExternalSystemMetadataMappingPlan(descriptor ExternalSystemDescriptor) "
           "ExternalSystemMetadataMappingPlan {\n";
    out << "\tplan := ExternalSystemMetadataMappingPlan{}\n";
    out << "\tif descriptor.Metadata == nil {\n";
    out << "\t\treturn plan\n";
    out << "\t}\n";
    out << "\tfor _, mapping := range descriptor.Metadata.Mappings {\n";
    out << "\t\tsourceParts := strings.SplitN(mapping.Source, \".\", 2)\n";
    out << "\t\tsourceRoot := sourceParts[0]\n";
    out << "\t\tsourceField := \"\"\n";
    out << "\t\tif len(sourceParts) == 2 {\n";
    out << "\t\t\tsourceField = sourceParts[1]\n";
    out << "\t\t}\n";
    out << "\t\tif strings.HasPrefix(mapping.Target, \"client.\") {\n";
    out << "\t\t\tassignment := ExternalSystemMetadataMappingAssignment{Source: "
           "mapping.Source, SourceRoot: sourceRoot, SourceField: sourceField, TargetRoot: "
           "\"client\", Field: strings.TrimPrefix(mapping.Target, \"client.\")}\n";
    out << "\t\t\tplan.AllMappings = append(plan.AllMappings, assignment)\n";
    out << "\t\t\tplan.ClientMappings = append(plan.ClientMappings, assignment)\n";
    out << "\t\t} else if strings.HasPrefix(mapping.Target, \"request.\") {\n";
    out << "\t\t\tassignment := ExternalSystemMetadataMappingAssignment{Source: "
           "mapping.Source, SourceRoot: sourceRoot, SourceField: sourceField, TargetRoot: "
           "\"request\", Field: strings.TrimPrefix(mapping.Target, \"request.\")}\n";
    out << "\t\t\tplan.AllMappings = append(plan.AllMappings, assignment)\n";
    out << "\t\t\tplan.RequestMappings = append(plan.RequestMappings, assignment)\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn plan\n";
    out << "}\n\n";

    out << "func BuildExternalSystemMetadataLookup(descriptor ExternalSystemDescriptor, "
           "keyValues []ExternalSystemMetadataKeyValue) *ExternalSystemMetadataLookup {\n";
    out << "\tif descriptor.Metadata == nil {\n";
    out << "\t\treturn nil\n";
    out << "\t}\n";
    out << "\tmetadata := descriptor.Metadata\n";
    out << "\treturn &ExternalSystemMetadataLookup{\n";
    out << "\t\tExternalSystem: descriptor.Name,\n";
    out << "\t\tMetadataEntity: metadata.Entity,\n";
    out << "\t\tTenantField: metadata.TenantField,\n";
    out << "\t\tProfileField: metadata.ProfileField,\n";
    out << "\t\tKeyFields: append([]string{}, metadata.KeyFields...),\n";
    out << "\t\tKeyValues: keyValues,\n";
    out << "\t\tRequiredFields: append([]string{}, metadata.RequiredFields...),\n";
    out << "\t}\n";
    out << "}\n\n";

    out << "func ExternalSystemMetadataLookupByName(externalSystem string, keyValues "
           "[]ExternalSystemMetadataKeyValue) (*ExternalSystemMetadataLookup, bool) {\n";
    out << "\tfor _, descriptor := range ExternalSystemDescriptors() {\n";
    out << "\t\tif descriptor.Name == externalSystem {\n";
    out << "\t\t\treturn BuildExternalSystemMetadataLookup(descriptor, keyValues), true\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil, false\n";
    out << "}\n\n";

    out << "func ResolveExternalSystemMetadataTx(ctx context.Context, resolver "
           "ExternalSystemMetadataResolver, tx Transaction, descriptor ExternalSystemDescriptor, "
           "keyValues []ExternalSystemMetadataKeyValue) (*ExternalSystemMetadataResolution, "
           "bool, error) {\n";
    out << "\tlookup := BuildExternalSystemMetadataLookup(descriptor, keyValues)\n";
    out << "\tif lookup == nil {\n";
    out << "\t\treturn nil, false, nil\n";
    out << "\t}\n";
    out << "\tif !lookup.KeyComplete() {\n";
    out << "\t\treturn nil, true, nil\n";
    out << "\t}\n";
    out << "\tresolution, err := resolver.ResolveMetadataTx(ctx, tx, *lookup)\n";
    out << "\treturn resolution, true, err\n";
    out << "}\n\n";

    out << "func ResolveExternalSystemMetadataByNameTx(ctx context.Context, resolver "
           "ExternalSystemMetadataResolver, tx Transaction, externalSystem string, keyValues "
           "[]ExternalSystemMetadataKeyValue) (*ExternalSystemMetadataResolution, bool, "
           "error) {\n";
    out << "\tlookup, ok := ExternalSystemMetadataLookupByName(externalSystem, keyValues)\n";
    out << "\tif !ok || lookup == nil {\n";
    out << "\t\treturn nil, ok, nil\n";
    out << "\t}\n";
    out << "\tif !lookup.KeyComplete() {\n";
    out << "\t\treturn nil, true, nil\n";
    out << "\t}\n";
    out << "\tresolution, err := resolver.ResolveMetadataTx(ctx, tx, *lookup)\n";
    out << "\treturn resolution, true, err\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
