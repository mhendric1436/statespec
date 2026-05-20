#include "statespec/generator_go.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace statespec
{

namespace
{

std::string read_template_file(
    const std::filesystem::path& path,
    DiagnosticBag& diagnostics
)
{
    std::ifstream input(path);
    if (!input)
    {
        diagnostics.error(
            SourceRange{}, "SSPEC5201", "failed to read binding template: " + path.string()
        );
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void add_template_file(
    GenerationResult& result,
    const std::filesystem::path& output_dir,
    const std::filesystem::path& template_path,
    const std::filesystem::path& relative_output_path,
    DiagnosticBag& diagnostics,
    GeneratedArtifactTier tier = GeneratedArtifactTier::Common
)
{
    const auto content = read_template_file(template_path, diagnostics);
    if (diagnostics.has_errors())
    {
        return;
    }

    result.files.push_back(
        GeneratedFile{
            (output_dir / "common" / relative_output_path).string(),
            content,
            tier,
            (std::filesystem::path{"common"} / relative_output_path).generic_string(),
        }
    );
}

std::string go_string(const std::string& value)
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

std::string go_field_type_expr(FieldDescriptorType type)
{
    switch (type)
    {
    case FieldDescriptorType::String:
        return "FieldTypeString";
    case FieldDescriptorType::Bool:
        return "FieldTypeBool";
    case FieldDescriptorType::Int:
        return "FieldTypeInt";
    case FieldDescriptorType::Int32:
        return "FieldTypeInt32";
    case FieldDescriptorType::Int64:
        return "FieldTypeInt64";
    case FieldDescriptorType::Long:
        return "FieldTypeLong";
    case FieldDescriptorType::Double:
        return "FieldTypeDouble";
    case FieldDescriptorType::Decimal:
        return "FieldTypeDecimal";
    case FieldDescriptorType::Json:
        return "FieldTypeJSON";
    case FieldDescriptorType::Timestamp:
        return "FieldTypeTimestamp";
    case FieldDescriptorType::Duration:
        return "FieldTypeDuration";
    case FieldDescriptorType::Uuid:
        return "FieldTypeUUID";
    case FieldDescriptorType::Named:
        return "FieldTypeNamed";
    case FieldDescriptorType::List:
        return "FieldTypeList";
    case FieldDescriptorType::Set:
        return "FieldTypeSet";
    case FieldDescriptorType::Map:
        return "FieldTypeMap";
    case FieldDescriptorType::Optional:
        return "FieldTypeOptional";
    case FieldDescriptorType::Reference:
        return "FieldTypeReference";
    }
    return "FieldTypeNamed";
}

bool is_required_descriptor_field(const std::string& type)
{
    return classify_field_descriptor_type(type) != FieldDescriptorType::Optional;
}

std::string go_field_descriptor_expr(const IrField& field)
{
    return "{Name: " + go_string(field.name) +
           ", Type: " + go_field_type_expr(classify_field_descriptor_type(field.type)) +
           ", TypeName: " + go_string(field.type) +
           ", Required: " + (is_required_descriptor_field(field.type) ? "true" : "false") + "}";
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

std::string lower_camel_identifier(const std::string& value)
{
    auto identifier = pascal_identifier(value);
    if (!identifier.empty())
    {
        identifier.front() =
            static_cast<char>(std::tolower(static_cast<unsigned char>(identifier.front())));
    }
    return identifier.empty() ? "value" : identifier;
}

std::string generate_go_mod()
{
    std::ostringstream out;
    out << "module statespec-generated\n\n";
    out << "go 1.22\n";
    return out.str();
}

std::string go_shape_type(const std::string& type)
{
    const auto optional = is_optional_type(type);
    const auto base = strip_optional_suffix(type);
    std::string mapped = "string";
    if (base == "bool")
    {
        mapped = "bool";
    }
    else if (base == "int" || base == "int32")
    {
        mapped = "int32";
    }
    else if (base == "int64" || base == "long")
    {
        mapped = "int64";
    }
    else if (base == "double" || base == "decimal")
    {
        mapped = "float64";
    }

    return optional ? "*" + mapped : mapped;
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

std::string string_ptr_expr(const std::optional<std::string>& value)
{
    return value.has_value() ? "stringPtr(" + go_string(*value) + ")" : "nil";
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

std::string duration_ptr_expr(const std::optional<std::string>& value)
{
    if (!value.has_value())
    {
        return "nil";
    }
    return "durationPtr(" + std::to_string(parse_duration_seconds(value)) + " * time.Second)";
}

std::string generate_descriptors_go(const IrSystem& system)
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import (\n";
    out << "\t\"context\"\n";
    out << "\t\"strconv\"\n";
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
    out << "type APIHandler interface {\n";
    out << "\tHandle(context.Context, APIRequestContext) (APIResponse, error)\n";
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
    out << "type Worker interface {\n";
    out << "\tRun(context.Context, WorkerContext) error\n";
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
    out << "func stringPtr(value string) *string { return &value }\n";
    out << "func durationPtr(value time.Duration) *time.Duration { return &value }\n\n";

    out << "func FeatureFlagDefinitions() []FeatureFlagDescriptor {\n";
    out << "\treturn []FeatureFlagDescriptor{\n";
    for (const auto& flag : system.feature_flags)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(flag.name) << ",\n";
        out << "\t\t\tType: " << go_string(flag.type) << ",\n";
        out << "\t\t\tDefaultValue: " << go_string(flag.default_value) << ",\n";
        out << "\t\t\tScope: " << go_string(flag.scope) << ",\n";
        out << "\t\t\tOwner: " << string_ptr_expr(flag.owner) << ",\n";
        out << "\t\t\tDescription: " << string_ptr_expr(flag.description) << ",\n";
        out << "\t\t\tExpires: " << string_ptr_expr(flag.expires) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func ValueDescriptors() []ValueDescriptor {\n";
    out << "\treturn []ValueDescriptor{\n";
    for (const auto& value : system.values)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(value.name) << ",\n";
        out << "\t\t\tType: " << go_string(value.type) << ",\n";
        out << "\t\t\tConstraint: " << string_ptr_expr(value.constraint) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func EnumDescriptors() []EnumDescriptor {\n";
    out << "\treturn []EnumDescriptor{\n";
    for (const auto& enum_decl : system.enums)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(enum_decl.name) << ",\n";
        out << "\t\t\tMembers: []EnumMemberDescriptor{\n";
        for (const auto& member : enum_decl.members)
        {
            out << "\t\t\t\t{Name: " << go_string(member.name)
                << ", Value: " << string_ptr_expr(member.value) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func EventDescriptors() []EventDescriptor {\n";
    out << "\treturn []EventDescriptor{\n";
    for (const auto& event : system.events)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(event.name) << ",\n";
        out << "\t\t\tFields: []FieldDescriptor{\n";
        for (const auto& field : event.fields)
        {
            out << "\t\t\t\t" << go_field_descriptor_expr(field) << ",\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

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

    out << "func ApiDescriptors() []ApiDescriptor {\n";
    out << "\treturn []ApiDescriptor{\n";
    for (const auto& api : system.apis)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(api.name) << ",\n";
        out << "\t\t\tMethod: " << string_ptr_expr(api.method) << ",\n";
        out << "\t\t\tPath: " << string_ptr_expr(api.path) << ",\n";
        out << "\t\t\tInput: " << string_ptr_expr(api.input) << ",\n";
        out << "\t\t\tOutput: " << string_ptr_expr(api.output) << ",\n";
        out << "\t\t\tError: " << string_ptr_expr(api.error) << ",\n";
        out << "\t\t\tStartsWorkflow: " << string_ptr_expr(api.starts_workflow) << ",\n";
        out << "\t\t\tEnqueues: " << string_ptr_expr(api.enqueues) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func ApiServerDescriptors() []ApiServerDescriptor {\n";
    out << "\treturn []ApiServerDescriptor{\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(api_server.name) << ",\n";
        out << "\t\t\tServes: []string{";
        for (std::size_t i = 0; i < api_server.serves.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_string(api_server.serves[i]);
        }
        out << "},\n";
        out << "\t\t\tConcurrency: " << api_server.concurrency.value_or(1) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func ApiRouteDescriptors() []ApiRouteDescriptor {\n";
    out << "\treturn []ApiRouteDescriptor{\n";
    for (const auto& api_server : system.api_servers)
    {
        for (const auto& api_name : api_server.serves)
        {
            const auto* api = find_api(system, api_name);
            out << "\t\t{\n";
            out << "\t\t\tName: " << go_string(api_server.name + "." + api_name) << ",\n";
            out << "\t\t\tServerName: " << go_string(api_server.name) << ",\n";
            out << "\t\t\tApiName: " << go_string(api_name) << ",\n";
            out << "\t\t\tMethod: " << string_ptr_expr(optional_api_field(api, &IrApi::method))
                << ",\n";
            out << "\t\t\tPath: " << string_ptr_expr(optional_api_field(api, &IrApi::path))
                << ",\n";
            out << "\t\t\tInput: " << string_ptr_expr(optional_api_field(api, &IrApi::input))
                << ",\n";
            out << "\t\t\tOutput: " << string_ptr_expr(optional_api_field(api, &IrApi::output))
                << ",\n";
            out << "\t\t\tError: " << string_ptr_expr(optional_api_field(api, &IrApi::error))
                << ",\n";
            out << "\t\t},\n";
        }
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func WorkerDescriptors() []WorkerDescriptor {\n";
    out << "\treturn []WorkerDescriptor{\n";
    for (const auto& worker : system.workers)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(worker.name) << ",\n";
        out << "\t\t\tSingleton: " << (worker.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "\t\t\tLease: " << string_ptr_expr(worker.lease) << ",\n";
        out << "\t\t\tPolls: " << string_ptr_expr(worker.polls) << ",\n";
        out << "\t\t\tExecutes: " << string_ptr_expr(worker.executes) << ",\n";
        out << "\t\t\tConcurrency: " << worker.concurrency.value_or(1) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func WorkerContexts() []WorkerContext {\n";
    out << "\treturn []WorkerContext{\n";
    for (const auto& worker : system.workers)
    {
        out << "\t\t{\n";
        out << "\t\t\tWorkerName: " << go_string(worker.name) << ",\n";
        out << "\t\t\tSingleton: " << (worker.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "\t\t\tLease: " << string_ptr_expr(worker.lease) << ",\n";
        out << "\t\t\tPolls: " << string_ptr_expr(worker.polls) << ",\n";
        out << "\t\t\tExecutes: " << string_ptr_expr(worker.executes) << ",\n";
        out << "\t\t\tConcurrency: " << worker.concurrency.value_or(1) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func PolicyDescriptors() []PolicyDescriptor {\n";
    out << "\treturn []PolicyDescriptor{\n";
    for (const auto& policy : system.policies)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(policy.name) << ",\n";
        out << "\t\t\tTenantScopedBy: " << string_ptr_expr(policy.tenant_scoped_by) << ",\n";
        out << "\t\t\tAllows: []PolicyRuleDescriptor{\n";
        for (const auto& allow : policy.allows)
        {
            out << "\t\t\t\t{Action: " << go_string(allow.action)
                << ", Condition: " << go_string(allow.condition) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tDenies: []PolicyRuleDescriptor{\n";
        for (const auto& deny : policy.denies)
        {
            out << "\t\t\t\t{Action: " << go_string(deny.action)
                << ", Condition: " << go_string(deny.condition) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tQuotas: []QuotaDescriptor{\n";
        for (const auto& quota : policy.quotas)
        {
            out << "\t\t\t\t{Name: " << go_string(quota.name)
                << ", Expression: " << go_string(quota.expression) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tAudits: []string{";
        for (std::size_t i = 0; i < policy.audits.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_string(policy.audits[i]);
        }
        out << "},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func ShapeDescriptors() []ShapeDescriptor {\n";
    out << "\treturn []ShapeDescriptor{\n";
    for (const auto& shape : system.shapes)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(shape.name) << ",\n";
        out << "\t\t\tFields: []FieldDescriptor{\n";
        for (const auto& field : shape.fields)
        {
            out << "\t\t\t\t" << go_field_descriptor_expr(field) << ",\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func LogDefinitions() []LogDefinition {\n";
    out << "\treturn []LogDefinition{\n";
    for (const auto& log : system.logs)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(log.name) << ",\n";
        out << "\t\t\tLevel: " << go_string(log.level) << ",\n";
        out << "\t\t\tEventName: " << go_string(log.event_name) << ",\n";
        out << "\t\t\tFields: []FieldDescriptor{\n";
        for (const auto& field : log.fields)
        {
            out << "\t\t\t\t" << go_field_descriptor_expr(field) << ",\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func MetricDefinitions() []MetricDefinition {\n";
    out << "\treturn []MetricDefinition{\n";
    for (const auto& metric : system.metrics)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(metric.name) << ",\n";
        out << "\t\t\tKind: " << go_string(metric.kind) << ",\n";
        out << "\t\t\tBackendName: " << go_string(metric.backend_name) << ",\n";
        out << "\t\t\tUnit: " << go_string(metric.unit) << ",\n";
        out << "\t\t\tLabels: []FieldDescriptor{\n";
        for (const auto& label : metric.labels)
        {
            out << "\t\t\t\t" << go_field_descriptor_expr(label) << ",\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func EntityDescriptors() []EntityDescriptor {\n";
    out << "\treturn []EntityDescriptor{\n";
    for (const auto& entity : system.entities)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(entity.name) << ",\n";
        out << "\t\t\tKeyFields: []string{";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_string(entity.key_fields[i]);
        }
        out << "},\n";
        if (entity.ownership.has_value())
        {
            out << "\t\t\tOwnership: &EntityOwnershipDescriptor{Authority: "
                << go_string(entity.ownership->authority)
                << ", SystemOfRecord: " << go_string(entity.ownership->system_of_record)
                << ", Lifecycle: " << go_string(entity.ownership->lifecycle) << "},\n";
        }
        else
        {
            out << "\t\t\tOwnership: nil,\n";
        }
        out << "\t\t\tRelations: []EntityRelationDescriptor{\n";
        for (const auto& relation : entity.relations)
        {
            out << "\t\t\t\t{\n";
            out << "\t\t\t\t\tKind: " << go_string(relation.kind) << ",\n";
            out << "\t\t\t\t\tName: " << go_string(relation.name) << ",\n";
            out << "\t\t\t\t\tTarget: " << go_string(relation.target) << ",\n";
            out << "\t\t\t\t\tOptional: " << (relation.optional ? "true" : "false") << ",\n";
            out << "\t\t\t\t\tRelationKind: " << string_ptr_expr(relation.relation_kind) << ",\n";
            out << "\t\t\t\t\tOnParentDelete: " << string_ptr_expr(relation.on_parent_delete)
                << ",\n";
            out << "\t\t\t\t\tParentMustBeIn: []string{";
            for (std::size_t i = 0; i < relation.parent_must_be_in.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << go_string(relation.parent_must_be_in[i]);
            }
            out << "},\n";
            out << "\t\t\t\t\tUniqueWithinParent: []string{";
            for (std::size_t i = 0; i < relation.unique_within_parent.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << go_string(relation.unique_within_parent[i]);
            }
            out << "},\n";
            out << "\t\t\t\t},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tChildren: []EntityChildDescriptor{\n";
        for (const auto& child : entity.children)
        {
            out << "\t\t\t\t{Name: " << go_string(child.name)
                << ", TargetEntity: " << go_string(child.target_entity)
                << ", Relation: " << go_string(child.relation) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tInvariants: []EntityInvariantDescriptor{\n";
        for (const auto& invariant : entity.invariants)
        {
            out << "\t\t\t\t{Name: " << go_string(invariant.name)
                << ", Expression: " << go_string(invariant.expression) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tStates: []EntityStateDescriptor{\n";
        for (const auto& state : entity.states)
        {
            out << "\t\t\t\t{\n";
            out << "\t\t\t\t\tName: " << go_string(state.name) << ",\n";
            out << "\t\t\t\t\tTerminal: " << (state.terminal ? "true" : "false") << ",\n";
            if (state.garbage_collection.has_value())
            {
                out << "\t\t\t\t\tGarbageCollection: &GarbageCollectionPolicy{After: "
                    << go_string(state.garbage_collection->after)
                    << ", Mode: " << go_string(state.garbage_collection->mode) << "},\n";
            }
            else
            {
                out << "\t\t\t\t\tGarbageCollection: nil,\n";
            }
            out << "\t\t\t\t},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tInitialState: " << string_ptr_expr(entity.initial_state) << ",\n";
        out << "\t\t\tTerminalStates: []string{";
        for (std::size_t i = 0; i < entity.terminal_states.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_string(entity.terminal_states[i]);
        }
        out << "},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func CollectionDescriptors() []CollectionDescriptor {\n";
    out << "\treturn []CollectionDescriptor{\n";

    for (const auto& entity : system.entities)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(entity.name) << ",\n";
        out << "\t\t\tFields: []FieldDescriptor{\n";
        for (const auto& field : entity.fields)
        {
            out << "\t\t\t\t" << go_field_descriptor_expr(field) << ",\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tKeyFields: []string{";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_string(entity.key_fields[i]);
        }
        out << "},\n";
        out << "\t\t\tIndexes: []IndexDescriptor{\n";
        for (const auto& index : entity.indexes)
        {
            out << "\t\t\t\t{\n";
            out << "\t\t\t\t\tName: " << go_string(index.name) << ",\n";
            out << "\t\t\t\t\tFields: []string{";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << go_string(index.fields[i]);
            }
            out << "},\n";
            out << "\t\t\t\t\tUnique: " << (index.unique ? "true" : "false") << ",\n";
            out << "\t\t\t\t},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tSchemaVersion: 1,\n";
        out << "\t\t},\n";
    }

    out << "\t}\n";
    out << "}\n\n";

    out << "func QueueDefinitions() []QueueDefinition {\n";
    out << "\treturn []QueueDefinition{\n";
    for (const auto& queue : system.queues)
    {
        out << "\t\t{\n";
        out << "\t\t\tQueue: " << go_string(queue.name) << ",\n";
        out << "\t\t\tChannel: " << go_string(queue.channel.value_or("default")) << ",\n";
        out << "\t\t\tVisibilityTimeout: " << parse_duration_seconds(queue.visibility_timeout)
            << " * time.Second,\n";
        out << "\t\t\tMaxAttempts: " << queue.max_attempts.value_or(1) << ",\n";
        out << "\t\t\tDeadLetterQueue: " << string_ptr_expr(queue.dead_letter) << ",\n";
        out << "\t\t\tMetadata: JSONObject(map[string]JSON{}),\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func LeaseDefinitions() []LeaseDescriptor {\n";
    out << "\treturn []LeaseDescriptor{\n";
    for (const auto& lease : system.leases)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(lease.name) << ",\n";
        out << "\t\t\tResource: " << string_ptr_expr(lease.resource) << ",\n";
        out << "\t\t\tTTL: " << parse_duration_seconds(lease.ttl) << " * time.Second,\n";
        out << "\t\t\tRenewEvery: " << duration_ptr_expr(lease.renew_every) << ",\n";
        out << "\t\t\tHolder: " << string_ptr_expr(lease.holder) << ",\n";
        out << "\t\t\tFencingToken: " << (lease.fencing_token.value_or(false) ? "true" : "false")
            << ",\n";
        out << "\t\t\tMaxTTL: " << duration_ptr_expr(lease.max_ttl) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func WorkflowDefinitions() []WorkflowDefinition {\n";
    out << "\treturn []WorkflowDefinition{\n";
    for (const auto& workflow : system.workflows)
    {
        out << "\t\t{\n";
        out << "\t\t\tWorkflowName: " << go_string(workflow.name) << ",\n";
        out << "\t\t\tWorkflowVersion: " << workflow.version.value_or(1) << ",\n";
        out << "\t\t\tStartStep: " << go_string(workflow.start_step.value_or("")) << ",\n";
        out << "\t\t\tExpectedExecutionTime: "
            << parse_duration_seconds(workflow.expected_execution_time) << " * time.Second,\n";
        out << "\t\t\tSingleton: " << (workflow.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "\t\t\tSteps: []WorkflowStepDefinition{\n";
        for (const auto& step : workflow.steps)
        {
            out << "\t\t\t\t{Name: " << go_string(step.name) << ", ExpectedExecutionTime: "
                << parse_duration_seconds(step.expected_execution_time)
                << " * time.Second, MaxRetries: " << step.max_retries.value_or(0) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tMetadata: JSONObject(map[string]JSON{}),\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n";

    out << "\nfunc logLevelFromDescriptor(level string) LogLevel {\n";
    out << "\tswitch level {\n";
    out << "\tcase \"debug\":\n";
    out << "\t\treturn LogLevelDebug\n";
    out << "\tcase \"warn\":\n";
    out << "\t\treturn LogLevelWarn\n";
    out << "\tcase \"error\":\n";
    out << "\t\treturn LogLevelError\n";
    out << "\tdefault:\n";
    out << "\t\treturn LogLevelInfo\n";
    out << "\t}\n";
    out << "}\n\n";

    out << "func metricKindFromDescriptor(kind string) MetricKind {\n";
    out << "\tswitch kind {\n";
    out << "\tcase \"gauge\":\n";
    out << "\t\treturn MetricGauge\n";
    out << "\tcase \"histogram\":\n";
    out << "\t\treturn MetricHistogram\n";
    out << "\tdefault:\n";
    out << "\t\treturn MetricCounter\n";
    out << "\t}\n";
    out << "}\n\n";

    out << "func featureFlagTypeFromDescriptor(flagType string) FeatureFlagType {\n";
    out << "\tswitch flagType {\n";
    out << "\tcase \"string\":\n";
    out << "\t\treturn FeatureFlagString\n";
    out << "\tcase \"int\":\n";
    out << "\t\treturn FeatureFlagInteger\n";
    out << "\tcase \"decimal\":\n";
    out << "\t\treturn FeatureFlagDecimal\n";
    out << "\tdefault:\n";
    out << "\t\treturn FeatureFlagBool\n";
    out << "\t}\n";
    out << "}\n\n";

    out << "func featureFlagScopeFromDescriptor(scope string) FeatureFlagScopeKind {\n";
    out << "\tswitch {\n";
    out << "\tcase scope == \"system\":\n";
    out << "\t\treturn FeatureFlagScopeSystem\n";
    out << "\tcase scope == \"user\":\n";
    out << "\t\treturn FeatureFlagScopeUser\n";
    out << "\tcase strings.HasPrefix(scope, \"entity \"):\n";
    out << "\t\treturn FeatureFlagScopeEntity\n";
    out << "\tdefault:\n";
    out << "\t\treturn FeatureFlagScopeTenant\n";
    out << "\t}\n";
    out << "}\n\n";

    out << "func featureFlagValueFromDescriptor(definition FeatureFlagDescriptor) FeatureFlagValue "
           "{\n";
    out << "\tswitch definition.Type {\n";
    out << "\tcase \"string\":\n";
    out << "\t\treturn StringFeatureFlagValue(definition.DefaultValue)\n";
    out << "\tcase \"int\":\n";
    out << "\t\tvalue, _ := strconv.ParseInt(definition.DefaultValue, 10, 64)\n";
    out << "\t\treturn IntegerFeatureFlagValue(value)\n";
    out << "\tcase \"decimal\":\n";
    out << "\t\tvalue, _ := strconv.ParseFloat(definition.DefaultValue, 64)\n";
    out << "\t\treturn DecimalFeatureFlagValue(value)\n";
    out << "\tdefault:\n";
    out << "\t\treturn BoolFeatureFlagValue(definition.DefaultValue == \"true\")\n";
    out << "\t}\n";
    out << "}\n\n";

    out << "func leaseDefinitionFromDescriptor(definition LeaseDescriptor) LeaseDefinition {\n";
    out << "\tresourcePattern := definition.Name\n";
    out << "\tif definition.Resource != nil {\n";
    out << "\t\tresourcePattern = *definition.Resource\n";
    out << "\t}\n";
    out << "\trenewEvery := definition.TTL\n";
    out << "\tif definition.RenewEvery != nil {\n";
    out << "\t\trenewEvery = *definition.RenewEvery\n";
    out << "\t}\n";
    out << "\treturn LeaseDefinition{\n";
    out << "\t\tID: LeaseDefinitionID{Name: definition.Name, Version: 1},\n";
    out << "\t\tResourcePattern: resourcePattern,\n";
    out << "\t\tTTL: definition.TTL,\n";
    out << "\t\tRenewEvery: renewEvery,\n";
    out << "\t\tMaxTTL: definition.MaxTTL,\n";
    out << "\t\tFencingToken: definition.FencingToken,\n";
    out << "\t}\n";
    out << "}\n\n";

    out << "func EnsureSystemCollections(ctx context.Context, backend Backend) error {\n";
    out << "\treturn backend.EnsureCollections(ctx, CollectionDescriptors())\n";
    out << "}\n\n";

    out << "func RegisterFeatureFlagDefinitionsTx(ctx context.Context, tx Transaction, store "
           "FeatureFlagStore) error {\n";
    out << "\tfor _, definition := range FeatureFlagDefinitions() {\n";
    out << "\t\t_, err := store.RegisterDefinitionTx(ctx, tx, FeatureFlagDefinition{\n";
    out << "\t\t\tName: definition.Name,\n";
    out << "\t\t\tType: featureFlagTypeFromDescriptor(definition.Type),\n";
    out << "\t\t\tDefaultValue: featureFlagValueFromDescriptor(definition),\n";
    out << "\t\t\tScope: featureFlagScopeFromDescriptor(definition.Scope),\n";
    out << "\t\t\tOwner: definition.Owner,\n";
    out << "\t\t\tDescription: definition.Description,\n";
    out << "\t\t\tExpires: definition.Expires,\n";
    out << "\t\t})\n";
    out << "\t\tif err != nil {\n";
    out << "\t\t\treturn err\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil\n";
    out << "}\n\n";

    out << "func CreateQueueDefinitionsTx(ctx context.Context, tx Transaction, store QueueStore) "
           "error {\n";
    out << "\tfor _, definition := range QueueDefinitions() {\n";
    out << "\t\t_, err := store.CreateTx(ctx, tx, CreateQueueRequest{Definition: definition})\n";
    out << "\t\tif err != nil {\n";
    out << "\t\t\treturn err\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil\n";
    out << "}\n\n";

    out << "func RegisterLeaseDefinitionsTx(ctx context.Context, tx Transaction, store LeaseStore) "
           "error {\n";
    out << "\tfor _, definition := range LeaseDefinitions() {\n";
    out << "\t\t_, err := store.RegisterDefinitionTx(ctx, tx, "
           "leaseDefinitionFromDescriptor(definition))\n";
    out << "\t\tif err != nil {\n";
    out << "\t\t\treturn err\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil\n";
    out << "}\n\n";

    out << "func RegisterLogDefinitionsTx(ctx context.Context, tx Transaction, sink LogSink) error "
           "{\n";
    out << "\tfor _, definition := range LogDefinitions() {\n";
    out << "\t\t_, err := sink.RegisterDefinitionTx(ctx, tx, LogSignalDefinition{\n";
    out << "\t\t\tName: definition.Name,\n";
    out << "\t\t\tLevel: logLevelFromDescriptor(definition.Level),\n";
    out << "\t\t\tEventName: definition.EventName,\n";
    out << "\t\t\tFields: definition.Fields,\n";
    out << "\t\t})\n";
    out << "\t\tif err != nil {\n";
    out << "\t\t\treturn err\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil\n";
    out << "}\n\n";

    out << "func RegisterMetricDefinitionsTx(ctx context.Context, tx Transaction, sink MetricSink) "
           "error {\n";
    out << "\tfor _, definition := range MetricDefinitions() {\n";
    out << "\t\t_, err := sink.RegisterDefinitionTx(ctx, tx, MetricInstrumentDefinition{\n";
    out << "\t\t\tName: definition.Name,\n";
    out << "\t\t\tKind: metricKindFromDescriptor(definition.Kind),\n";
    out << "\t\t\tBackendName: definition.BackendName,\n";
    out << "\t\t\tUnit: definition.Unit,\n";
    out << "\t\t\tLabels: definition.Labels,\n";
    out << "\t\t})\n";
    out << "\t\tif err != nil {\n";
    out << "\t\t\treturn err\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil\n";
    out << "}\n\n";

    out << "func RegisterObservabilityCatalogTx(ctx context.Context, tx Transaction, logSink "
           "LogSink, metricSink MetricSink) error {\n";
    out << "\tif err := RegisterLogDefinitionsTx(ctx, tx, logSink); err != nil {\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\treturn RegisterMetricDefinitionsTx(ctx, tx, metricSink)\n";
    out << "}\n\n";

    out << "func RegisterWorkflowDefinitionsTx(ctx context.Context, tx Transaction, store "
           "WorkflowStore) error {\n";
    out << "\tfor _, definition := range WorkflowDefinitions() {\n";
    out << "\t\t_, err := store.RegisterDefinitionTx(ctx, tx, "
           "RegisterWorkflowDefinitionRequest{Definition: definition})\n";
    out << "\t\tif err != nil {\n";
    out << "\t\t\treturn err\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn nil\n";
    out << "}\n\n";

    out << "func RegisterRuntimeCatalogTx(ctx context.Context, tx Transaction, featureFlagStore "
           "FeatureFlagStore, queueStore QueueStore, leaseStore LeaseStore, workflowStore "
           "WorkflowStore, logSink LogSink, metricSink MetricSink) error {\n";
    out << "\tif err := RegisterFeatureFlagDefinitionsTx(ctx, tx, featureFlagStore); err != nil "
           "{\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\tif err := CreateQueueDefinitionsTx(ctx, tx, queueStore); err != nil {\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\tif err := RegisterLeaseDefinitionsTx(ctx, tx, leaseStore); err != nil {\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\tif err := RegisterWorkflowDefinitionsTx(ctx, tx, workflowStore); err != nil {\n";
    out << "\t\treturn err\n";
    out << "\t}\n";
    out << "\treturn RegisterObservabilityCatalogTx(ctx, tx, logSink, metricSink)\n";
    out << "}\n";
    return out.str();
}

std::string generate_api_descriptors_go()
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import common \"statespec-generated/common/backend\"\n\n";
    out << "type APITierDescriptor = common.ApiDescriptor\n";
    out << "type APITierServerDescriptor = common.ApiServerDescriptor\n";
    out << "\n";
    out << "func APITierDescriptors() []common.ApiDescriptor {\n";
    out << "\treturn common.ApiDescriptors()\n";
    out << "}\n\n";
    out << "func APITierServerDescriptors() []common.ApiServerDescriptor {\n";
    out << "\treturn common.ApiServerDescriptors()\n";
    out << "}\n\n";
    return out.str();
}

std::string generate_api_handlers_go()
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import common \"statespec-generated/common/backend\"\n\n";
    out << "type APITierHandler = common.APIHandler\n";
    out << "type APITierRequestContext = common.APIRequestContext\n";
    out << "type APITierResponse = common.APIResponse\n";
    return out.str();
}

std::string generate_api_routes_go()
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import common \"statespec-generated/common/backend\"\n\n";
    out << "type APITierRouteDescriptor = common.ApiRouteDescriptor\n\n";
    out << "func APITierRouteDescriptors() []common.ApiRouteDescriptor {\n";
    out << "\treturn common.ApiRouteDescriptors()\n";
    out << "}\n";
    return out.str();
}

std::string generate_external_system_operator_metadata_api_go()
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import common \"statespec-generated/common/backend\"\n\n";
    out << "type APITierExternalSystemOperatorMetadataHandler = "
           "common.ExternalSystemOperatorMetadataAPIHandler\n";
    return out.str();
}

std::string generate_worker_descriptors_go()
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import common \"statespec-generated/common/backend\"\n\n";
    out << "type WorkerTierDescriptor = common.WorkerDescriptor\n";
    out << "func WorkerTierDescriptors() []common.WorkerDescriptor {\n";
    out << "\treturn common.WorkerDescriptors()\n";
    out << "}\n";
    return out.str();
}

std::string generate_worker_contexts_go()
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import common \"statespec-generated/common/backend\"\n\n";
    out << "type WorkerTierContext = common.WorkerContext\n\n";
    out << "func WorkerTierContexts() []common.WorkerContext {\n";
    out << "\treturn common.WorkerContexts()\n";
    out << "}\n";
    return out.str();
}

std::string generate_worker_handlers_go()
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import common \"statespec-generated/common/backend\"\n\n";
    out << "type WorkerTierHandler = common.Worker\n";
    return out.str();
}

std::string generate_worker_queues_go()
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import (\n";
    out << "\t\"context\"\n\n";
    out << "\tcommon \"statespec-generated/common/backend\"\n";
    out << ")\n\n";
    out << "func WorkerTierQueueDefinitions() []common.QueueDefinition {\n";
    out << "\treturn common.QueueDefinitions()\n";
    out << "}\n\n";
    out << "func WorkerTierCreateQueueDefinitionsTx(ctx context.Context, tx common.Transaction, "
           "store common.QueueStore) error {\n";
    out << "\treturn common.CreateQueueDefinitionsTx(ctx, tx, store)\n";
    out << "}\n\n";
    return out.str();
}

std::string generate_worker_leases_go()
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import (\n";
    out << "\t\"context\"\n\n";
    out << "\tcommon \"statespec-generated/common/backend\"\n";
    out << ")\n\n";
    out << "func WorkerTierLeaseDefinitions() []common.LeaseDescriptor {\n";
    out << "\treturn common.LeaseDefinitions()\n";
    out << "}\n\n";
    out << "func WorkerTierRegisterLeaseDefinitionsTx(ctx context.Context, tx common.Transaction, "
           "store common.LeaseStore) error {\n";
    out << "\treturn common.RegisterLeaseDefinitionsTx(ctx, tx, store)\n";
    out << "}\n\n";
    return out.str();
}

std::string generate_worker_workflows_go()
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import (\n";
    out << "\t\"context\"\n\n";
    out << "\tcommon \"statespec-generated/common/backend\"\n";
    out << ")\n\n";
    out << "func WorkerTierWorkflowDefinitions() []common.WorkflowDefinition {\n";
    out << "\treturn common.WorkflowDefinitions()\n";
    out << "}\n\n";
    out << "func WorkerTierRegisterWorkflowDefinitionsTx(ctx context.Context, tx "
           "common.Transaction, "
           "store common.WorkflowStore) error {\n";
    out << "\treturn common.RegisterWorkflowDefinitionsTx(ctx, tx, store)\n";
    out << "}\n";
    return out.str();
}

} // namespace

GenerationResult generate_go_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/go/backend"};

    add_template_file(
        result, options.output_dir, template_root / "json.go", "backend/json.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "backend.go", "backend/backend.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "external_system.go",
        "backend/external_system.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "feature_flag.go", "backend/feature_flag.go",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "lease.go", "backend/lease.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "log.go", "backend/log.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "metric.go", "backend/metric.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "queue.go", "backend/queue.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "workflow.go", "backend/workflow.go",
        diagnostics
    );

    if (!diagnostics.has_errors())
    {
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "common/backend/descriptors.go").string(),
                generate_descriptors_go(system),
                GeneratedArtifactTier::Common,
                "common/backend/descriptors.go",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "go.mod").string(),
                generate_go_mod(),
                GeneratedArtifactTier::Common,
                "common/go.mod",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "api/backend/api_descriptors.go").string(),
                generate_api_descriptors_go(),
                GeneratedArtifactTier::Api,
                "api/backend/api_descriptors.go",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "api/backend/api_handlers.go").string(),
                generate_api_handlers_go(),
                GeneratedArtifactTier::Api,
                "api/backend/api_handlers.go",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "api/backend/api_routes.go").string(),
                generate_api_routes_go(),
                GeneratedArtifactTier::Api,
                "api/backend/api_routes.go",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "api/backend/external_system_operator_metadata_api.go")
                    .string(),
                generate_external_system_operator_metadata_api_go(),
                GeneratedArtifactTier::Api,
                "api/backend/external_system_operator_metadata_api.go",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "worker/backend/worker_descriptors.go").string(),
                generate_worker_descriptors_go(),
                GeneratedArtifactTier::Worker,
                "worker/backend/worker_descriptors.go",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "worker/backend/worker_contexts.go").string(),
                generate_worker_contexts_go(),
                GeneratedArtifactTier::Worker,
                "worker/backend/worker_contexts.go",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "worker/backend/worker_handlers.go").string(),
                generate_worker_handlers_go(),
                GeneratedArtifactTier::Worker,
                "worker/backend/worker_handlers.go",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "worker/backend/worker_queues.go").string(),
                generate_worker_queues_go(),
                GeneratedArtifactTier::Worker,
                "worker/backend/worker_queues.go",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "worker/backend/worker_leases.go").string(),
                generate_worker_leases_go(),
                GeneratedArtifactTier::Worker,
                "worker/backend/worker_leases.go",
            }
        );
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "worker/backend/worker_workflows.go").string(),
                generate_worker_workflows_go(),
                GeneratedArtifactTier::Worker,
                "worker/backend/worker_workflows.go",
            }
        );
    }

    return result;
}

} // namespace statespec
