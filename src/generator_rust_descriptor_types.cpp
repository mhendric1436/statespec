#include "generator_rust_artifacts.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_shape_types_file()
{
    return "use crate::backend::FieldDescriptor;\n\n"
           "#[derive(Debug, Clone)]\n"
           "pub struct ShapeDescriptor {\n"
           "    pub name: String,\n"
           "    pub fields: Vec<FieldDescriptor>,\n"
           "}\n";
}

std::string generate_rust_descriptor_types_file()
{
    return "use crate::json::Json;\n\n"
           "#[derive(Debug, Clone)]\n"
           "pub struct ApiDescriptor {\n"
           "    pub name: String,\n"
           "    pub method: Option<String>,\n"
           "    pub path: Option<String>,\n"
           "    pub input: Option<String>,\n"
           "    pub output: Option<String>,\n"
           "    pub error: Option<String>,\n"
           "    pub starts_workflow: Option<String>,\n"
           "    pub enqueues: Option<String>,\n"
           "}\n\n"
           "#[derive(Debug, Clone)]\n"
           "pub struct ApiServerDescriptor {\n"
           "    pub name: String,\n"
           "    pub serves: Vec<String>,\n"
           "    pub concurrency: i32,\n"
           "}\n\n"
           "#[derive(Debug, Clone)]\n"
           "pub struct ApiRouteDescriptor {\n"
           "    pub name: String,\n"
           "    pub server_name: String,\n"
           "    pub api_name: String,\n"
           "    pub method: Option<String>,\n"
           "    pub path: Option<String>,\n"
           "    pub input: Option<String>,\n"
           "    pub output: Option<String>,\n"
           "    pub error: Option<String>,\n"
           "}\n\n"
           "#[derive(Debug, Clone)]\n"
           "pub struct ApiRequestContext {\n"
           "    pub server_name: String,\n"
           "    pub api_name: String,\n"
           "    pub method: Option<String>,\n"
           "    pub path: Option<String>,\n"
           "    pub body: Json,\n"
           "}\n\n"
           "#[derive(Debug, Clone)]\n"
           "pub struct ApiResponse {\n"
           "    pub status_code: i32,\n"
           "    pub body: Json,\n"
           "}\n\n"
           "#[derive(Debug, Clone)]\n"
           "pub struct WorkerDescriptor {\n"
           "    pub name: String,\n"
           "    pub singleton: bool,\n"
           "    pub lease: Option<String>,\n"
           "    pub polls: Option<String>,\n"
           "    pub executes: Option<String>,\n"
           "    pub concurrency: i32,\n"
           "}\n\n"
           "#[derive(Debug, Clone)]\n"
           "pub struct WorkerContext {\n"
           "    pub worker_name: String,\n"
           "    pub singleton: bool,\n"
           "    pub lease: Option<String>,\n"
           "    pub polls: Option<String>,\n"
           "    pub executes: Option<String>,\n"
           "    pub concurrency: i32,\n"
           "}\n";
}

std::string generate_rust_backend_descriptor_types(
    const std::string& external_system_runtime,
    const std::string& external_system_metadata_runtime,
    const std::string& entity_repository_runtime
)
{
    std::ostringstream out;
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ValueDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub value_type: String,\n";
    out << "    pub constraint: Option<String>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EnumMemberDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub value: Option<String>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EnumDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub members: Vec<EnumMemberDescriptor>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EventDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub fields: Vec<FieldDescriptor>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ExternalSystemPropertyDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub value: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ExternalSystemMetadataMappingDescriptor {\n";
    out << "    pub source: String,\n";
    out << "    pub target: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ExternalSystemMetadataMappingAssignment {\n";
    out << "    pub source: String,\n";
    out << "    pub source_root: String,\n";
    out << "    pub source_field: String,\n";
    out << "    pub target_root: String,\n";
    out << "    pub field: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone, Default)]\n";
    out << "pub struct ExternalSystemMetadataMappingPlan {\n";
    out << "    pub all_mappings: Vec<ExternalSystemMetadataMappingAssignment>,\n";
    out << "    pub client_mappings: Vec<ExternalSystemMetadataMappingAssignment>,\n";
    out << "    pub request_mappings: Vec<ExternalSystemMetadataMappingAssignment>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ExternalSystemMetadataMissingMappingSource {\n";
    out << "    pub source: String,\n";
    out << "    pub source_root: String,\n";
    out << "    pub source_field: String,\n";
    out << "    pub target_root: String,\n";
    out << "    pub field: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone, Default)]\n";
    out << "pub struct ExternalSystemMetadataMappingInputs {\n";
    out << "    pub input: BTreeMap<String, Json>,\n";
    out << "    pub entity: BTreeMap<String, Json>,\n";
    out << "    pub workflow: BTreeMap<String, Json>,\n";
    out << "    pub metadata: BTreeMap<String, Json>,\n";
    out << "}\n\n";
    out << "impl ExternalSystemMetadataMappingInputs {\n";
    out << "    pub fn source_value(&self, source_root: &str, source_field: &str) -> "
           "Option<&Json> {\n";
    out << "        match source_root {\n";
    out << "            \"input\" => self.input.get(source_field),\n";
    out << "            \"entity\" => self.entity.get(source_field),\n";
    out << "            \"workflow\" => self.workflow.get(source_field),\n";
    out << "            \"metadata\" => self.metadata.get(source_field),\n";
    out << "            _ => None,\n";
    out << "        }\n";
    out << "    }\n\n";
    out << "    pub fn assignment_value(\n";
    out << "        &self,\n";
    out << "        assignment: &ExternalSystemMetadataMappingAssignment,\n";
    out << "    ) -> Option<&Json> {\n";
    out << "        self.source_value(&assignment.source_root, &assignment.source_field)\n";
    out << "    }\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone, Default)]\n";
    out << "pub struct ExternalSystemMetadataMappingOutput {\n";
    out << "    pub client_config: BTreeMap<String, Json>,\n";
    out << "    pub request_payload: BTreeMap<String, Json>,\n";
    out << "    pub missing_sources: Vec<ExternalSystemMetadataMissingMappingSource>,\n";
    out << "}\n\n";
    out << "pub fn missing_external_system_metadata_mapping_sources(\n";
    out << "    plan: &ExternalSystemMetadataMappingPlan,\n";
    out << "    inputs: &ExternalSystemMetadataMappingInputs,\n";
    out << ") -> Vec<ExternalSystemMetadataMissingMappingSource> {\n";
    out << "    plan.all_mappings\n";
    out << "        .iter()\n";
    out << "        .filter(|assignment| inputs.assignment_value(assignment).is_none())\n";
    out << "        .map(|assignment| ExternalSystemMetadataMissingMappingSource {\n";
    out << "            source: assignment.source.clone(),\n";
    out << "            source_root: assignment.source_root.clone(),\n";
    out << "            source_field: assignment.source_field.clone(),\n";
    out << "            target_root: assignment.target_root.clone(),\n";
    out << "            field: assignment.field.clone(),\n";
    out << "        })\n";
    out << "        .collect()\n";
    out << "}\n\n";
    out << "pub trait ExternalSystemMetadataMappingApplicator {\n";
    out << "    fn apply_external_system_metadata_mappings(\n";
    out << "        &self,\n";
    out << "        plan: &ExternalSystemMetadataMappingPlan,\n";
    out << "        inputs: &ExternalSystemMetadataMappingInputs,\n";
    out << "    ) -> BackendResult<ExternalSystemMetadataMappingOutput>;\n";
    out << "}\n\n";
    out << external_system_runtime << "\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ExternalSystemMetadataDescriptor {\n";
    out << "    pub entity: String,\n";
    out << "    pub tenant_field: Option<String>,\n";
    out << "    pub profile_field: String,\n";
    out << "    pub key_fields: Vec<String>,\n";
    out << "    pub required_fields: Vec<String>,\n";
    out << "    pub mappings: Vec<ExternalSystemMetadataMappingDescriptor>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ExternalSystemOperatorMetadataUpsertRequest {\n";
    out << "    pub lookup: ExternalSystemMetadataLookup,\n";
    out << "    pub document: Json,\n";
    out << "    pub expected_version: Option<u64>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ExternalSystemOperatorMetadataGetRequest {\n";
    out << "    pub lookup: ExternalSystemMetadataLookup,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ExternalSystemOperatorMetadataDisableRequest {\n";
    out << "    pub lookup: ExternalSystemMetadataLookup,\n";
    out << "    pub expected_version: Option<u64>,\n";
    out << "    pub disabled_status: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ExternalSystemOperatorMetadataDeleteRequest {\n";
    out << "    pub lookup: ExternalSystemMetadataLookup,\n";
    out << "    pub expected_version: Option<u64>,\n";
    out << "    pub deleted_status: String,\n";
    out << "}\n\n";
    out << "pub trait ExternalSystemOperatorMetadataRepository<B: Backend> {\n";
    out << "    fn upsert_metadata_tx(\n";
    out << "        &self,\n";
    out << "        tx: &mut B::Tx,\n";
    out << "        request: &ExternalSystemOperatorMetadataUpsertRequest,\n";
    out << "    ) -> BackendResult<Option<VersionedRecord>>;\n\n";
    out << "    fn get_metadata_tx(\n";
    out << "        &self,\n";
    out << "        tx: &mut B::Tx,\n";
    out << "        request: &ExternalSystemOperatorMetadataGetRequest,\n";
    out << "    ) -> BackendResult<Option<VersionedRecord>>;\n\n";
    out << "    fn disable_metadata_tx(\n";
    out << "        &self,\n";
    out << "        tx: &mut B::Tx,\n";
    out << "        request: &ExternalSystemOperatorMetadataDisableRequest,\n";
    out << "    ) -> BackendResult<Option<VersionedRecord>>;\n\n";
    out << "    fn delete_metadata_tx(\n";
    out << "        &self,\n";
    out << "        tx: &mut B::Tx,\n";
    out << "        request: &ExternalSystemOperatorMetadataDeleteRequest,\n";
    out << "    ) -> BackendResult<Option<VersionedRecord>>;\n";
    out << "}\n\n";
    out << external_system_metadata_runtime << "\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ExternalSystemDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub properties: Vec<ExternalSystemPropertyDescriptor>,\n";
    out << "    pub metadata: Option<ExternalSystemMetadataDescriptor>,\n";
    out << "}\n\n";
    out << "pub trait ExternalSystemOperatorMetadataApiHandler<B: Backend> {\n";
    out << "    fn handle_upsert_metadata_tx<R: ExternalSystemOperatorMetadataRepository<B>>(\n";
    out << "        &self,\n";
    out << "        tx: &mut B::Tx,\n";
    out << "        repository: &R,\n";
    out << "        request: &ExternalSystemOperatorMetadataUpsertRequest,\n";
    out << "    ) -> BackendResult<ApiResponse>;\n\n";
    out << "    fn handle_get_metadata_tx<R: ExternalSystemOperatorMetadataRepository<B>>(\n";
    out << "        &self,\n";
    out << "        tx: &mut B::Tx,\n";
    out << "        repository: &R,\n";
    out << "        request: &ExternalSystemOperatorMetadataGetRequest,\n";
    out << "    ) -> BackendResult<ApiResponse>;\n\n";
    out << "    fn handle_disable_metadata_tx<R: ExternalSystemOperatorMetadataRepository<B>>(\n";
    out << "        &self,\n";
    out << "        tx: &mut B::Tx,\n";
    out << "        repository: &R,\n";
    out << "        request: &ExternalSystemOperatorMetadataDisableRequest,\n";
    out << "    ) -> BackendResult<ApiResponse>;\n\n";
    out << "    fn handle_delete_metadata_tx<R: ExternalSystemOperatorMetadataRepository<B>>(\n";
    out << "        &self,\n";
    out << "        tx: &mut B::Tx,\n";
    out << "        repository: &R,\n";
    out << "        request: &ExternalSystemOperatorMetadataDeleteRequest,\n";
    out << "    ) -> BackendResult<ApiResponse>;\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct PolicyRuleDescriptor {\n";
    out << "    pub action: String,\n";
    out << "    pub condition: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct QuotaDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub expression: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct PolicyDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub tenant_scoped_by: Option<String>,\n";
    out << "    pub allows: Vec<PolicyRuleDescriptor>,\n";
    out << "    pub denies: Vec<PolicyRuleDescriptor>,\n";
    out << "    pub quotas: Vec<QuotaDescriptor>,\n";
    out << "    pub audits: Vec<String>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct GarbageCollectionPolicy {\n";
    out << "    pub after: String,\n";
    out << "    pub mode: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EntityStateDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub terminal: bool,\n";
    out << "    pub garbage_collection: Option<GarbageCollectionPolicy>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EntityOwnershipDescriptor {\n";
    out << "    pub authority: String,\n";
    out << "    pub system_of_record: String,\n";
    out << "    pub lifecycle: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EntityRelationDescriptor {\n";
    out << "    pub kind: String,\n";
    out << "    pub name: String,\n";
    out << "    pub target: String,\n";
    out << "    pub optional: bool,\n";
    out << "    pub relation_kind: Option<String>,\n";
    out << "    pub on_parent_delete: Option<String>,\n";
    out << "    pub parent_must_be_in: Vec<String>,\n";
    out << "    pub unique_within_parent: Vec<String>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EntityChildDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub target_entity: String,\n";
    out << "    pub relation: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EntityInvariantDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub expression: String,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EntityDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub key_fields: Vec<String>,\n";
    out << "    pub ownership: Option<EntityOwnershipDescriptor>,\n";
    out << "    pub relations: Vec<EntityRelationDescriptor>,\n";
    out << "    pub children: Vec<EntityChildDescriptor>,\n";
    out << "    pub invariants: Vec<EntityInvariantDescriptor>,\n";
    out << "    pub states: Vec<EntityStateDescriptor>,\n";
    out << "    pub initial_state: Option<String>,\n";
    out << "    pub terminal_states: Vec<String>,\n";
    out << "}\n\n";

    out << entity_repository_runtime << "\n";

    return out.str();
}

} // namespace statespec
