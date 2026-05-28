#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

namespace
{

std::string cpp_descriptor_module_includes(const IrSystem& system)
{
    std::ostringstream out;
    out << "#include \"descriptors/core.hpp\"\n";
    out << "#include \"descriptors/runtime.hpp\"\n";
    for (const auto& entity : system.entities)
    {
        out << "#include \"entities/" << snake_identifier(entity.name) << "/schema.hpp\"\n";
    }
    for (const auto& workflow : system.workflows)
    {
        out << "#include \"workflows/" << snake_identifier(workflow.name) << ".hpp\"\n";
    }
    return out.str();
}

} // namespace

std::string generate_cpp_descriptor_prelude(
    const IrSystem& system,
    const std::string& external_system_runtime,
    const std::string& external_system_metadata_runtime,
    const std::string& entity_repository_runtime
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
    out << "#include \"shape_types.hpp\"\n";
    out << "#include \"descriptors/types.hpp\"\n\n";
    out << cpp_descriptor_module_includes(system) << "\n";
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
    out << "#include \"descriptors/events.hpp\"\n\n";
    out << "#ifndef STATESPEC_GENERATED_DESCRIPTOR_TYPES_DEFINED\n\n";

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

    out << external_system_metadata_runtime << "\n";

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

    out << "#endif // STATESPEC_GENERATED_DESCRIPTOR_TYPES_DEFINED\n\n";
    out << entity_repository_runtime << "\n";

    return out.str();
}

} // namespace statespec
