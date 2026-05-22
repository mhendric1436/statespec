#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_external_system_descriptors(
    const IrSystem& system,
    const std::string& external_system_call_adapters
)
{
    std::ostringstream out;
    out << "inline std::vector<ExternalSystemDescriptor> external_system_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& external_system : system.external_systems)
    {
        out << "        ExternalSystemDescriptor{\n";
        out << "            " << cpp_string(external_system.name) << ",\n";
        out << "            {\n";
        for (const auto& property : external_system.properties)
        {
            out << "                ExternalSystemPropertyDescriptor{" << cpp_string(property.name)
                << ", " << cpp_string(property.value) << "},\n";
        }
        out << "            },\n";
        if (external_system.metadata.has_value())
        {
            out << "            ExternalSystemMetadataDescriptor{\n";
            out << "                " << cpp_string(external_system.metadata->entity) << ",\n";
            if (external_system.metadata->tenant_field.has_value())
            {
                out << "                " << cpp_string(*external_system.metadata->tenant_field)
                    << ",\n";
            }
            else
            {
                out << "                std::nullopt,\n";
            }
            out << "                " << cpp_string(external_system.metadata->profile_field)
                << ",\n";
            out << "                {";
            for (std::size_t i = 0; i < external_system.metadata->key_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << cpp_string(external_system.metadata->key_fields[i]);
            }
            out << "},\n";
            out << "                {";
            for (std::size_t i = 0; i < external_system.metadata->required_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << cpp_string(external_system.metadata->required_fields[i]);
            }
            out << "},\n";
            out << "                {\n";
            for (const auto& mapping : external_system.metadata->mappings)
            {
                out << "                    ExternalSystemMetadataMappingDescriptor{"
                    << cpp_string(mapping.source) << ", " << cpp_string(mapping.target) << "},\n";
            }
            out << "                },\n";
            out << "            },\n";
        }
        else
        {
            out << "            std::nullopt,\n";
        }
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline ExternalSystemMetadataMappingPlan external_system_metadata_mapping_plan(\n";
    out << "    const ExternalSystemDescriptor& descriptor\n";
    out << ")\n";
    out << "{\n";
    out << "    ExternalSystemMetadataMappingPlan plan;\n";
    out << "    if (!descriptor.metadata.has_value())\n";
    out << "    {\n";
    out << "        return plan;\n";
    out << "    }\n";
    out << "    constexpr std::string_view client_prefix = \"client.\";\n";
    out << "    constexpr std::string_view request_prefix = \"request.\";\n";
    out << "    for (const auto& mapping : descriptor.metadata->mappings)\n";
    out << "    {\n";
    out << "        const auto source_separator = mapping.source.find('.');\n";
    out << "        const auto source_root = source_separator == std::string::npos\n";
    out << "            ? mapping.source\n";
    out << "            : mapping.source.substr(0, source_separator);\n";
    out << "        const auto source_field = source_separator == std::string::npos\n";
    out << "            ? std::string{}\n";
    out << "            : mapping.source.substr(source_separator + 1);\n";
    out << "        if (mapping.target.rfind(client_prefix, 0) == 0)\n";
    out << "        {\n";
    out << "            auto assignment = ExternalSystemMetadataMappingAssignment{\n";
    out << "                mapping.source, source_root, source_field, \"client\",\n";
    out << "                mapping.target.substr(client_prefix.size()),\n";
    out << "            };\n";
    out << "            plan.all_mappings.push_back(assignment);\n";
    out << "            plan.client_mappings.push_back(std::move(assignment));\n";
    out << "        }\n";
    out << "        else if (mapping.target.rfind(request_prefix, 0) == 0)\n";
    out << "        {\n";
    out << "            auto assignment = ExternalSystemMetadataMappingAssignment{\n";
    out << "                mapping.source, source_root, source_field, \"request\",\n";
    out << "                mapping.target.substr(request_prefix.size()),\n";
    out << "            };\n";
    out << "            plan.all_mappings.push_back(assignment);\n";
    out << "            plan.request_mappings.push_back(std::move(assignment));\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return plan;\n";
    out << "}\n\n";

    out << "inline std::optional<statespec::backend::ExternalSystemMetadataLookup> "
           "external_system_metadata_lookup(\n";
    out << "    const ExternalSystemDescriptor& descriptor,\n";
    out << "    std::vector<statespec::backend::ExternalSystemMetadataKeyValue> key_values\n";
    out << ")\n";
    out << "{\n";
    out << "    if (!descriptor.metadata.has_value())\n";
    out << "    {\n";
    out << "        return std::nullopt;\n";
    out << "    }\n";
    out << "    const auto& metadata = *descriptor.metadata;\n";
    out << "    return statespec::backend::ExternalSystemMetadataLookup{\n";
    out << "        descriptor.name,\n";
    out << "        metadata.entity,\n";
    out << "        metadata.tenant_field,\n";
    out << "        metadata.profile_field,\n";
    out << "        metadata.key_fields,\n";
    out << "        std::move(key_values),\n";
    out << "        metadata.required_fields,\n";
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::optional<statespec::backend::ExternalSystemMetadataLookup> "
           "external_system_metadata_lookup(\n";
    out << "    std::string_view external_system,\n";
    out << "    std::vector<statespec::backend::ExternalSystemMetadataKeyValue> key_values\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& descriptor : external_system_descriptors())\n";
    out << "    {\n";
    out << "        if (descriptor.name == external_system)\n";
    out << "        {\n";
    out << "            return external_system_metadata_lookup(descriptor, "
           "std::move(key_values));\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return std::nullopt;\n";
    out << "}\n\n";

    out << "inline std::optional<statespec::backend::ExternalSystemMetadataResolution> "
           "resolve_external_system_metadataTx(\n";
    out << "    statespec::backend::IExternalSystemMetadataResolver& resolver,\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    const ExternalSystemDescriptor& descriptor,\n";
    out << "    std::vector<statespec::backend::ExternalSystemMetadataKeyValue> key_values\n";
    out << ")\n";
    out << "{\n";
    out << "    auto lookup = external_system_metadata_lookup(descriptor, "
           "std::move(key_values));\n";
    out << "    if (!lookup.has_value())\n";
    out << "    {\n";
    out << "        return std::nullopt;\n";
    out << "    }\n";
    out << "    if (!lookup->key_complete())\n";
    out << "    {\n";
    out << "        return std::nullopt;\n";
    out << "    }\n";
    out << "    return resolver.resolve_metadataTx(tx, *lookup);\n";
    out << "}\n\n";

    out << "inline std::optional<statespec::backend::ExternalSystemMetadataResolution> "
           "resolve_external_system_metadataTx(\n";
    out << "    statespec::backend::IExternalSystemMetadataResolver& resolver,\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    std::string_view external_system,\n";
    out << "    std::vector<statespec::backend::ExternalSystemMetadataKeyValue> key_values\n";
    out << ")\n";
    out << "{\n";
    out << "    auto lookup = external_system_metadata_lookup(external_system, "
           "std::move(key_values));\n";
    out << "    if (!lookup.has_value())\n";
    out << "    {\n";
    out << "        return std::nullopt;\n";
    out << "    }\n";
    out << "    if (!lookup->key_complete())\n";
    out << "    {\n";
    out << "        return std::nullopt;\n";
    out << "    }\n";
    out << "    return resolver.resolve_metadataTx(tx, *lookup);\n";
    out << "}\n\n";

    out << external_system_call_adapters << "\n";

    return out.str();
}

} // namespace statespec
