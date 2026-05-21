#include "statespec/generator_rust.hpp"
#include "statespec/template_renderer.hpp"

#include "generator_support.hpp"

#include <cctype>
#include <filesystem>
#include <sstream>
#include <string>

namespace statespec
{

namespace
{

std::string rust_string(const std::string& value)
{
    std::ostringstream out;
    out << '"';
    for (const auto ch : value)
    {
        switch (ch)
        {
        case '"':
            out << "\\\"";
            break;
        case '\\':
            out << "\\\\";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            out << ch;
            break;
        }
    }
    out << '"';
    return out.str();
}

bool is_optional_type(const std::string& type)
{
    return !type.empty() && type.back() == '?';
}

std::string strip_optional_suffix(const std::string& type)
{
    return is_optional_type(type) ? type.substr(0, type.size() - 1) : type;
}

std::string rust_field_type_expr(FieldDescriptorType type)
{
    switch (type)
    {
    case FieldDescriptorType::String:
        return "FieldType::String";
    case FieldDescriptorType::Bool:
        return "FieldType::Bool";
    case FieldDescriptorType::Int:
        return "FieldType::Int";
    case FieldDescriptorType::Int32:
        return "FieldType::Int32";
    case FieldDescriptorType::Int64:
        return "FieldType::Int64";
    case FieldDescriptorType::Long:
        return "FieldType::Long";
    case FieldDescriptorType::Double:
        return "FieldType::Double";
    case FieldDescriptorType::Decimal:
        return "FieldType::Decimal";
    case FieldDescriptorType::Json:
        return "FieldType::Json";
    case FieldDescriptorType::Timestamp:
        return "FieldType::Timestamp";
    case FieldDescriptorType::Duration:
        return "FieldType::Duration";
    case FieldDescriptorType::Uuid:
        return "FieldType::Uuid";
    case FieldDescriptorType::Named:
        return "FieldType::Named";
    case FieldDescriptorType::List:
        return "FieldType::List";
    case FieldDescriptorType::Set:
        return "FieldType::Set";
    case FieldDescriptorType::Map:
        return "FieldType::Map";
    case FieldDescriptorType::Optional:
        return "FieldType::Optional";
    case FieldDescriptorType::Reference:
        return "FieldType::Reference";
    }
    return "FieldType::Named";
}

bool is_required_descriptor_field(const std::string& type)
{
    return classify_field_descriptor_type(type) != FieldDescriptorType::Optional;
}

std::string rust_field_descriptor_expr(const IrField& field)
{
    return "FieldDescriptor { name: " + rust_string(field.name) + ".to_string(), field_type: " +
           rust_field_type_expr(classify_field_descriptor_type(field.type)) +
           ", type_name: " + rust_string(field.type) + ".to_string(), required: " +
           (is_required_descriptor_field(field.type) ? "true" : "false") + " }";
}

std::string pascal_identifier(const std::string& value)
{
    std::string result;
    bool upper_next = true;
    for (const auto ch : value)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0)
        {
            upper_next = true;
            continue;
        }
        result.push_back(
            upper_next ? static_cast<char>(std::toupper(static_cast<unsigned char>(ch))) : ch
        );
        upper_next = false;
    }
    return result.empty() ? "GeneratedShape" : result;
}

std::string snake_identifier(const std::string& value)
{
    std::string result;
    bool previous_was_separator = true;
    for (const auto ch : value)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0)
        {
            if (!result.empty() && !previous_was_separator)
            {
                result.push_back('_');
            }
            previous_was_separator = true;
            continue;
        }
        if (std::isupper(static_cast<unsigned char>(ch)) != 0 && !previous_was_separator &&
            !result.empty())
        {
            result.push_back('_');
        }
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        previous_was_separator = false;
    }
    return result.empty() ? "generated_event" : result;
}

TemplateRenderer::Values rust_makefile_values(BindingGenerationTier tier)
{
    const auto include_api =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api;
    const auto include_worker =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker;

    std::ostringstream target_additions;
    std::ostringstream phony_targets;
    std::ostringstream api_rules;
    std::ostringstream worker_rules;
    if (include_api)
    {
        target_additions << "\nCHECK_TARGETS += check-api";
        target_additions << "\nBUILD_TARGETS += build-api";
        target_additions << "\nPACKAGE_TARGETS += package-api";
        phony_targets << " check-api build-api package-api";
        api_rules << "check-api:\n";
        api_rules << "\t$(CARGO) test\n\n";
        api_rules << "build-api:\n";
        api_rules << "\t$(CARGO) build\n\n";
        api_rules << "package-api: build-api $(DIST_DIR)\n";
        api_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-api-rust.tgz common api "
                     "Cargo.toml lib.rs Makefile\n\n";
    }
    if (include_worker)
    {
        target_additions << "\nCHECK_TARGETS += check-worker";
        target_additions << "\nBUILD_TARGETS += build-worker";
        target_additions << "\nPACKAGE_TARGETS += package-worker";
        phony_targets << " check-worker build-worker package-worker";
        worker_rules << "check-worker:\n";
        worker_rules << "\t$(CARGO) test\n\n";
        worker_rules << "build-worker:\n";
        worker_rules << "\t$(CARGO) build\n\n";
        worker_rules << "package-worker: build-worker $(DIST_DIR)\n";
        worker_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-worker-rust.tgz common worker "
                        "Cargo.toml lib.rs Makefile\n\n";
    }
    return TemplateRenderer::Values{
        {"target_additions", target_additions.str()},
        {"phony_targets", phony_targets.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
    };
}

TemplateRenderer::Values rust_lib_values(BindingGenerationTier tier)
{
    std::ostringstream api_modules;
    std::ostringstream worker_modules;
    if (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api)
    {
        api_modules << "#[path = \"api/api_descriptors.rs\"]\n";
        api_modules << "pub mod api_descriptors;\n";
        api_modules << "#[path = \"api/api_dispatcher.rs\"]\n";
        api_modules << "pub mod api_dispatcher;\n";
        api_modules << "#[path = \"api/api_handlers.rs\"]\n";
        api_modules << "pub mod api_handlers;\n";
        api_modules << "#[path = \"api/api_routes.rs\"]\n";
        api_modules << "pub mod api_routes;\n";
        api_modules << "#[path = \"api/api_server.rs\"]\n";
        api_modules << "pub mod api_server;\n";
        api_modules << "#[path = \"api/external_system_operator_metadata_api.rs\"]\n";
        api_modules << "pub mod external_system_operator_metadata_api;\n";
    }
    if (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker)
    {
        worker_modules << "#[path = \"worker/worker_contexts.rs\"]\n";
        worker_modules << "pub mod worker_contexts;\n";
        worker_modules << "#[path = \"worker/worker_descriptors.rs\"]\n";
        worker_modules << "pub mod worker_descriptors;\n";
        worker_modules << "#[path = \"worker/worker_handlers.rs\"]\n";
        worker_modules << "pub mod worker_handlers;\n";
        worker_modules << "#[path = \"worker/worker_leases.rs\"]\n";
        worker_modules << "pub mod worker_leases;\n";
        worker_modules << "#[path = \"worker/worker_queues.rs\"]\n";
        worker_modules << "pub mod worker_queues;\n";
        worker_modules << "#[path = \"worker/worker_workflows.rs\"]\n";
        worker_modules << "pub mod worker_workflows;\n";
        worker_modules << "#[path = \"worker/worker_registry.rs\"]\n";
        worker_modules << "pub mod worker_registry;\n";
        worker_modules << "#[path = \"worker/worker_application.rs\"]\n";
        worker_modules << "pub mod worker_application;\n";
        worker_modules << "#[path = \"worker/workflow_step_handlers.rs\"]\n";
        worker_modules << "pub mod workflow_step_handlers;\n";
        worker_modules << "#[path = \"worker/workflow_runner.rs\"]\n";
        worker_modules << "pub mod workflow_runner;\n";
    }
    return TemplateRenderer::Values{
        {"api_modules", api_modules.str()},
        {"worker_modules", worker_modules.str()},
    };
}

std::string rust_shape_type(const std::string& type)
{
    const auto optional = is_optional_type(type);
    const auto base = strip_optional_suffix(type);
    std::string mapped = "String";
    if (base == "bool")
    {
        mapped = "bool";
    }
    else if (base == "int" || base == "int32")
    {
        mapped = "i32";
    }
    else if (base == "int64" || base == "long")
    {
        mapped = "i64";
    }
    else if (base == "double" || base == "decimal")
    {
        mapped = "f64";
    }

    return optional ? "Option<" + mapped + ">" : mapped;
}

long long parse_duration_seconds(const std::optional<std::string>& value)
{
    if (!value.has_value() || value->empty())
    {
        return 0;
    }
    const auto& text = *value;
    if (text.size() < 3 || text[0] != 'P' || text[1] != 'T')
    {
        return 0;
    }

    long long total = 0;
    long long current = 0;
    for (std::size_t i = 2; i < text.size(); ++i)
    {
        const auto ch = text[i];
        if (std::isdigit(static_cast<unsigned char>(ch)))
        {
            current = current * 10 + static_cast<long long>(ch - '0');
        }
        else
        {
            if (ch == 'H')
            {
                total += current * 3600;
            }
            else if (ch == 'M')
            {
                total += current * 60;
            }
            else if (ch == 'S')
            {
                total += current;
            }
            current = 0;
        }
    }
    return total;
}

std::string optional_string_expr(const std::optional<std::string>& value)
{
    return value.has_value() ? "Some(" + rust_string(*value) + ".to_string())" : "None";
}

const IrApi* find_api(
    const IrSystem& system,
    const std::string& name
)
{
    for (const auto& api : system.apis)
    {
        if (api.name == name)
        {
            return &api;
        }
    }
    return nullptr;
}

const std::optional<std::string>& optional_api_field(
    const IrApi* api,
    const std::optional<std::string> IrApi::* field
)
{
    static const std::optional<std::string> empty;
    return api == nullptr ? empty : api->*field;
}

std::string optional_duration_expr(const std::optional<std::string>& value)
{
    return value.has_value()
               ? "Some(Duration::from_secs(" + std::to_string(parse_duration_seconds(value)) + "))"
               : "None";
}

std::string generate_descriptors_rs(const IrSystem& system)
{
    std::ostringstream out;
    out << "use std::collections::BTreeMap;\n";
    out << "use std::time::Duration;\n\n";
    out << "use crate::backend::{Backend, BackendResult, CollectionDescriptor, FieldDescriptor, "
           "FieldType, IndexDescriptor, VersionedRecord};\n";
    out << "use crate::external_system::{ExternalSystemMetadataKeyValue, "
           "ExternalSystemMetadataLookup, ExternalSystemMetadataResolution, "
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
           "WorkflowStepDefinition, WorkflowStore};\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EventEnvelope {\n";
    out << "    pub name: String,\n";
    out << "    pub fields: BTreeMap<String, Json>,\n";
    out << "}\n\n";
    for (const auto& shape : system.shapes)
    {
        out << "#[derive(Debug, Clone)]\n";
        out << "pub struct " << pascal_identifier(shape.name) << " {\n";
        for (const auto& field : shape.fields)
        {
            out << "    pub " << field.name << ": " << rust_shape_type(field.type) << ",\n";
        }
        out << "}\n\n";
    }

    for (const auto& event : system.events)
    {
        out << "pub fn make_" << snake_identifier(event.name) << "_event(\n";
        for (const auto& field : event.fields)
        {
            out << "    " << field.name << ": Json,\n";
        }
        out << ") -> EventEnvelope {\n";
        out << "    let mut fields = BTreeMap::new();\n";
        for (const auto& field : event.fields)
        {
            out << "    fields.insert(" << rust_string(field.name) << ".to_string(), " << field.name
                << ");\n";
        }
        out << "    EventEnvelope { name: " << rust_string(event.name)
            << ".to_string(), fields }\n";
        out << "}\n\n";
    }

    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct LeaseDefinition {\n";
    out << "    pub name: String,\n";
    out << "    pub resource: Option<String>,\n";
    out << "    pub ttl: Duration,\n";
    out << "    pub renew_every: Option<Duration>,\n";
    out << "    pub holder: Option<String>,\n";
    out << "    pub fencing_token: bool,\n";
    out << "    pub max_ttl: Option<Duration>,\n";
    out << "}\n\n";
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
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ExternalSystemDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub properties: Vec<ExternalSystemPropertyDescriptor>,\n";
    out << "    pub metadata: Option<ExternalSystemMetadataDescriptor>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ApiDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub method: Option<String>,\n";
    out << "    pub path: Option<String>,\n";
    out << "    pub input: Option<String>,\n";
    out << "    pub output: Option<String>,\n";
    out << "    pub error: Option<String>,\n";
    out << "    pub starts_workflow: Option<String>,\n";
    out << "    pub enqueues: Option<String>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ApiServerDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub serves: Vec<String>,\n";
    out << "    pub concurrency: i32,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ApiRouteDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub server_name: String,\n";
    out << "    pub api_name: String,\n";
    out << "    pub method: Option<String>,\n";
    out << "    pub path: Option<String>,\n";
    out << "    pub input: Option<String>,\n";
    out << "    pub output: Option<String>,\n";
    out << "    pub error: Option<String>,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ApiRequestContext {\n";
    out << "    pub server_name: String,\n";
    out << "    pub api_name: String,\n";
    out << "    pub method: Option<String>,\n";
    out << "    pub path: Option<String>,\n";
    out << "    pub body: Json,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct ApiResponse {\n";
    out << "    pub status_code: i32,\n";
    out << "    pub body: Json,\n";
    out << "}\n\n";
    out << "pub trait ApiHandler {\n";
    out << "    fn handle(&self, context: &ApiRequestContext) -> BackendResult<ApiResponse>;\n";
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
    out << "pub struct WorkerDescriptor {\n";
    out << "    pub name: String,\n";
    out << "    pub singleton: bool,\n";
    out << "    pub lease: Option<String>,\n";
    out << "    pub polls: Option<String>,\n";
    out << "    pub executes: Option<String>,\n";
    out << "    pub concurrency: i32,\n";
    out << "}\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct WorkerContext {\n";
    out << "    pub worker_name: String,\n";
    out << "    pub singleton: bool,\n";
    out << "    pub lease: Option<String>,\n";
    out << "    pub polls: Option<String>,\n";
    out << "    pub executes: Option<String>,\n";
    out << "    pub concurrency: i32,\n";
    out << "}\n\n";
    out << "pub trait Worker {\n";
    out << "    fn run(&self, context: &WorkerContext) -> BackendResult<()>;\n";
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

    out << "pub fn feature_flag_definitions() -> Vec<FeatureFlagDefinition> {\n";
    out << "    vec![\n";
    for (const auto& flag : system.feature_flags)
    {
        out << "        FeatureFlagDefinition {\n";
        out << "            name: " << rust_string(flag.name) << ".to_string(),\n";
        out << "            flag_type: " << rust_string(flag.type) << ".to_string(),\n";
        out << "            default_value: " << rust_string(flag.default_value)
            << ".to_string(),\n";
        out << "            scope: " << rust_string(flag.scope) << ".to_string(),\n";
        out << "            owner: " << optional_string_expr(flag.owner) << ",\n";
        out << "            description: " << optional_string_expr(flag.description) << ",\n";
        out << "            expires: " << optional_string_expr(flag.expires) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn value_descriptors() -> Vec<ValueDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& value : system.values)
    {
        out << "        ValueDescriptor {\n";
        out << "            name: " << rust_string(value.name) << ".to_string(),\n";
        out << "            value_type: " << rust_string(value.type) << ".to_string(),\n";
        out << "            constraint: " << optional_string_expr(value.constraint) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn enum_descriptors() -> Vec<EnumDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& enum_decl : system.enums)
    {
        out << "        EnumDescriptor {\n";
        out << "            name: " << rust_string(enum_decl.name) << ".to_string(),\n";
        out << "            members: vec![\n";
        for (const auto& member : enum_decl.members)
        {
            out << "                EnumMemberDescriptor { name: " << rust_string(member.name)
                << ".to_string(), value: " << optional_string_expr(member.value) << " },\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn event_descriptors() -> Vec<EventDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& event : system.events)
    {
        out << "        EventDescriptor {\n";
        out << "            name: " << rust_string(event.name) << ".to_string(),\n";
        out << "            fields: vec![\n";
        for (const auto& field : event.fields)
        {
            out << "                " << rust_field_descriptor_expr(field) << ",\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

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

    out << "pub fn api_descriptors() -> Vec<ApiDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& api : system.apis)
    {
        out << "        ApiDescriptor {\n";
        out << "            name: " << rust_string(api.name) << ".to_string(),\n";
        out << "            method: " << optional_string_expr(api.method) << ",\n";
        out << "            path: " << optional_string_expr(api.path) << ",\n";
        out << "            input: " << optional_string_expr(api.input) << ",\n";
        out << "            output: " << optional_string_expr(api.output) << ",\n";
        out << "            error: " << optional_string_expr(api.error) << ",\n";
        out << "            starts_workflow: " << optional_string_expr(api.starts_workflow)
            << ",\n";
        out << "            enqueues: " << optional_string_expr(api.enqueues) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn api_server_descriptors() -> Vec<ApiServerDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "        ApiServerDescriptor {\n";
        out << "            name: " << rust_string(api_server.name) << ".to_string(),\n";
        out << "            serves: vec![";
        for (std::size_t i = 0; i < api_server.serves.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(api_server.serves[i]) << ".to_string()";
        }
        out << "],\n";
        out << "            concurrency: " << api_server.concurrency.value_or(1) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn api_route_descriptors() -> Vec<ApiRouteDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& api_server : system.api_servers)
    {
        for (const auto& api_name : api_server.serves)
        {
            const auto* api = find_api(system, api_name);
            out << "        ApiRouteDescriptor {\n";
            out << "            name: " << rust_string(api_server.name + "." + api_name)
                << ".to_string(),\n";
            out << "            server_name: " << rust_string(api_server.name) << ".to_string(),\n";
            out << "            api_name: " << rust_string(api_name) << ".to_string(),\n";
            out << "            method: "
                << optional_string_expr(optional_api_field(api, &IrApi::method)) << ",\n";
            out << "            path: "
                << optional_string_expr(optional_api_field(api, &IrApi::path)) << ",\n";
            out << "            input: "
                << optional_string_expr(optional_api_field(api, &IrApi::input)) << ",\n";
            out << "            output: "
                << optional_string_expr(optional_api_field(api, &IrApi::output)) << ",\n";
            out << "            error: "
                << optional_string_expr(optional_api_field(api, &IrApi::error)) << ",\n";
            out << "        },\n";
        }
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn worker_descriptors() -> Vec<WorkerDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& worker : system.workers)
    {
        out << "        WorkerDescriptor {\n";
        out << "            name: " << rust_string(worker.name) << ".to_string(),\n";
        out << "            singleton: " << (worker.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "            lease: " << optional_string_expr(worker.lease) << ",\n";
        out << "            polls: " << optional_string_expr(worker.polls) << ",\n";
        out << "            executes: " << optional_string_expr(worker.executes) << ",\n";
        out << "            concurrency: " << worker.concurrency.value_or(1) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn worker_contexts() -> Vec<WorkerContext> {\n";
    out << "    vec![\n";
    for (const auto& worker : system.workers)
    {
        out << "        WorkerContext {\n";
        out << "            worker_name: " << rust_string(worker.name) << ".to_string(),\n";
        out << "            singleton: " << (worker.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "            lease: " << optional_string_expr(worker.lease) << ",\n";
        out << "            polls: " << optional_string_expr(worker.polls) << ",\n";
        out << "            executes: " << optional_string_expr(worker.executes) << ",\n";
        out << "            concurrency: " << worker.concurrency.value_or(1) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn policy_descriptors() -> Vec<PolicyDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& policy : system.policies)
    {
        out << "        PolicyDescriptor {\n";
        out << "            name: " << rust_string(policy.name) << ".to_string(),\n";
        out << "            tenant_scoped_by: " << optional_string_expr(policy.tenant_scoped_by)
            << ",\n";
        out << "            allows: vec![\n";
        for (const auto& allow : policy.allows)
        {
            out << "                PolicyRuleDescriptor { action: " << rust_string(allow.action)
                << ".to_string(), condition: " << rust_string(allow.condition)
                << ".to_string() },\n";
        }
        out << "            ],\n";
        out << "            denies: vec![\n";
        for (const auto& deny : policy.denies)
        {
            out << "                PolicyRuleDescriptor { action: " << rust_string(deny.action)
                << ".to_string(), condition: " << rust_string(deny.condition)
                << ".to_string() },\n";
        }
        out << "            ],\n";
        out << "            quotas: vec![\n";
        for (const auto& quota : policy.quotas)
        {
            out << "                QuotaDescriptor { name: " << rust_string(quota.name)
                << ".to_string(), expression: " << rust_string(quota.expression)
                << ".to_string() },\n";
        }
        out << "            ],\n";
        out << "            audits: vec![";
        for (std::size_t i = 0; i < policy.audits.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(policy.audits[i]) << ".to_string()";
        }
        out << "],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn shape_descriptors() -> Vec<ShapeDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& shape : system.shapes)
    {
        out << "        ShapeDescriptor {\n";
        out << "            name: " << rust_string(shape.name) << ".to_string(),\n";
        out << "            fields: vec![\n";
        for (const auto& field : shape.fields)
        {
            out << "                " << rust_field_descriptor_expr(field) << ",\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn log_definitions() -> Vec<LogDefinition> {\n";
    out << "    vec![\n";
    for (const auto& log : system.logs)
    {
        out << "        LogDefinition {\n";
        out << "            name: " << rust_string(log.name) << ".to_string(),\n";
        out << "            level: " << rust_string(log.level) << ".to_string(),\n";
        out << "            event_name: " << rust_string(log.event_name) << ".to_string(),\n";
        out << "            fields: vec![\n";
        for (const auto& field : log.fields)
        {
            out << "                " << rust_field_descriptor_expr(field) << ",\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn metric_definitions() -> Vec<MetricDefinition> {\n";
    out << "    vec![\n";
    for (const auto& metric : system.metrics)
    {
        out << "        MetricDefinition {\n";
        out << "            name: " << rust_string(metric.name) << ".to_string(),\n";
        out << "            kind: " << rust_string(metric.kind) << ".to_string(),\n";
        out << "            backend_name: " << rust_string(metric.backend_name)
            << ".to_string(),\n";
        out << "            unit: " << rust_string(metric.unit) << ".to_string(),\n";
        out << "            labels: vec![\n";
        for (const auto& label : metric.labels)
        {
            out << "                " << rust_field_descriptor_expr(label) << ",\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn entity_descriptors() -> Vec<EntityDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& entity : system.entities)
    {
        out << "        EntityDescriptor {\n";
        out << "            name: " << rust_string(entity.name) << ".to_string(),\n";
        out << "            key_fields: vec![";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(entity.key_fields[i]) << ".to_string()";
        }
        out << "],\n";
        if (entity.ownership.has_value())
        {
            out << "            ownership: Some(EntityOwnershipDescriptor {\n";
            out << "                authority: " << rust_string(entity.ownership->authority)
                << ".to_string(),\n";
            out << "                system_of_record: "
                << rust_string(entity.ownership->system_of_record) << ".to_string(),\n";
            out << "                lifecycle: " << rust_string(entity.ownership->lifecycle)
                << ".to_string(),\n";
            out << "            }),\n";
        }
        else
        {
            out << "            ownership: None,\n";
        }
        out << "            relations: vec![\n";
        for (const auto& relation : entity.relations)
        {
            out << "                EntityRelationDescriptor {\n";
            out << "                    kind: " << rust_string(relation.kind) << ".to_string(),\n";
            out << "                    name: " << rust_string(relation.name) << ".to_string(),\n";
            out << "                    target: " << rust_string(relation.target)
                << ".to_string(),\n";
            out << "                    optional: " << (relation.optional ? "true" : "false")
                << ",\n";
            out << "                    relation_kind: "
                << optional_string_expr(relation.relation_kind) << ",\n";
            out << "                    on_parent_delete: "
                << optional_string_expr(relation.on_parent_delete) << ",\n";
            out << "                    parent_must_be_in: vec![";
            for (std::size_t i = 0; i < relation.parent_must_be_in.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(relation.parent_must_be_in[i]) << ".to_string()";
            }
            out << "],\n";
            out << "                    unique_within_parent: vec![";
            for (std::size_t i = 0; i < relation.unique_within_parent.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(relation.unique_within_parent[i]) << ".to_string()";
            }
            out << "],\n";
            out << "                },\n";
        }
        out << "            ],\n";
        out << "            children: vec![\n";
        for (const auto& child : entity.children)
        {
            out << "                EntityChildDescriptor { name: " << rust_string(child.name)
                << ".to_string(), target_entity: " << rust_string(child.target_entity)
                << ".to_string(), relation: " << rust_string(child.relation) << ".to_string() },\n";
        }
        out << "            ],\n";
        out << "            invariants: vec![\n";
        for (const auto& invariant : entity.invariants)
        {
            out << "                EntityInvariantDescriptor { name: "
                << rust_string(invariant.name)
                << ".to_string(), expression: " << rust_string(invariant.expression)
                << ".to_string() },\n";
        }
        out << "            ],\n";
        out << "            states: vec![\n";
        for (const auto& state : entity.states)
        {
            out << "                EntityStateDescriptor {\n";
            out << "                    name: " << rust_string(state.name) << ".to_string(),\n";
            out << "                    terminal: " << (state.terminal ? "true" : "false") << ",\n";
            if (state.garbage_collection.has_value())
            {
                out << "                    garbage_collection: Some(GarbageCollectionPolicy { "
                       "after: "
                    << rust_string(state.garbage_collection->after)
                    << ".to_string(), mode: " << rust_string(state.garbage_collection->mode)
                    << ".to_string() }),\n";
            }
            else
            {
                out << "                    garbage_collection: None,\n";
            }
            out << "                },\n";
        }
        out << "            ],\n";
        out << "            initial_state: " << optional_string_expr(entity.initial_state) << ",\n";
        out << "            terminal_states: vec![";
        for (std::size_t i = 0; i < entity.terminal_states.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(entity.terminal_states[i]) << ".to_string()";
        }
        out << "],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn collection_descriptors() -> Vec<CollectionDescriptor> {\n";
    out << "    vec![\n";

    for (const auto& entity : system.entities)
    {
        out << "        CollectionDescriptor {\n";
        out << "            name: " << rust_string(entity.name) << ".to_string(),\n";
        out << "            fields: vec![\n";
        for (const auto& field : entity.fields)
        {
            out << "                " << rust_field_descriptor_expr(field) << ",\n";
        }
        out << "            ],\n";
        out << "            key_fields: vec![";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(entity.key_fields[i]) << ".to_string()";
        }
        out << "],\n";
        out << "            indexes: vec![\n";
        for (const auto& index : entity.indexes)
        {
            out << "                IndexDescriptor {\n";
            out << "                    name: " << rust_string(index.name) << ".to_string(),\n";
            out << "                    fields: vec![";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(index.fields[i]) << ".to_string()";
            }
            out << "],\n";
            out << "                    unique: " << (index.unique ? "true" : "false") << ",\n";
            out << "                },\n";
        }
        out << "            ],\n";
        out << "            schema_version: 1,\n";
        out << "        },\n";
    }

    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn queue_definitions() -> Vec<QueueDefinition> {\n";
    out << "    vec![\n";
    for (const auto& queue : system.queues)
    {
        out << "        QueueDefinition {\n";
        out << "            queue: " << rust_string(queue.name) << ".to_string(),\n";
        out << "            channel: " << rust_string(queue.channel.value_or("default"))
            << ".to_string(),\n";
        out << "            visibility_timeout: Duration::from_secs("
            << parse_duration_seconds(queue.visibility_timeout) << "),\n";
        out << "            max_attempts: " << queue.max_attempts.value_or(1) << ",\n";
        out << "            dead_letter_queue: " << optional_string_expr(queue.dead_letter)
            << ",\n";
        out << "            metadata: Json::Object(BTreeMap::new()),\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn lease_definitions() -> Vec<LeaseDefinition> {\n";
    out << "    vec![\n";
    for (const auto& lease : system.leases)
    {
        out << "        LeaseDefinition {\n";
        out << "            name: " << rust_string(lease.name) << ".to_string(),\n";
        out << "            resource: " << optional_string_expr(lease.resource) << ",\n";
        out << "            ttl: Duration::from_secs(" << parse_duration_seconds(lease.ttl)
            << "),\n";
        out << "            renew_every: " << optional_duration_expr(lease.renew_every) << ",\n";
        out << "            holder: " << optional_string_expr(lease.holder) << ",\n";
        out << "            fencing_token: "
            << (lease.fencing_token.value_or(false) ? "true" : "false") << ",\n";
        out << "            max_ttl: " << optional_duration_expr(lease.max_ttl) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn workflow_definitions() -> Vec<WorkflowDefinition> {\n";
    out << "    vec![\n";
    for (const auto& workflow : system.workflows)
    {
        out << "        WorkflowDefinition {\n";
        out << "            workflow_name: " << rust_string(workflow.name) << ".to_string(),\n";
        out << "            workflow_version: " << workflow.version.value_or(1) << ",\n";
        out << "            start_step: " << rust_string(workflow.start_step.value_or(""))
            << ".to_string(),\n";
        out << "            expected_execution_time: Duration::from_secs("
            << parse_duration_seconds(workflow.expected_execution_time) << "),\n";
        out << "            singleton: " << (workflow.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "            steps: vec![\n";
        for (const auto& step : workflow.steps)
        {
            out << "                WorkflowStepDefinition { name: " << rust_string(step.name)
                << ".to_string(), expected_execution_time: Duration::from_secs("
                << parse_duration_seconds(step.expected_execution_time)
                << "), max_retries: " << step.max_retries.value_or(0) << " },\n";
        }
        out << "            ],\n";
        out << "            metadata: Json::Object(BTreeMap::new()),\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n";

    out << "\nfn log_level_from_descriptor(level: &str) -> LogLevel {\n";
    out << "    match level {\n";
    out << "        \"debug\" => LogLevel::Debug,\n";
    out << "        \"warn\" => LogLevel::Warn,\n";
    out << "        \"error\" => LogLevel::Error,\n";
    out << "        _ => LogLevel::Info,\n";
    out << "    }\n";
    out << "}\n\n";

    out << "fn metric_kind_from_descriptor(kind: &str) -> MetricKind {\n";
    out << "    match kind {\n";
    out << "        \"gauge\" => MetricKind::Gauge,\n";
    out << "        \"histogram\" => MetricKind::Histogram,\n";
    out << "        _ => MetricKind::Counter,\n";
    out << "    }\n";
    out << "}\n\n";

    out << "fn feature_flag_type_from_descriptor(flag_type: &str) -> FeatureFlagType {\n";
    out << "    match flag_type {\n";
    out << "        \"string\" => FeatureFlagType::String,\n";
    out << "        \"int\" => FeatureFlagType::Integer,\n";
    out << "        \"decimal\" => FeatureFlagType::Decimal,\n";
    out << "        _ => FeatureFlagType::Bool,\n";
    out << "    }\n";
    out << "}\n\n";

    out << "fn feature_flag_scope_from_descriptor(scope: &str) -> FeatureFlagScopeKind {\n";
    out << "    match scope {\n";
    out << "        \"system\" => FeatureFlagScopeKind::System,\n";
    out << "        \"user\" => FeatureFlagScopeKind::User,\n";
    out << "        _ if scope.starts_with(\"entity \") => FeatureFlagScopeKind::Entity,\n";
    out << "        _ => FeatureFlagScopeKind::Tenant,\n";
    out << "    }\n";
    out << "}\n\n";

    out << "fn feature_flag_value_from_descriptor(\n";
    out << "    definition: &FeatureFlagDefinition,\n";
    out << ") -> FeatureFlagValue {\n";
    out << "    match definition.flag_type.as_str() {\n";
    out << "        \"string\" => FeatureFlagValue::String(definition.default_value.clone()),\n";
    out << "        \"int\" => "
           "FeatureFlagValue::Integer(definition.default_value.parse().unwrap_or(0)),\n";
    out << "        \"decimal\" => "
           "FeatureFlagValue::Decimal(definition.default_value.parse().unwrap_or(0.0)),\n";
    out << "        _ => FeatureFlagValue::Bool(definition.default_value == \"true\"),\n";
    out << "    }\n";
    out << "}\n\n";

    out << "fn lease_definition_from_descriptor(\n";
    out << "    definition: LeaseDefinition,\n";
    out << ") -> RuntimeLeaseDefinition {\n";
    out << "    RuntimeLeaseDefinition {\n";
    out << "        id: LeaseDefinitionId { name: definition.name.clone(), version: 1 },\n";
    out << "        resource_pattern: definition.resource.unwrap_or_else(|| "
           "definition.name.clone()),\n";
    out << "        ttl: definition.ttl,\n";
    out << "        renew_every: definition.renew_every.unwrap_or(definition.ttl),\n";
    out << "        max_ttl: definition.max_ttl,\n";
    out << "        fencing_token: definition.fencing_token,\n";
    out << "    }\n";
    out << "}\n\n";

    out << "pub fn ensure_system_collections<B: Backend>(backend: &B) -> BackendResult<()> {\n";
    out << "    backend.ensure_collections(&collection_descriptors())\n";
    out << "}\n\n";

    out << "pub fn register_feature_flag_definitions_tx<B: Backend, S: FeatureFlagStore<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    store: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in feature_flag_definitions() {\n";
    out << "        store.register_definition_tx(\n";
    out << "            tx,\n";
    out << "            &RuntimeFeatureFlagDefinition {\n";
    out << "                name: definition.name.clone(),\n";
    out << "                flag_type: feature_flag_type_from_descriptor(&definition.flag_type),\n";
    out << "                default_value: feature_flag_value_from_descriptor(&definition),\n";
    out << "                scope: feature_flag_scope_from_descriptor(&definition.scope),\n";
    out << "                owner: definition.owner.clone(),\n";
    out << "                description: definition.description.clone(),\n";
    out << "                expires: definition.expires.clone(),\n";
    out << "            },\n";
    out << "        )?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_queue_definitions_tx<B: Backend, S: QueueStore<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    store: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in queue_definitions() {\n";
    out << "        store.register_definition_tx(tx, &RegisterQueueDefinitionRequest { definition "
           "})?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_lease_definitions_tx<B: Backend, S: LeaseStore<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    store: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in lease_definitions() {\n";
    out << "        let runtime_definition = lease_definition_from_descriptor(definition);\n";
    out << "        store.register_definition_tx(tx, &runtime_definition)?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_log_definitions_tx<B: Backend, S: LogSink<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    sink: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in log_definitions() {\n";
    out << "        sink.register_definition_tx(\n";
    out << "            tx,\n";
    out << "            &RuntimeLogDefinition {\n";
    out << "                name: definition.name,\n";
    out << "                level: log_level_from_descriptor(&definition.level),\n";
    out << "                event_name: definition.event_name,\n";
    out << "                fields: definition.fields,\n";
    out << "            },\n";
    out << "        )?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_metric_definitions_tx<B: Backend, S: MetricSink<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    sink: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in metric_definitions() {\n";
    out << "        sink.register_definition_tx(\n";
    out << "            tx,\n";
    out << "            &RuntimeMetricDefinition {\n";
    out << "                name: definition.name,\n";
    out << "                kind: metric_kind_from_descriptor(&definition.kind),\n";
    out << "                backend_name: definition.backend_name,\n";
    out << "                unit: definition.unit,\n";
    out << "                labels: definition.labels,\n";
    out << "            },\n";
    out << "        )?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_observability_catalog_tx<B, L, M>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    log_sink: &L,\n";
    out << "    metric_sink: &M,\n";
    out << ") -> BackendResult<()>\n";
    out << "where\n";
    out << "    B: Backend,\n";
    out << "    L: LogSink<B>,\n";
    out << "    M: MetricSink<B>,\n";
    out << "{\n";
    out << "    register_log_definitions_tx(tx, log_sink)?;\n";
    out << "    register_metric_definitions_tx(tx, metric_sink)\n";
    out << "}\n\n";

    out << "pub fn register_workflow_definitions_tx<B: Backend, S: WorkflowStore<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    store: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in workflow_definitions() {\n";
    out << "        store.register_definition_tx(\n";
    out << "            tx,\n";
    out << "            &RegisterWorkflowDefinitionRequest { definition },\n";
    out << "        )?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_runtime_catalog_tx<B, F, Q, L, W, LS, M>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    feature_flag_store: &F,\n";
    out << "    queue_store: &Q,\n";
    out << "    lease_store: &L,\n";
    out << "    workflow_store: &W,\n";
    out << "    log_sink: &LS,\n";
    out << "    metric_sink: &M,\n";
    out << ") -> BackendResult<()>\n";
    out << "where\n";
    out << "    B: Backend,\n";
    out << "    F: FeatureFlagStore<B>,\n";
    out << "    Q: QueueStore<B>,\n";
    out << "    L: LeaseStore<B>,\n";
    out << "    W: WorkflowStore<B>,\n";
    out << "    LS: LogSink<B>,\n";
    out << "    M: MetricSink<B>,\n";
    out << "{\n";
    out << "    register_feature_flag_definitions_tx(tx, feature_flag_store)?;\n";
    out << "    register_queue_definitions_tx(tx, queue_store)?;\n";
    out << "    register_lease_definitions_tx(tx, lease_store)?;\n";
    out << "    register_workflow_definitions_tx(tx, workflow_store)?;\n";
    out << "    register_observability_catalog_tx(tx, log_sink, metric_sink)\n";
    out << "}\n";
    return out.str();
}

std::string generate_workflow_step_handler_keys_rs(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "        " << rust_string(workflow.name + "." + step.name) << ",\n";
        }
    }
    return out.str();
}

} // namespace

GenerationResult generate_rust_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const TemplatePackage templates{resolve_binding_template_root(options)};

    add_template_file(result, options.output_dir, templates, "json.rs", "json.rs", diagnostics);
    add_template_file(
        result, options.output_dir, templates, "backend.rs", "backend.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "external_system.rs", "external_system.rs",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "feature_flag.rs", "feature_flag.rs", diagnostics
    );
    add_template_file(result, options.output_dir, templates, "lease.rs", "lease.rs", diagnostics);
    add_template_file(result, options.output_dir, templates, "log.rs", "log.rs", diagnostics);
    add_template_file(result, options.output_dir, templates, "metric.rs", "metric.rs", diagnostics);
    add_template_file(result, options.output_dir, templates, "queue.rs", "queue.rs", diagnostics);
    add_template_file(
        result, options.output_dir, templates, "workflow.rs", "workflow.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "memory/backend.rs", "memory/backend.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "memory/transaction.rs", "memory/transaction.rs",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec.rs", "runtime/codec.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/feature_flags.rs",
        "runtime/feature_flags.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/queues.rs", "runtime/queues.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/leases.rs", "runtime/leases.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/workflows.rs", "runtime/workflows.rs",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/logs.rs", "runtime/logs.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/metrics.rs", "runtime/metrics.rs",
        diagnostics
    );

    if (!diagnostics.has_errors())
    {
        add_generated_template_file(
            result, options.output_dir, templates, "generated/descriptors.rs.tmpl",
            "common/descriptors.rs", diagnostics, GeneratedArtifactTier::Common,
            TemplateRenderer::Values{{"descriptors", generate_descriptors_rs(system)}}
        );
        add_generated_template_file(
            result, options.output_dir, templates, "generated/Cargo.toml.tmpl", "Cargo.toml",
            diagnostics, GeneratedArtifactTier::Common, {}, "common/Cargo.toml"
        );
        add_generated_template_file(
            result, options.output_dir, templates, "generated/lib.rs.tmpl", "lib.rs", diagnostics,
            GeneratedArtifactTier::Common, rust_lib_values(options.tier), "common/lib.rs"
        );
        add_generated_template_file(
            result, options.output_dir, templates, "generated/Makefile.tmpl", "Makefile",
            diagnostics, GeneratedArtifactTier::Common, rust_makefile_values(options.tier),
            "common/Makefile"
        );
        add_generated_template_file(
            result, options.output_dir, templates, "api/api_descriptors.rs.tmpl",
            "api/api_descriptors.rs", diagnostics, GeneratedArtifactTier::Api
        );
        add_generated_template_file(
            result, options.output_dir, templates, "api/api_handlers.rs.tmpl",
            "api/api_handlers.rs", diagnostics, GeneratedArtifactTier::Api
        );
        add_generated_template_file(
            result, options.output_dir, templates, "api/api_dispatcher.rs.tmpl",
            "api/api_dispatcher.rs", diagnostics, GeneratedArtifactTier::Api
        );
        add_generated_template_file(
            result, options.output_dir, templates, "api/api_server.rs.tmpl", "api/api_server.rs",
            diagnostics, GeneratedArtifactTier::Api
        );
        add_generated_template_file(
            result, options.output_dir, templates, "api/api_routes.rs.tmpl", "api/api_routes.rs",
            diagnostics, GeneratedArtifactTier::Api
        );
        add_generated_template_file(
            result, options.output_dir, templates,
            "api/external_system_operator_metadata_api.rs.tmpl",
            "api/external_system_operator_metadata_api.rs", diagnostics, GeneratedArtifactTier::Api
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_descriptors.rs.tmpl",
            "worker/worker_descriptors.rs", diagnostics, GeneratedArtifactTier::Worker
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_contexts.rs.tmpl",
            "worker/worker_contexts.rs", diagnostics, GeneratedArtifactTier::Worker
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_registry.rs.tmpl",
            "worker/worker_registry.rs", diagnostics, GeneratedArtifactTier::Worker
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_application.rs.tmpl",
            "worker/worker_application.rs", diagnostics, GeneratedArtifactTier::Worker
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/workflow_step_handlers.rs.tmpl",
            "worker/workflow_step_handlers.rs", diagnostics, GeneratedArtifactTier::Worker,
            TemplateRenderer::Values{
                {"workflow_step_handler_keys", generate_workflow_step_handler_keys_rs(system)}
            }
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/workflow_runner.rs.tmpl",
            "worker/workflow_runner.rs", diagnostics, GeneratedArtifactTier::Worker
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_handlers.rs.tmpl",
            "worker/worker_handlers.rs", diagnostics, GeneratedArtifactTier::Worker
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_queues.rs.tmpl",
            "worker/worker_queues.rs", diagnostics, GeneratedArtifactTier::Worker
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_leases.rs.tmpl",
            "worker/worker_leases.rs", diagnostics, GeneratedArtifactTier::Worker
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_workflows.rs.tmpl",
            "worker/worker_workflows.rs", diagnostics, GeneratedArtifactTier::Worker
        );
    }

    return result;
}

} // namespace statespec
