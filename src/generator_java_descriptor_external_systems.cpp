#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_external_system_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<ExternalSystemDescriptor> externalSystemDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t system_index = 0; system_index < system.external_systems.size();
         ++system_index)
    {
        const auto& external_system = system.external_systems[system_index];
        out << "            new ExternalSystemDescriptor(\n";
        out << "                " << java_string(external_system.name) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < external_system.properties.size(); ++i)
        {
            const auto& property = external_system.properties[i];
            out << "                    new ExternalSystemPropertyDescriptor("
                << java_string(property.name) << ", " << java_string(property.value) << ")";
            out << (i + 1 < external_system.properties.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        if (external_system.metadata.has_value())
        {
            out << "                Optional.of(new ExternalSystemMetadataDescriptor(\n";
            out << "                    " << java_string(external_system.metadata->entity) << ",\n";
            if (external_system.metadata->tenant_field.has_value())
            {
                out << "                    Optional.of("
                    << java_string(*external_system.metadata->tenant_field) << "),\n";
            }
            else
            {
                out << "                    Optional.empty(),\n";
            }
            out << "                    " << java_string(external_system.metadata->profile_field)
                << ",\n";
            out << "                    List.of(";
            for (std::size_t i = 0; i < external_system.metadata->key_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << java_string(external_system.metadata->key_fields[i]);
            }
            out << "),\n";
            out << "                    List.of(";
            for (std::size_t i = 0; i < external_system.metadata->required_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << java_string(external_system.metadata->required_fields[i]);
            }
            out << "),\n";
            out << "                    List.of(";
            for (std::size_t i = 0; i < external_system.metadata->mappings.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                const auto& mapping = external_system.metadata->mappings[i];
                out << "new ExternalSystemMetadataMappingDescriptor(" << java_string(mapping.source)
                    << ", " << java_string(mapping.target) << ")";
            }
            out << ")\n";
            out << "                ))\n";
        }
        else
        {
            out << "                Optional.empty()\n";
        }
        out << "            )" << (system_index + 1 < system.external_systems.size() ? "," : "")
            << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";
    return out.str();
}

std::string generate_java_external_system_metadata_runtime(const IrSystem&)
{
    std::ostringstream out;
    out << "    public static ExternalSystemMetadataMappingPlan "
           "externalSystemMetadataMappingPlan(\n";
    out << "        ExternalSystemDescriptor descriptor\n";
    out << "    ) {\n";
    out << "        java.util.ArrayList<ExternalSystemMetadataMappingAssignment> "
           "allMappings = new java.util.ArrayList<>();\n";
    out << "        java.util.ArrayList<ExternalSystemMetadataMappingAssignment> "
           "clientMappings = new java.util.ArrayList<>();\n";
    out << "        java.util.ArrayList<ExternalSystemMetadataMappingAssignment> "
           "requestMappings = new java.util.ArrayList<>();\n";
    out << "        if (descriptor.metadata().isPresent()) {\n";
    out << "            for (ExternalSystemMetadataMappingDescriptor mapping : "
           "descriptor.metadata().orElseThrow().mappings()) {\n";
    out << "                int sourceSeparator = mapping.source().indexOf('.');\n";
    out << "                String sourceRoot = sourceSeparator < 0\n";
    out << "                    ? mapping.source()\n";
    out << "                    : mapping.source().substring(0, sourceSeparator);\n";
    out << "                String sourceField = sourceSeparator < 0\n";
    out << "                    ? \"\"\n";
    out << "                    : mapping.source().substring(sourceSeparator + 1);\n";
    out << "                if (mapping.target().startsWith(\"client.\")) {\n";
    out << "                    ExternalSystemMetadataMappingAssignment assignment =\n";
    out << "                        new ExternalSystemMetadataMappingAssignment(\n";
    out << "                        mapping.source(), sourceRoot, sourceField,\n"
           "                        \"client\",\n"
           "mapping.target().substring(\"client.\".length())\n";
    out << "                    );\n";
    out << "                    allMappings.add(assignment);\n";
    out << "                    clientMappings.add(assignment);\n";
    out << "                } else if (mapping.target().startsWith(\"request.\")) {\n";
    out << "                    ExternalSystemMetadataMappingAssignment assignment =\n";
    out << "                        new ExternalSystemMetadataMappingAssignment(\n";
    out << "                        mapping.source(), sourceRoot, sourceField,\n"
           "                        \"request\",\n"
           "mapping.target().substring(\"request.\".length())\n";
    out << "                    );\n";
    out << "                    allMappings.add(assignment);\n";
    out << "                    requestMappings.add(assignment);\n";
    out << "                }\n";
    out << "            }\n";
    out << "        }\n";
    out << "        return new ExternalSystemMetadataMappingPlan(\n";
    out << "            List.copyOf(allMappings),\n";
    out << "            List.copyOf(clientMappings),\n";
    out << "            List.copyOf(requestMappings)\n";
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static Optional<ExternalSystem.MetadataLookup> "
           "externalSystemMetadataLookup(\n";
    out << "        ExternalSystemDescriptor descriptor,\n";
    out << "        List<ExternalSystem.MetadataKeyValue> keyValues\n";
    out << "    ) {\n";
    out << "        if (descriptor.metadata().isEmpty()) {\n";
    out << "            return Optional.empty();\n";
    out << "        }\n";
    out << "        ExternalSystemMetadataDescriptor metadata = "
           "descriptor.metadata().orElseThrow();\n";
    out << "        return Optional.of(new ExternalSystem.MetadataLookup(\n";
    out << "            descriptor.name(),\n";
    out << "            metadata.entity(),\n";
    out << "            metadata.tenantField(),\n";
    out << "            metadata.profileField(),\n";
    out << "            metadata.keyFields(),\n";
    out << "            keyValues,\n";
    out << "            metadata.requiredFields()\n";
    out << "        ));\n";
    out << "    }\n\n";

    out << "    public static Optional<ExternalSystem.MetadataLookup> "
           "externalSystemMetadataLookup(\n";
    out << "        String externalSystem,\n";
    out << "        List<ExternalSystem.MetadataKeyValue> keyValues\n";
    out << "    ) {\n";
    out << "        for (ExternalSystemDescriptor descriptor : "
           "ExternalSystemDescriptorModule.externalSystemDescriptors()) {\n";
    out << "            if (descriptor.name().equals(externalSystem)) {\n";
    out << "                return externalSystemMetadataLookup(descriptor, keyValues);\n";
    out << "            }\n";
    out << "        }\n";
    out << "        return Optional.empty();\n";
    out << "    }\n\n";

    out << "    public static Optional<ExternalSystem.MetadataResolution> "
           "resolveExternalSystemMetadataTx(\n";
    out << "        ExternalSystem resolver,\n";
    out << "        Backend.Transaction tx,\n";
    out << "        ExternalSystemDescriptor descriptor,\n";
    out << "        List<ExternalSystem.MetadataKeyValue> keyValues\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        Optional<ExternalSystem.MetadataLookup> lookup = "
           "externalSystemMetadataLookup(descriptor, keyValues);\n";
    out << "        if (lookup.isEmpty()) {\n";
    out << "            return Optional.empty();\n";
    out << "        }\n";
    out << "        ExternalSystem.MetadataLookup resolvedLookup = lookup.orElseThrow();\n";
    out << "        if (!resolvedLookup.keyComplete()) {\n";
    out << "            return Optional.empty();\n";
    out << "        }\n";
    out << "        return resolver.resolveMetadataTx(tx, resolvedLookup);\n";
    out << "    }\n\n";

    out << "    public static Optional<ExternalSystem.MetadataResolution> "
           "resolveExternalSystemMetadataTx(\n";
    out << "        ExternalSystem resolver,\n";
    out << "        Backend.Transaction tx,\n";
    out << "        String externalSystem,\n";
    out << "        List<ExternalSystem.MetadataKeyValue> keyValues\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        Optional<ExternalSystem.MetadataLookup> lookup = "
           "externalSystemMetadataLookup(externalSystem, keyValues);\n";
    out << "        if (lookup.isEmpty()) {\n";
    out << "            return Optional.empty();\n";
    out << "        }\n";
    out << "        ExternalSystem.MetadataLookup resolvedLookup = lookup.orElseThrow();\n";
    out << "        if (!resolvedLookup.keyComplete()) {\n";
    out << "            return Optional.empty();\n";
    out << "        }\n";
    out << "        return resolver.resolveMetadataTx(tx, resolvedLookup);\n";
    out << "    }\n\n";

    out << "    public static ExternalSystemCallRequest buildExternalSystemCallRequest(\n";
    out << "        ExternalSystemMetadataMappingApplicator applicator,\n";
    out << "        ExternalSystemDescriptor descriptor,\n";
    out << "        ExternalSystemMetadataMappingInputs inputs\n";
    out << "    ) throws Exception {\n";
    out << "        ExternalSystemMetadataMappingOutput mapped = "
           "applicator.applyExternalSystemMetadataMappings(\n";
    out << "            externalSystemMetadataMappingPlan(descriptor), inputs\n";
    out << "        );\n";
    out << "        if (!mapped.missingSources().isEmpty()) {\n";
    out << "            throw new Backend.BackendException(\n";
    out << "                \"external system mapping incomplete for \" + descriptor.name()\n";
    out << "            );\n";
    out << "        }\n";
    out << "        return new ExternalSystemCallRequest(\n";
    out << "            descriptor.name(), mapped.clientConfig(), mapped.requestPayload()\n";
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static ExternalSystemCallResponse callExternalSystem(\n";
    out << "        ExternalSystemClient client,\n";
    out << "        ExternalSystemMetadataMappingApplicator applicator,\n";
    out << "        ExternalSystemDescriptor descriptor,\n";
    out << "        ExternalSystemMetadataMappingInputs inputs\n";
    out << "    ) throws Exception {\n";
    out << "        return client.callExternalSystem(\n";
    out << "            buildExternalSystemCallRequest(applicator, descriptor, inputs)\n";
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static Optional<ExternalSystemCallResponse> callExternalSystem(\n";
    out << "        ExternalSystemClient client,\n";
    out << "        ExternalSystemMetadataMappingApplicator applicator,\n";
    out << "        String externalSystem,\n";
    out << "        ExternalSystemMetadataMappingInputs inputs\n";
    out << "    ) throws Exception {\n";
    out << "        for (ExternalSystemDescriptor descriptor : "
           "ExternalSystemDescriptorModule.externalSystemDescriptors()) {\n";
    out << "            if (descriptor.name().equals(externalSystem)) {\n";
    out << "                return Optional.of(callExternalSystem(client, applicator, descriptor, "
           "inputs));\n";
    out << "            }\n";
    out << "        }\n";
    out << "        return Optional.empty();\n";
    out << "    }\n\n";

    return out.str();
}

} // namespace statespec
