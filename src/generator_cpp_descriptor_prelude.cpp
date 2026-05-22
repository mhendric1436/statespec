#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_descriptor_prelude(
    const IrSystem& system,
    const std::string& external_system_runtime
)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"backend.hpp\"\n";
    out << "#include \"external_system.hpp\"\n";
    out << "#include \"feature_flag.hpp\"\n";
    out << "#include \"lease.hpp\"\n";
    out << "#include \"log.hpp\"\n";
    out << "#include \"metric.hpp\"\n";
    out << "#include \"queue.hpp\"\n";
    out << "#include \"workflow.hpp\"\n\n";
    out << "#include <chrono>\n";
    out << "#include <cstdint>\n";
    out << "#include <map>\n";
    out << "#include <optional>\n";
    out << "#include <string>\n";
    out << "#include <string_view>\n";
    out << "#include <utility>\n";
    out << "#include <vector>\n\n";
    out << "namespace statespec_generated\n";
    out << "{\n\n";
    out << "struct EventEnvelope\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::map<std::string, statespec::backend::Json> fields;\n";
    out << "};\n\n";
    for (const auto& shape : system.shapes)
    {
        out << "struct " << pascal_identifier(shape.name) << "\n";
        out << "{\n";
        for (const auto& field : shape.fields)
        {
            out << "    " << cpp_shape_type(field.type) << " " << field.name << "{};\n";
        }
        out << "};\n\n";
    }

    for (const auto& event : system.events)
    {
        out << "inline EventEnvelope make_" << snake_identifier(event.name) << "_event(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "    statespec::backend::Json " << field.name;
            out << (i + 1 < event.fields.size() ? ",\n" : "\n");
        }
        out << ")\n";
        out << "{\n";
        out << "    return EventEnvelope{\n";
        out << "        " << cpp_string(event.name) << ",\n";
        out << "        {\n";
        for (const auto& field : event.fields)
        {
            out << "            {" << cpp_string(field.name) << ", std::move(" << field.name
                << ")},\n";
        }
        out << "        },\n";
        out << "    };\n";
        out << "}\n\n";
    }

    out << "struct LeaseDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> resource;\n";
    out << "    std::chrono::seconds ttl;\n";
    out << "    std::optional<std::chrono::seconds> renew_every;\n";
    out << "    std::optional<std::string> holder;\n";
    out << "    bool fencing_token = false;\n";
    out << "    std::optional<std::chrono::seconds> max_ttl;\n";
    out << "};\n\n";
    out << "struct FeatureFlagDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string type;\n";
    out << "    std::string default_value;\n";
    out << "    std::string scope;\n";
    out << "    std::optional<std::string> owner;\n";
    out << "    std::optional<std::string> description;\n";
    out << "    std::optional<std::string> expires;\n";
    out << "};\n\n";

    out << "struct ValueDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string type;\n";
    out << "    std::optional<std::string> constraint;\n";
    out << "};\n\n";

    out << "struct EnumMemberDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> value;\n";
    out << "};\n\n";

    out << "struct EnumDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<EnumMemberDescriptor> members;\n";
    out << "};\n\n";

    out << "struct EventDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> fields;\n";
    out << "};\n\n";

    out << "struct ExternalSystemPropertyDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string value;\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataMappingDescriptor\n";
    out << "{\n";
    out << "    std::string source;\n";
    out << "    std::string target;\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataMappingAssignment\n";
    out << "{\n";
    out << "    std::string source;\n";
    out << "    std::string source_root;\n";
    out << "    std::string source_field;\n";
    out << "    std::string target_root;\n";
    out << "    std::string field;\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataMappingPlan\n";
    out << "{\n";
    out << "    std::vector<ExternalSystemMetadataMappingAssignment> all_mappings;\n";
    out << "    std::vector<ExternalSystemMetadataMappingAssignment> client_mappings;\n";
    out << "    std::vector<ExternalSystemMetadataMappingAssignment> request_mappings;\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataMissingMappingSource\n";
    out << "{\n";
    out << "    std::string source;\n";
    out << "    std::string source_root;\n";
    out << "    std::string source_field;\n";
    out << "    std::string target_root;\n";
    out << "    std::string field;\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataMappingInputs\n";
    out << "{\n";
    out << "    std::map<std::string, statespec::backend::Json> input;\n";
    out << "    std::map<std::string, statespec::backend::Json> entity;\n";
    out << "    std::map<std::string, statespec::backend::Json> workflow;\n";
    out << "    std::map<std::string, statespec::backend::Json> metadata;\n";
    out << "\n";
    out << "    const statespec::backend::Json* source_value(\n";
    out << "        std::string_view source_root,\n";
    out << "        std::string_view source_field\n";
    out << "    ) const\n";
    out << "    {\n";
    out << "        const auto* values = source_root == \"input\" ? &input\n";
    out << "            : source_root == \"entity\"                 ? &entity\n";
    out << "            : source_root == \"workflow\"               ? &workflow\n";
    out << "            : source_root == \"metadata\"               ? &metadata\n";
    out << "                                                       : nullptr;\n";
    out << "        if (values == nullptr)\n";
    out << "        {\n";
    out << "            return nullptr;\n";
    out << "        }\n";
    out << "        const auto value = values->find(std::string(source_field));\n";
    out << "        return value == values->end() ? nullptr : &value->second;\n";
    out << "    }\n";
    out << "\n";
    out << "    const statespec::backend::Json* source_value(\n";
    out << "        const ExternalSystemMetadataMappingAssignment& assignment\n";
    out << "    ) const\n";
    out << "    {\n";
    out << "        return source_value(assignment.source_root, assignment.source_field);\n";
    out << "    }\n";
    out << "};\n\n";

    out << "struct ExternalSystemMetadataMappingOutput\n";
    out << "{\n";
    out << "    std::map<std::string, statespec::backend::Json> client_config;\n";
    out << "    std::map<std::string, statespec::backend::Json> request_payload;\n";
    out << "    std::vector<ExternalSystemMetadataMissingMappingSource> missing_sources;\n";
    out << "};\n\n";

    out << "inline std::vector<ExternalSystemMetadataMissingMappingSource> "
           "missing_external_system_metadata_mapping_sources(\n";
    out << "    const ExternalSystemMetadataMappingPlan& plan,\n";
    out << "    const ExternalSystemMetadataMappingInputs& inputs\n";
    out << ")\n";
    out << "{\n";
    out << "    std::vector<ExternalSystemMetadataMissingMappingSource> missing;\n";
    out << "    for (const auto& assignment : plan.all_mappings)\n";
    out << "    {\n";
    out << "        if (inputs.source_value(assignment) == nullptr)\n";
    out << "        {\n";
    out << "            missing.push_back(ExternalSystemMetadataMissingMappingSource{\n";
    out << "                assignment.source,\n";
    out << "                assignment.source_root,\n";
    out << "                assignment.source_field,\n";
    out << "                assignment.target_root,\n";
    out << "                assignment.field,\n";
    out << "            });\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return missing;\n";
    out << "}\n\n";

    out << "class IExternalSystemMetadataMappingApplicator\n";
    out << "{\n";
    out << "public:\n";
    out << "    virtual ~IExternalSystemMetadataMappingApplicator() = default;\n";
    out << "    virtual ExternalSystemMetadataMappingOutput apply(\n";
    out << "        const ExternalSystemMetadataMappingPlan& plan,\n";
    out << "        const ExternalSystemMetadataMappingInputs& inputs\n";
    out << "    ) = 0;\n";
    out << "};\n\n";

    out << external_system_runtime << "\n";

    out << "struct ExternalSystemMetadataDescriptor\n";
    out << "{\n";
    out << "    std::string entity;\n";
    out << "    std::optional<std::string> tenant_field;\n";
    out << "    std::string profile_field;\n";
    out << "    std::vector<std::string> key_fields;\n";
    out << "    std::vector<std::string> required_fields;\n";
    out << "    std::vector<ExternalSystemMetadataMappingDescriptor> mappings;\n";
    out << "};\n\n";

    out << "struct ExternalSystemOperatorMetadataUpsertRequest\n";
    out << "{\n";
    out << "    statespec::backend::ExternalSystemMetadataLookup lookup;\n";
    out << "    statespec::backend::Json document;\n";
    out << "    std::optional<statespec::backend::Version> expected_version;\n";
    out << "};\n\n";

    out << "struct ExternalSystemOperatorMetadataGetRequest\n";
    out << "{\n";
    out << "    statespec::backend::ExternalSystemMetadataLookup lookup;\n";
    out << "};\n\n";

    out << "struct ExternalSystemOperatorMetadataDisableRequest\n";
    out << "{\n";
    out << "    statespec::backend::ExternalSystemMetadataLookup lookup;\n";
    out << "    std::optional<statespec::backend::Version> expected_version;\n";
    out << "    std::string disabled_status = \"Disabled\";\n";
    out << "};\n\n";

    out << "struct ExternalSystemOperatorMetadataDeleteRequest\n";
    out << "{\n";
    out << "    statespec::backend::ExternalSystemMetadataLookup lookup;\n";
    out << "    std::optional<statespec::backend::Version> expected_version;\n";
    out << "    std::string deleted_status = \"Deleted\";\n";
    out << "};\n\n";

    out << "class IExternalSystemOperatorMetadataRepository\n";
    out << "{\n";
    out << "public:\n";
    out << "    virtual ~IExternalSystemOperatorMetadataRepository() = default;\n";
    out << "    virtual std::optional<statespec::backend::VersionedRecord> upsert_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataUpsertRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual std::optional<statespec::backend::VersionedRecord> get_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataGetRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual std::optional<statespec::backend::VersionedRecord> disable_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataDisableRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual std::optional<statespec::backend::VersionedRecord> delete_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataDeleteRequest& request\n";
    out << "    ) = 0;\n";
    out << "};\n\n";

    out << "class DefaultExternalSystemMetadataMappingApplicator final\n";
    out << "    : public IExternalSystemMetadataMappingApplicator\n";
    out << "{\n";
    out << "public:\n";
    out << "    ExternalSystemMetadataMappingOutput apply(\n";
    out << "        const ExternalSystemMetadataMappingPlan& plan,\n";
    out << "        const ExternalSystemMetadataMappingInputs& inputs\n";
    out << "    ) override\n";
    out << "    {\n";
    out << "        ExternalSystemMetadataMappingOutput output;\n";
    out << "        output.missing_sources = "
           "missing_external_system_metadata_mapping_sources(plan, inputs);\n";
    out << "        for (const auto& assignment : plan.client_mappings)\n";
    out << "        {\n";
    out << "            if (const auto* value = inputs.source_value(assignment))\n";
    out << "            {\n";
    out << "                output.client_config[assignment.field] = *value;\n";
    out << "            }\n";
    out << "        }\n";
    out << "        for (const auto& assignment : plan.request_mappings)\n";
    out << "        {\n";
    out << "            if (const auto* value = inputs.source_value(assignment))\n";
    out << "            {\n";
    out << "                output.request_payload[assignment.field] = *value;\n";
    out << "            }\n";
    out << "        }\n";
    out << "        return output;\n";
    out << "    }\n";
    out << "};\n\n";

    out << "inline std::optional<statespec::backend::Json> "
           "external_system_metadata_key_value(\n";
    out << "    const statespec::backend::ExternalSystemMetadataLookup& lookup,\n";
    out << "    std::string_view field\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& key_value : lookup.key_values)\n";
    out << "    {\n";
    out << "        if (key_value.field == field)\n";
    out << "        {\n";
    out << "            return key_value.value;\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return std::nullopt;\n";
    out << "}\n\n";

    out << "inline std::optional<statespec::backend::Key> external_system_metadata_key(\n";
    out << "    const statespec::backend::ExternalSystemMetadataLookup& lookup\n";
    out << ")\n";
    out << "{\n";
    out << "    if (!lookup.key_complete())\n";
    out << "    {\n";
    out << "        return std::nullopt;\n";
    out << "    }\n";
    out << "    std::string key;\n";
    out << "    for (const auto& key_field : lookup.key_fields)\n";
    out << "    {\n";
    out << "        const auto value = external_system_metadata_key_value(lookup, key_field);\n";
    out << "        if (!value)\n";
    out << "        {\n";
    out << "            return std::nullopt;\n";
    out << "        }\n";
    out << "        key += key_field;\n";
    out << "        key += '=';\n";
    out << "        key += value->canonical_string();\n";
    out << "        key += '\\n';\n";
    out << "    }\n";
    out << "    return key;\n";
    out << "}\n\n";

    out << "inline statespec::backend::Json external_system_metadata_document_with_keys(\n";
    out << "    const statespec::backend::Json& document,\n";
    out << "    const statespec::backend::ExternalSystemMetadataLookup& lookup\n";
    out << ")\n";
    out << "{\n";
    out << "    statespec::backend::Json::Object object = "
           "document.is_object() ? document.as_object() : statespec::backend::Json::Object{};\n";
    out << "    for (const auto& key_value : lookup.key_values)\n";
    out << "    {\n";
    out << "        object[key_value.field] = key_value.value;\n";
    out << "    }\n";
    out << "    return statespec::backend::Json::object(std::move(object));\n";
    out << "}\n\n";

    out << "class DefaultExternalSystemOperatorMetadataRepository final\n";
    out << "    : public IExternalSystemOperatorMetadataRepository,\n";
    out << "      public statespec::backend::IExternalSystemMetadataResolver\n";
    out << "{\n";
    out << "public:\n";
    out << "    std::optional<statespec::backend::VersionedRecord> upsert_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataUpsertRequest& request\n";
    out << "    ) override\n";
    out << "    {\n";
    out << "        const auto key = external_system_metadata_key(request.lookup);\n";
    out << "        if (!key)\n";
    out << "        {\n";
    out << "            return std::nullopt;\n";
    out << "        }\n";
    out << "        const auto existing = tx.get(request.lookup.metadata_entity, *key);\n";
    out << "        assert_expected_version(existing, request.expected_version);\n";
    out << "        tx.put(\n";
    out << "            request.lookup.metadata_entity,\n";
    out << "            *key,\n";
    out << "            external_system_metadata_document_with_keys(request.document, "
           "request.lookup)\n";
    out << "        );\n";
    out << "        return tx.get(request.lookup.metadata_entity, *key);\n";
    out << "    }\n\n";
    out << "    std::optional<statespec::backend::VersionedRecord> get_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataGetRequest& request\n";
    out << "    ) override\n";
    out << "    {\n";
    out << "        const auto key = external_system_metadata_key(request.lookup);\n";
    out << "        return key ? tx.get(request.lookup.metadata_entity, *key) : std::nullopt;\n";
    out << "    }\n\n";
    out << "    std::optional<statespec::backend::VersionedRecord> disable_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataDisableRequest& request\n";
    out << "    ) override\n";
    out << "    {\n";
    out << "        return update_statusTx(tx, request.lookup, request.expected_version, "
           "request.disabled_status);\n";
    out << "    }\n\n";
    out << "    std::optional<statespec::backend::VersionedRecord> delete_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataDeleteRequest& request\n";
    out << "    ) override\n";
    out << "    {\n";
    out << "        return update_statusTx(tx, request.lookup, request.expected_version, "
           "request.deleted_status);\n";
    out << "    }\n\n";
    out << "    std::optional<statespec::backend::ExternalSystemMetadataResolution> "
           "resolve_metadata(\n";
    out << "        statespec::backend::IBackend& backend,\n";
    out << "        const statespec::backend::ExternalSystemMetadataLookup& lookup\n";
    out << "    ) override\n";
    out << "    {\n";
    out << "        auto tx = backend.begin();\n";
    out << "        auto resolved = resolve_metadataTx(*tx, lookup);\n";
    out << "        backend.commit(*tx);\n";
    out << "        return resolved;\n";
    out << "    }\n\n";
    out << "    std::optional<statespec::backend::ExternalSystemMetadataResolution> "
           "resolve_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const statespec::backend::ExternalSystemMetadataLookup& lookup\n";
    out << "    ) override\n";
    out << "    {\n";
    out << "        const auto record = get_metadataTx(tx, "
           "ExternalSystemOperatorMetadataGetRequest{lookup});\n";
    out << "        if (!record)\n";
    out << "        {\n";
    out << "            return std::nullopt;\n";
    out << "        }\n";
    out << "        return statespec::backend::ExternalSystemMetadataResolution{\n";
    out << "            *record,\n";
    out << "            statespec::backend::missing_required_metadata_fields(\n";
    out << "                record->document,\n";
    out << "                lookup.required_fields\n";
    out << "            ),\n";
    out << "        };\n";
    out << "    }\n\n";
    out << "private:\n";
    out << "    static void assert_expected_version(\n";
    out << "        const std::optional<statespec::backend::VersionedRecord>& existing,\n";
    out << "        const std::optional<statespec::backend::Version>& expected_version\n";
    out << "    )\n";
    out << "    {\n";
    out << "        if (!expected_version)\n";
    out << "        {\n";
    out << "            return;\n";
    out << "        }\n";
    out << "        if (!existing || existing->version != *expected_version)\n";
    out << "        {\n";
    out << "            throw statespec::backend::ConflictError(\n";
    out << "                statespec::backend::ConflictKind::VersionConflict,\n";
    out << "                \"external system metadata version conflict\"\n";
    out << "            );\n";
    out << "        }\n";
    out << "    }\n\n";
    out << "    static std::optional<statespec::backend::VersionedRecord> update_statusTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const statespec::backend::ExternalSystemMetadataLookup& lookup,\n";
    out << "        const std::optional<statespec::backend::Version>& expected_version,\n";
    out << "        const std::string& status\n";
    out << "    )\n";
    out << "    {\n";
    out << "        const auto key = external_system_metadata_key(lookup);\n";
    out << "        if (!key)\n";
    out << "        {\n";
    out << "            return std::nullopt;\n";
    out << "        }\n";
    out << "        const auto existing = tx.get(lookup.metadata_entity, *key);\n";
    out << "        if (!existing)\n";
    out << "        {\n";
    out << "            return std::nullopt;\n";
    out << "        }\n";
    out << "        assert_expected_version(existing, expected_version);\n";
    out << "        auto document = external_system_metadata_document_with_keys(\n";
    out << "            existing->document,\n";
    out << "            lookup\n";
    out << "        );\n";
    out << "        auto object = document.as_object();\n";
    out << "        object[\"status\"] = status;\n";
    out << "        tx.put(lookup.metadata_entity, *key, "
           "statespec::backend::Json::object(std::move(object)));\n";
    out << "        return tx.get(lookup.metadata_entity, *key);\n";
    out << "    }\n";
    out << "};\n\n";

    out << "struct ExternalSystemDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<ExternalSystemPropertyDescriptor> properties;\n";
    out << "    std::optional<ExternalSystemMetadataDescriptor> metadata;\n";
    out << "};\n\n";

    out << "struct ApiDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> method;\n";
    out << "    std::optional<std::string> path;\n";
    out << "    std::optional<std::string> input;\n";
    out << "    std::optional<std::string> output;\n";
    out << "    std::optional<std::string> error;\n";
    out << "    std::optional<std::string> starts_workflow;\n";
    out << "    std::optional<std::string> enqueues;\n";
    out << "};\n\n";

    out << "struct ApiServerDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<std::string> serves;\n";
    out << "    int concurrency = 1;\n";
    out << "};\n\n";

    out << "struct ApiRouteDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string server_name;\n";
    out << "    std::string api_name;\n";
    out << "    std::optional<std::string> method;\n";
    out << "    std::optional<std::string> path;\n";
    out << "    std::optional<std::string> input;\n";
    out << "    std::optional<std::string> output;\n";
    out << "    std::optional<std::string> error;\n";
    out << "};\n\n";

    out << "struct ApiRequestContext\n";
    out << "{\n";
    out << "    std::string server_name;\n";
    out << "    std::string api_name;\n";
    out << "    std::optional<std::string> method;\n";
    out << "    std::optional<std::string> path;\n";
    out << "    statespec::backend::Json body;\n";
    out << "};\n\n";

    out << "struct ApiResponse\n";
    out << "{\n";
    out << "    int status_code = 200;\n";
    out << "    statespec::backend::Json body;\n";
    out << "};\n\n";

    out << "class IExternalSystemOperatorMetadataApiHandler\n";
    out << "{\n";
    out << "public:\n";
    out << "    virtual ~IExternalSystemOperatorMetadataApiHandler() = default;\n";
    out << "    virtual ApiResponse handle_upsert_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        IExternalSystemOperatorMetadataRepository& repository,\n";
    out << "        const ExternalSystemOperatorMetadataUpsertRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual ApiResponse handle_get_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        IExternalSystemOperatorMetadataRepository& repository,\n";
    out << "        const ExternalSystemOperatorMetadataGetRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual ApiResponse handle_disable_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        IExternalSystemOperatorMetadataRepository& repository,\n";
    out << "        const ExternalSystemOperatorMetadataDisableRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual ApiResponse handle_delete_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        IExternalSystemOperatorMetadataRepository& repository,\n";
    out << "        const ExternalSystemOperatorMetadataDeleteRequest& request\n";
    out << "    ) = 0;\n";
    out << "};\n\n";

    out << "struct WorkerDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    bool singleton = false;\n";
    out << "    std::optional<std::string> lease;\n";
    out << "    std::optional<std::string> polls;\n";
    out << "    std::optional<std::string> executes;\n";
    out << "    int concurrency = 1;\n";
    out << "};\n\n";

    out << "struct WorkerContext\n";
    out << "{\n";
    out << "    std::string worker_name;\n";
    out << "    bool singleton = false;\n";
    out << "    std::optional<std::string> lease;\n";
    out << "    std::optional<std::string> polls;\n";
    out << "    std::optional<std::string> executes;\n";
    out << "    int concurrency = 1;\n";
    out << "};\n\n";

    out << "struct PolicyRuleDescriptor\n";
    out << "{\n";
    out << "    std::string action;\n";
    out << "    std::string condition;\n";
    out << "};\n\n";

    out << "struct QuotaDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string expression;\n";
    out << "};\n\n";

    out << "struct PolicyDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> tenant_scoped_by;\n";
    out << "    std::vector<PolicyRuleDescriptor> allows;\n";
    out << "    std::vector<PolicyRuleDescriptor> denies;\n";
    out << "    std::vector<QuotaDescriptor> quotas;\n";
    out << "    std::vector<std::string> audits;\n";
    out << "};\n\n";

    out << "struct ShapeDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> fields;\n";
    out << "};\n\n";

    out << "struct LogDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string level;\n";
    out << "    std::string event_name;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> fields;\n";
    out << "};\n\n";

    out << "struct MetricDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string kind;\n";
    out << "    std::string backend_name;\n";
    out << "    std::string unit;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> labels;\n";
    out << "};\n\n";

    out << "struct GarbageCollectionPolicy\n";
    out << "{\n";
    out << "    std::string after;\n";
    out << "    std::string mode;\n";
    out << "};\n\n";

    out << "struct EntityStateDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    bool terminal = false;\n";
    out << "    std::optional<GarbageCollectionPolicy> garbage_collection;\n";
    out << "};\n\n";

    out << "struct EntityOwnershipDescriptor\n";
    out << "{\n";
    out << "    std::string authority;\n";
    out << "    std::string system_of_record;\n";
    out << "    std::string lifecycle;\n";
    out << "};\n\n";

    out << "struct EntityRelationDescriptor\n";
    out << "{\n";
    out << "    std::string kind;\n";
    out << "    std::string name;\n";
    out << "    std::string target;\n";
    out << "    bool optional = false;\n";
    out << "    std::optional<std::string> relation_kind;\n";
    out << "    std::optional<std::string> on_parent_delete;\n";
    out << "    std::vector<std::string> parent_must_be_in;\n";
    out << "    std::vector<std::string> unique_within_parent;\n";
    out << "};\n\n";

    out << "struct EntityChildDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string target_entity;\n";
    out << "    std::string relation;\n";
    out << "};\n\n";

    out << "struct EntityInvariantDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string expression;\n";
    out << "};\n\n";

    out << "struct EntityDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<std::string> key_fields;\n";
    out << "    std::optional<EntityOwnershipDescriptor> ownership;\n";
    out << "    std::vector<EntityRelationDescriptor> relations;\n";
    out << "    std::vector<EntityChildDescriptor> children;\n";
    out << "    std::vector<EntityInvariantDescriptor> invariants;\n";
    out << "    std::vector<EntityStateDescriptor> states;\n";
    out << "    std::optional<std::string> initial_state;\n";
    out << "    std::vector<std::string> terminal_states;\n";
    out << "};\n\n";

    return out.str();
}

} // namespace statespec
