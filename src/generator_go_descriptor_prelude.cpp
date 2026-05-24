#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{

namespace
{

std::string go_descriptor_module_imports(const IrSystem& system)
{
    std::ostringstream out;
    out << "\t_ \"statespec-generated/common/backend/descriptors\"\n";
    if (!system.entities.empty())
    {
        out << "\t_ \"statespec-generated/common/backend/descriptors/entities\"\n";
    }
    return out.str();
}

} // namespace

std::string generate_go_descriptor_prelude(
    const IrSystem& system,
    const std::string& external_system_runtime,
    const std::string& external_system_metadata_runtime,
    const std::string& entity_repository_runtime
)
{
    std::ostringstream out;
    const auto runtime_usage = runtime_domain_usage(system);
    out << "package backend\n\n";
    out << "import (\n";
    out << "\t\"context\"\n";
    out << "\t\"errors\"\n";
    out << go_descriptor_module_imports(system);
    if (runtime_usage.uses_feature_flags)
    {
        out << "\t\"strconv\"\n";
    }
    out << "\t\"strings\"\n";
    out << "\t\"time\"\n";
    out << ")\n\n";
    out << "type EventEnvelope struct {\n";
    out << "\tName string\n";
    out << "\tFields map[string]JSON\n";
    out << "}\n\n";
    for (const auto& shape : system.shapes)
    {
        out << "type " << pascal_identifier(shape.name) << " struct {\n";
        for (const auto& field : shape.fields)
        {
            out << "\t" << pascal_identifier(field.name) << " " << go_shape_type(field.type)
                << " `json:\"" << field.name << "\"`\n";
        }
        out << "}\n\n";
    }

    for (const auto& event : system.events)
    {
        out << "func New" << pascal_identifier(event.name) << "Event(";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            if (i > 0)
            {
                out << ", ";
            }
            out << lower_camel_identifier(field.name) << " JSON";
        }
        out << ") EventEnvelope {\n";
        out << "\treturn EventEnvelope{\n";
        out << "\t\tName: " << go_string(event.name) << ",\n";
        out << "\t\tFields: map[string]JSON{\n";
        for (const auto& field : event.fields)
        {
            out << "\t\t\t" << go_string(field.name) << ": " << lower_camel_identifier(field.name)
                << ",\n";
        }
        out << "\t\t},\n";
        out << "\t}\n";
        out << "}\n\n";
    }

    out << "type LeaseDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tResource *string\n";
    out << "\tTTL time.Duration\n";
    out << "\tRenewEvery *time.Duration\n";
    out << "\tHolder *string\n";
    out << "\tFencingToken bool\n";
    out << "\tMaxTTL *time.Duration\n";
    out << "}\n\n";
    out << "type FeatureFlagDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tType string\n";
    out << "\tDefaultValue string\n";
    out << "\tScope string\n";
    out << "\tOwner *string\n";
    out << "\tDescription *string\n";
    out << "\tExpires *string\n";
    out << "}\n\n";
    out << "type ValueDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tType string\n";
    out << "\tConstraint *string\n";
    out << "}\n\n";
    out << "type EnumMemberDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tValue *string\n";
    out << "}\n\n";
    out << "type EnumDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tMembers []EnumMemberDescriptor\n";
    out << "}\n\n";
    out << "type EventDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tFields []FieldDescriptor\n";
    out << "}\n\n";
    out << "type ExternalSystemPropertyDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tValue string\n";
    out << "}\n\n";
    out << "type ExternalSystemMetadataMappingDescriptor struct {\n";
    out << "\tSource string\n";
    out << "\tTarget string\n";
    out << "}\n\n";
    out << "type ExternalSystemMetadataMappingAssignment struct {\n";
    out << "\tSource string\n";
    out << "\tSourceRoot string\n";
    out << "\tSourceField string\n";
    out << "\tTargetRoot string\n";
    out << "\tField string\n";
    out << "}\n\n";
    out << "type ExternalSystemMetadataMappingPlan struct {\n";
    out << "\tAllMappings []ExternalSystemMetadataMappingAssignment\n";
    out << "\tClientMappings []ExternalSystemMetadataMappingAssignment\n";
    out << "\tRequestMappings []ExternalSystemMetadataMappingAssignment\n";
    out << "}\n\n";
    out << "type ExternalSystemMetadataMissingMappingSource struct {\n";
    out << "\tSource string\n";
    out << "\tSourceRoot string\n";
    out << "\tSourceField string\n";
    out << "\tTargetRoot string\n";
    out << "\tField string\n";
    out << "}\n\n";
    out << "type ExternalSystemMetadataMappingInputs struct {\n";
    out << "\tInput map[string]JSON\n";
    out << "\tEntity map[string]JSON\n";
    out << "\tWorkflow map[string]JSON\n";
    out << "\tMetadata map[string]JSON\n";
    out << "}\n\n";
    out << "func (i ExternalSystemMetadataMappingInputs) SourceValue(sourceRoot string, "
           "sourceField string) (JSON, bool) {\n";
    out << "\tvar values map[string]JSON\n";
    out << "\tswitch sourceRoot {\n";
    out << "\tcase \"input\":\n";
    out << "\t\tvalues = i.Input\n";
    out << "\tcase \"entity\":\n";
    out << "\t\tvalues = i.Entity\n";
    out << "\tcase \"workflow\":\n";
    out << "\t\tvalues = i.Workflow\n";
    out << "\tcase \"metadata\":\n";
    out << "\t\tvalues = i.Metadata\n";
    out << "\tdefault:\n";
    out << "\t\treturn JSON{}, false\n";
    out << "\t}\n";
    out << "\tvalue, ok := values[sourceField]\n";
    out << "\treturn value, ok\n";
    out << "}\n\n";
    out << "func (i ExternalSystemMetadataMappingInputs) AssignmentValue(assignment "
           "ExternalSystemMetadataMappingAssignment) (JSON, bool) {\n";
    out << "\treturn i.SourceValue(assignment.SourceRoot, assignment.SourceField)\n";
    out << "}\n\n";
    out << "type ExternalSystemMetadataMappingOutput struct {\n";
    out << "\tClientConfig map[string]JSON\n";
    out << "\tRequestPayload map[string]JSON\n";
    out << "\tMissingSources []ExternalSystemMetadataMissingMappingSource\n";
    out << "}\n\n";
    out << "func MissingExternalSystemMetadataMappingSources(plan "
           "ExternalSystemMetadataMappingPlan, inputs ExternalSystemMetadataMappingInputs) "
           "[]ExternalSystemMetadataMissingMappingSource {\n";
    out << "\tmissing := []ExternalSystemMetadataMissingMappingSource{}\n";
    out << "\tfor _, assignment := range plan.AllMappings {\n";
    out << "\t\tif _, ok := inputs.AssignmentValue(assignment); !ok {\n";
    out << "\t\t\tmissing = append(missing, ExternalSystemMetadataMissingMappingSource{\n";
    out << "\t\t\t\tSource: assignment.Source,\n";
    out << "\t\t\t\tSourceRoot: assignment.SourceRoot,\n";
    out << "\t\t\t\tSourceField: assignment.SourceField,\n";
    out << "\t\t\t\tTargetRoot: assignment.TargetRoot,\n";
    out << "\t\t\t\tField: assignment.Field,\n";
    out << "\t\t\t})\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn missing\n";
    out << "}\n\n";
    out << "type ExternalSystemMetadataMappingApplicator interface {\n";
    out << "\tApplyExternalSystemMetadataMappings(context.Context, "
           "ExternalSystemMetadataMappingPlan, ExternalSystemMetadataMappingInputs) "
           "(ExternalSystemMetadataMappingOutput, error)\n";
    out << "}\n\n";
    out << external_system_runtime << "\n";
    out << "type ExternalSystemMetadataDescriptor struct {\n";
    out << "\tEntity string\n";
    out << "\tTenantField *string\n";
    out << "\tProfileField string\n";
    out << "\tKeyFields []string\n";
    out << "\tRequiredFields []string\n";
    out << "\tMappings []ExternalSystemMetadataMappingDescriptor\n";
    out << "}\n\n";
    out << "type ExternalSystemOperatorMetadataUpsertRequest struct {\n";
    out << "\tLookup ExternalSystemMetadataLookup\n";
    out << "\tDocument JSON\n";
    out << "\tExpectedVersion *Version\n";
    out << "}\n\n";
    out << "type ExternalSystemOperatorMetadataGetRequest struct {\n";
    out << "\tLookup ExternalSystemMetadataLookup\n";
    out << "}\n\n";
    out << "type ExternalSystemOperatorMetadataDisableRequest struct {\n";
    out << "\tLookup ExternalSystemMetadataLookup\n";
    out << "\tExpectedVersion *Version\n";
    out << "\tDisabledStatus string\n";
    out << "}\n\n";
    out << "type ExternalSystemOperatorMetadataDeleteRequest struct {\n";
    out << "\tLookup ExternalSystemMetadataLookup\n";
    out << "\tExpectedVersion *Version\n";
    out << "\tDeletedStatus string\n";
    out << "}\n\n";
    out << "type ExternalSystemOperatorMetadataRepository interface {\n";
    out << "\tUpsertMetadataTx(context.Context, Transaction, "
           "ExternalSystemOperatorMetadataUpsertRequest) (*VersionedRecord, error)\n";
    out << "\tGetMetadataTx(context.Context, Transaction, "
           "ExternalSystemOperatorMetadataGetRequest) (*VersionedRecord, error)\n";
    out << "\tDisableMetadataTx(context.Context, Transaction, "
           "ExternalSystemOperatorMetadataDisableRequest) (*VersionedRecord, error)\n";
    out << "\tDeleteMetadataTx(context.Context, Transaction, "
           "ExternalSystemOperatorMetadataDeleteRequest) (*VersionedRecord, error)\n";
    out << "}\n\n";
    out << external_system_metadata_runtime << "\n";
    out << "type ExternalSystemDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tProperties []ExternalSystemPropertyDescriptor\n";
    out << "\tMetadata *ExternalSystemMetadataDescriptor\n";
    out << "}\n\n";
    out << "type ApiDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tMethod *string\n";
    out << "\tPath *string\n";
    out << "\tInput *string\n";
    out << "\tOutput *string\n";
    out << "\tError *string\n";
    out << "\tStartsWorkflow *string\n";
    out << "\tEnqueues *string\n";
    out << "}\n\n";
    out << "type ApiServerDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tServes []string\n";
    out << "\tConcurrency int\n";
    out << "}\n\n";
    out << "type ApiRouteDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tServerName string\n";
    out << "\tApiName string\n";
    out << "\tMethod *string\n";
    out << "\tPath *string\n";
    out << "\tInput *string\n";
    out << "\tOutput *string\n";
    out << "\tError *string\n";
    out << "}\n\n";
    out << "type APIRequestContext struct {\n";
    out << "\tServerName string\n";
    out << "\tAPIName string\n";
    out << "\tMethod *string\n";
    out << "\tPath *string\n";
    out << "\tBody JSON\n";
    out << "}\n\n";
    out << "type APIResponse struct {\n";
    out << "\tStatusCode int\n";
    out << "\tBody JSON\n";
    out << "}\n\n";
    out << "type ExternalSystemOperatorMetadataAPIHandler interface {\n";
    out << "\tHandleUpsertMetadataTx(context.Context, Transaction, "
           "ExternalSystemOperatorMetadataRepository, "
           "ExternalSystemOperatorMetadataUpsertRequest) (APIResponse, error)\n";
    out << "\tHandleGetMetadataTx(context.Context, Transaction, "
           "ExternalSystemOperatorMetadataRepository, "
           "ExternalSystemOperatorMetadataGetRequest) (APIResponse, error)\n";
    out << "\tHandleDisableMetadataTx(context.Context, Transaction, "
           "ExternalSystemOperatorMetadataRepository, "
           "ExternalSystemOperatorMetadataDisableRequest) (APIResponse, error)\n";
    out << "\tHandleDeleteMetadataTx(context.Context, Transaction, "
           "ExternalSystemOperatorMetadataRepository, "
           "ExternalSystemOperatorMetadataDeleteRequest) (APIResponse, error)\n";
    out << "}\n\n";
    out << "type WorkerDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tSingleton bool\n";
    out << "\tLease *string\n";
    out << "\tPolls *string\n";
    out << "\tExecutes *string\n";
    out << "\tConcurrency int\n";
    out << "}\n\n";
    out << "type WorkerContext struct {\n";
    out << "\tWorkerName string\n";
    out << "\tSingleton bool\n";
    out << "\tLease *string\n";
    out << "\tPolls *string\n";
    out << "\tExecutes *string\n";
    out << "\tConcurrency int\n";
    out << "}\n\n";
    out << "type PolicyRuleDescriptor struct {\n";
    out << "\tAction string\n";
    out << "\tCondition string\n";
    out << "}\n\n";
    out << "type QuotaDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tExpression string\n";
    out << "}\n\n";
    out << "type PolicyDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tTenantScopedBy *string\n";
    out << "\tAllows []PolicyRuleDescriptor\n";
    out << "\tDenies []PolicyRuleDescriptor\n";
    out << "\tQuotas []QuotaDescriptor\n";
    out << "\tAudits []string\n";
    out << "}\n\n";
    out << "type ShapeDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tFields []FieldDescriptor\n";
    out << "}\n\n";
    out << "type LogDefinition struct {\n";
    out << "\tName string\n";
    out << "\tLevel string\n";
    out << "\tEventName string\n";
    out << "\tFields []FieldDescriptor\n";
    out << "}\n\n";
    out << "type MetricDefinition struct {\n";
    out << "\tName string\n";
    out << "\tKind string\n";
    out << "\tBackendName string\n";
    out << "\tUnit string\n";
    out << "\tLabels []FieldDescriptor\n";
    out << "}\n\n";
    out << "type GarbageCollectionPolicy struct {\n";
    out << "\tAfter string\n";
    out << "\tMode string\n";
    out << "}\n\n";
    out << "type EntityStateDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tTerminal bool\n";
    out << "\tGarbageCollection *GarbageCollectionPolicy\n";
    out << "}\n\n";
    out << "type EntityOwnershipDescriptor struct {\n";
    out << "\tAuthority string\n";
    out << "\tSystemOfRecord string\n";
    out << "\tLifecycle string\n";
    out << "}\n\n";
    out << "type EntityRelationDescriptor struct {\n";
    out << "\tKind string\n";
    out << "\tName string\n";
    out << "\tTarget string\n";
    out << "\tOptional bool\n";
    out << "\tRelationKind *string\n";
    out << "\tOnParentDelete *string\n";
    out << "\tParentMustBeIn []string\n";
    out << "\tUniqueWithinParent []string\n";
    out << "}\n\n";
    out << "type EntityChildDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tTargetEntity string\n";
    out << "\tRelation string\n";
    out << "}\n\n";
    out << "type EntityInvariantDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tExpression string\n";
    out << "}\n\n";
    out << "type EntityDescriptor struct {\n";
    out << "\tName string\n";
    out << "\tKeyFields []string\n";
    out << "\tOwnership *EntityOwnershipDescriptor\n";
    out << "\tRelations []EntityRelationDescriptor\n";
    out << "\tChildren []EntityChildDescriptor\n";
    out << "\tInvariants []EntityInvariantDescriptor\n";
    out << "\tStates []EntityStateDescriptor\n";
    out << "\tInitialState *string\n";
    out << "\tTerminalStates []string\n";
    out << "}\n\n";
    out << entity_repository_runtime << "\n";
    out << "func stringPtr(value string) *string { return &value }\n";
    out << "func durationPtr(value time.Duration) *time.Duration { return &value }\n\n";

    return out.str();
}

} // namespace statespec
