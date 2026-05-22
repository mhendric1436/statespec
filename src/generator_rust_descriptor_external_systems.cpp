#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_external_system_descriptors(
    const IrSystem& system,
    const std::string& external_system_call_adapters
)
{
    std::ostringstream out;
    out << "pub fn external_system_descriptors() -> Vec<ExternalSystemDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& external_system : system.external_systems)
    {
        out << "        ExternalSystemDescriptor {\n";
        out << "            name: " << rust_string(external_system.name) << ".to_string(),\n";
        out << "            properties: vec![\n";
        for (const auto& property : external_system.properties)
        {
            out << "                ExternalSystemPropertyDescriptor { name: "
                << rust_string(property.name)
                << ".to_string(), value: " << rust_string(property.value) << ".to_string() },\n";
        }
        out << "            ],\n";
        if (external_system.metadata.has_value())
        {
            out << "            metadata: Some(ExternalSystemMetadataDescriptor {\n";
            out << "                entity: " << rust_string(external_system.metadata->entity)
                << ".to_string(),\n";
            if (external_system.metadata->tenant_field.has_value())
            {
                out << "                tenant_field: Some("
                    << rust_string(*external_system.metadata->tenant_field) << ".to_string()),\n";
            }
            else
            {
                out << "                tenant_field: None,\n";
            }
            out << "                profile_field: "
                << rust_string(external_system.metadata->profile_field) << ".to_string(),\n";
            out << "                key_fields: vec![";
            for (std::size_t i = 0; i < external_system.metadata->key_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(external_system.metadata->key_fields[i]) << ".to_string()";
            }
            out << "],\n";
            out << "                required_fields: vec![";
            for (std::size_t i = 0; i < external_system.metadata->required_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(external_system.metadata->required_fields[i]) << ".to_string()";
            }
            out << "],\n";
            out << "                mappings: vec![";
            for (std::size_t i = 0; i < external_system.metadata->mappings.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                const auto& mapping = external_system.metadata->mappings[i];
                out << "ExternalSystemMetadataMappingDescriptor { source: "
                    << rust_string(mapping.source)
                    << ".to_string(), target: " << rust_string(mapping.target) << ".to_string() }";
            }
            out << "],\n";
            out << "            }),\n";
        }
        else
        {
            out << "            metadata: None,\n";
        }
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn external_system_metadata_lookup(\n";
    out << "    descriptor: &ExternalSystemDescriptor,\n";
    out << "    key_values: Vec<ExternalSystemMetadataKeyValue>,\n";
    out << ") -> Option<ExternalSystemMetadataLookup> {\n";
    out << "    descriptor.metadata.as_ref().map(|metadata| ExternalSystemMetadataLookup {\n";
    out << "        external_system: descriptor.name.clone(),\n";
    out << "        metadata_entity: metadata.entity.clone(),\n";
    out << "        tenant_field: metadata.tenant_field.clone(),\n";
    out << "        profile_field: metadata.profile_field.clone(),\n";
    out << "        key_fields: metadata.key_fields.clone(),\n";
    out << "        key_values,\n";
    out << "        required_fields: metadata.required_fields.clone(),\n";
    out << "    })\n";
    out << "}\n\n";

    out << "pub fn external_system_metadata_mapping_plan(\n";
    out << "    descriptor: &ExternalSystemDescriptor,\n";
    out << ") -> ExternalSystemMetadataMappingPlan {\n";
    out << "    let mut plan = ExternalSystemMetadataMappingPlan::default();\n";
    out << "    let Some(metadata) = descriptor.metadata.as_ref() else { return plan; };\n";
    out << "    for mapping in &metadata.mappings {\n";
    out << "        let (source_root, source_field) = mapping\n";
    out << "            .source\n";
    out << "            .split_once('.')\n";
    out << "            .map_or((mapping.source.as_str(), \"\"), |(root, field)| (root, field));\n";
    out << "        if let Some(field) = mapping.target.strip_prefix(\"client.\") {\n";
    out << "            plan.client_mappings.push(ExternalSystemMetadataMappingAssignment {\n";
    out << "                source: mapping.source.clone(),\n";
    out << "                source_root: source_root.to_string(),\n";
    out << "                source_field: source_field.to_string(),\n";
    out << "                target_root: \"client\".to_string(),\n";
    out << "                field: field.to_string(),\n";
    out << "            });\n";
    out << "            plan.all_mappings.push(plan.client_mappings.last().unwrap().clone());\n";
    out << "        } else if let Some(field) = mapping.target.strip_prefix(\"request.\") {\n";
    out << "            plan.request_mappings.push(ExternalSystemMetadataMappingAssignment {\n";
    out << "                source: mapping.source.clone(),\n";
    out << "                source_root: source_root.to_string(),\n";
    out << "                source_field: source_field.to_string(),\n";
    out << "                target_root: \"request\".to_string(),\n";
    out << "                field: field.to_string(),\n";
    out << "            });\n";
    out << "            plan.all_mappings.push(plan.request_mappings.last().unwrap().clone());\n";
    out << "        }\n";
    out << "    }\n";
    out << "    plan\n";
    out << "}\n\n";

    out << "pub fn external_system_metadata_lookup_by_name(\n";
    out << "    external_system: &str,\n";
    out << "    key_values: Vec<ExternalSystemMetadataKeyValue>,\n";
    out << ") -> Option<ExternalSystemMetadataLookup> {\n";
    out << "    external_system_descriptors()\n";
    out << "        .iter()\n";
    out << "        .find(|descriptor| descriptor.name == external_system)\n";
    out << "        .and_then(|descriptor| external_system_metadata_lookup(descriptor, "
           "key_values))\n";
    out << "}\n\n";

    out << "pub fn resolve_external_system_metadata_tx<B: Backend, R: "
           "ExternalSystemMetadataResolver<B>>(\n";
    out << "    resolver: &R,\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    descriptor: &ExternalSystemDescriptor,\n";
    out << "    key_values: Vec<ExternalSystemMetadataKeyValue>,\n";
    out << ") -> BackendResult<Option<ExternalSystemMetadataResolution>> {\n";
    out << "    match external_system_metadata_lookup(descriptor, key_values) {\n";
    out << "        Some(lookup) if lookup.key_complete() => resolver.resolve_metadata_tx(tx, "
           "&lookup),\n";
    out << "        Some(_) => Ok(None),\n";
    out << "        None => Ok(None),\n";
    out << "    }\n";
    out << "}\n\n";

    out << "pub fn resolve_external_system_metadata_by_name_tx<B: Backend, R: "
           "ExternalSystemMetadataResolver<B>>(\n";
    out << "    resolver: &R,\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    external_system: &str,\n";
    out << "    key_values: Vec<ExternalSystemMetadataKeyValue>,\n";
    out << ") -> BackendResult<Option<ExternalSystemMetadataResolution>> {\n";
    out << "    match external_system_metadata_lookup_by_name(external_system, key_values) {\n";
    out << "        Some(lookup) if lookup.key_complete() => resolver.resolve_metadata_tx(tx, "
           "&lookup),\n";
    out << "        Some(_) => Ok(None),\n";
    out << "        None => Ok(None),\n";
    out << "    }\n";
    out << "}\n\n";

    out << external_system_call_adapters << "\n";

    return out.str();
}

} // namespace statespec
