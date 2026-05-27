#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>
#include <string_view>

namespace statespec
{

namespace
{

std::string rust_descriptor_module_declarations(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream out;
    out << "#[path = \"descriptors/core.rs\"]\n";
    out << "mod descriptor_core;\n";
    out << "#[path = \"descriptors/events.rs\"]\n";
    out << "mod descriptor_events;\n";
    out << "pub use descriptor_events::*;\n";
    out << "#[path = \"descriptors/external_systems.rs\"]\n";
    out << "mod descriptor_external_systems;\n";
    out << "pub use descriptor_external_systems::*;\n";
    out << "#[path = \"descriptors/shapes.rs\"]\n";
    out << "mod descriptor_shapes;\n";
    out << "pub use descriptor_shapes::*;\n";
    out << "#[path = \"descriptors/runtime.rs\"]\n";
    out << "mod descriptor_runtime;\n";
    auto add_runtime_registration_module = [&](bool used, std::string_view module_name)
    {
        if (!used)
        {
            return;
        }
        out << "#[path = \"descriptors/runtime/" << module_name << ".rs\"]\n";
        out << "mod descriptor_runtime_" << module_name << ";\n";
        out << "pub use descriptor_runtime_" << module_name << "::*;\n";
    };
    add_runtime_registration_module(usage.uses_feature_flags, "feature_flags");
    add_runtime_registration_module(usage.uses_queues, "queues");
    add_runtime_registration_module(usage.uses_leases, "leases");
    add_runtime_registration_module(usage.uses_logs, "logs");
    add_runtime_registration_module(usage.uses_metrics, "metrics");
    add_runtime_registration_module(usage.uses_logs && usage.uses_metrics, "observability");
    add_runtime_registration_module(usage.uses_workflows, "workflows");
    for (const auto& workflow : system.workflows)
    {
        out << "#[path = \"workflows/" << snake_identifier(workflow.name) << ".rs\"]\n";
        out << "mod workflow_" << snake_identifier(workflow.name) << ";\n";
    }
    return out.str();
}

} // namespace

std::string generate_rust_descriptor_prelude(
    const IrSystem& system,
    const std::string& external_system_runtime,
    const std::string& external_system_metadata_runtime,
    const std::string& entity_repository_runtime
)
{
    std::ostringstream out;
    out << rust_descriptor_module_declarations(system) << "\n";
    out << "use std::collections::BTreeMap;\n";
    out << "use std::time::Duration;\n\n";
    out << "use crate::backend::{Backend, BackendError, BackendResult, "
           "ConflictKind, FieldDescriptor, FieldType, Transaction, "
           "VersionedRecord};\n";
    out << "use crate::external_system::{ExternalSystemMetadataLookup, "
           "ExternalSystemMetadataResolution, "
           "ExternalSystemMetadataResolver};\n";
    out << "use crate::feature_flag::{FeatureFlagDefinition as RuntimeFeatureFlagDefinition, "
           "FeatureFlagScopeKind, FeatureFlagStore, FeatureFlagType, FeatureFlagValue};\n";
    out << "use crate::lease::{LeaseDefinition as RuntimeLeaseDefinition, LeaseDefinitionId, "
           "LeaseStore};\n";
    out << "use crate::json::Json;\n";
    out << "use crate::log::{LogDefinition as RuntimeLogDefinition, LogLevel, LogSink};\n";
    out << "use crate::metric::{MetricDefinition as RuntimeMetricDefinition, MetricKind, "
           "MetricSink};\n";
    out << "use crate::queue::{RegisterQueueDefinitionRequest, QueueDefinition, QueueStore};\n";
    out << "use crate::workflow::{RegisterWorkflowDefinitionRequest, WorkflowDefinition, "
           "WorkflowStore};\n\n";
    out << "pub use crate::descriptor_types::{ApiDescriptor, ApiRequestContext, ApiResponse, "
           "ApiRouteDescriptor, ApiServerDescriptor, LeaseDefinition, WorkerContext, "
           "WorkerDescriptor};\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct FeatureFlagDefinition {\n";
    out << "    pub name: String,\n";
    out << "    pub flag_type: String,\n";
    out << "    pub default_value: String,\n";
    out << "    pub scope: String,\n";
    out << "    pub owner: Option<String>,\n";
    out << "    pub description: Option<String>,\n";
    out << "    pub expires: Option<String>,\n";
    out << "}\n\n";
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
    out << "pub struct ShapeDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub fields: Vec<FieldDescriptor>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct LogDefinition {\n";
    out << "    pub name: String,\n";
    out << "    pub level: String,\n";
    out << "    pub event_name: String,\n";
    out << "    pub fields: Vec<FieldDescriptor>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct MetricDefinition {\n";
    out << "    pub name: String,\n";
    out << "    pub kind: String,\n";
    out << "    pub backend_name: String,\n";
    out << "    pub unit: String,\n";
    out << "    pub labels: Vec<FieldDescriptor>,\n";
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
