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
    DiagnosticBag& diagnostics
)
{
    const auto content = read_template_file(template_path, diagnostics);
    if (diagnostics.has_errors())
    {
        return;
    }

    result.files.push_back(
        GeneratedFile{
            (output_dir / relative_output_path).string(),
            content,
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
            out << "\t\t\t\t{Name: " << go_string(field.name)
                << ", Type: " << go_string(strip_optional_suffix(field.type))
                << ", Required: " << (is_optional_type(field.type) ? "false" : "true") << "},\n";
        }
        out << "\t\t\t},\n";
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
            out << "\t\t\t\t{Name: " << go_string(field.name)
                << ", Type: " << go_string(strip_optional_suffix(field.type))
                << ", Required: " << (is_optional_type(field.type) ? "false" : "true") << "},\n";
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
            out << "\t\t\t\t{Name: " << go_string(field.name)
                << ", Type: " << go_string(strip_optional_suffix(field.type))
                << ", Required: " << (is_optional_type(field.type) ? "false" : "true") << "},\n";
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
            out << "\t\t\t\t{Name: " << go_string(label.name)
                << ", Type: " << go_string(strip_optional_suffix(label.type))
                << ", Required: " << (is_optional_type(label.type) ? "false" : "true") << "},\n";
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
            out << "\t\t\t\t{Name: " << go_string(field.name)
                << ", Type: " << go_string(strip_optional_suffix(field.type))
                << ", Required: " << (is_optional_type(field.type) ? "false" : "true") << "},\n";
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
                (options.output_dir / "backend/descriptors.go").string(),
                generate_descriptors_go(system),
            }
        );
    }

    return result;
}

} // namespace statespec
