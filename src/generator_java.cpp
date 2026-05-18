#include "statespec/generator_java.hpp"

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

std::string java_string(const std::string& value)
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

std::string java_shape_type(const std::string& type)
{
    const auto optional = is_optional_type(type);
    const auto base = strip_optional_suffix(type);
    std::string mapped = "String";
    if (base == "bool")
    {
        mapped = "Boolean";
    }
    else if (base == "int" || base == "int32")
    {
        mapped = "Integer";
    }
    else if (base == "int64" || base == "long")
    {
        mapped = "Long";
    }
    else if (base == "double" || base == "decimal")
    {
        mapped = "Double";
    }

    return optional ? "Optional<" + mapped + ">" : mapped;
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
    return value.has_value() ? "Optional.of(" + java_string(*value) + ")" : "Optional.empty()";
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
    return value.has_value() ? "Optional.of(Duration.ofSeconds(" +
                                   std::to_string(parse_duration_seconds(value)) + "L))"
                             : "Optional.empty()";
}

std::string generate_descriptors_java(const IrSystem& system)
{
    std::ostringstream out;
    out << "package com.statespec.generated;\n\n";
    out << "import com.statespec.backend.Backend;\n";
    out << "import com.statespec.backend.FeatureFlag;\n";
    out << "import com.statespec.backend.Json;\n";
    out << "import com.statespec.backend.Lease;\n";
    out << "import com.statespec.backend.Log;\n";
    out << "import com.statespec.backend.Metric;\n";
    out << "import com.statespec.backend.Queue;\n";
    out << "import com.statespec.backend.Workflow;\n";
    out << "import com.statespec.backend.Backend.CollectionDescriptor;\n";
    out << "import com.statespec.backend.Backend.FieldDescriptor;\n";
    out << "import com.statespec.backend.Backend.IndexDescriptor;\n";
    out << "import com.statespec.backend.Queue.QueueDefinition;\n";
    out << "import com.statespec.backend.Workflow.WorkflowDefinition;\n";
    out << "import com.statespec.backend.Workflow.WorkflowStepDefinition;\n";
    out << "import java.time.Duration;\n";
    out << "import java.util.List;\n";
    out << "import java.util.Locale;\n";
    out << "import java.util.Map;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class Descriptors {\n";
    out << "    private Descriptors() {}\n\n";
    out << "    public record EventEnvelope(\n";
    out << "        String name,\n";
    out << "        Map<String, Json> fields\n";
    out << "    ) {}\n\n";
    for (const auto& shape : system.shapes)
    {
        out << "    public record " << pascal_identifier(shape.name) << "(\n";
        for (std::size_t i = 0; i < shape.fields.size(); ++i)
        {
            const auto& field = shape.fields[i];
            out << "        " << java_shape_type(field.type) << " " << field.name;
            out << (i + 1 < shape.fields.size() ? "," : "") << "\n";
        }
        out << "    ) {}\n\n";
    }

    for (const auto& event : system.events)
    {
        out << "    public static EventEnvelope build" << pascal_identifier(event.name)
            << "Event(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "        Json " << lower_camel_identifier(field.name);
            out << (i + 1 < event.fields.size() ? "," : "") << "\n";
        }
        out << "    ) {\n";
        out << "        return new EventEnvelope(\n";
        out << "            " << java_string(event.name) << ",\n";
        out << "            Map.of(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "                " << java_string(field.name) << ", "
                << lower_camel_identifier(field.name);
            out << (i + 1 < event.fields.size() ? "," : "") << "\n";
        }
        out << "            )\n";
        out << "        );\n";
        out << "    }\n\n";
    }

    out << "    public record LeaseDefinition(\n";
    out << "        String name,\n";
    out << "        Optional<String> resource,\n";
    out << "        Duration ttl,\n";
    out << "        Optional<Duration> renewEvery,\n";
    out << "        Optional<String> holder,\n";
    out << "        boolean fencingToken,\n";
    out << "        Optional<Duration> maxTtl\n";
    out << "    ) {}\n\n";
    out << "    public record FeatureFlagDefinition(\n";
    out << "        String name,\n";
    out << "        String type,\n";
    out << "        String defaultValue,\n";
    out << "        String scope,\n";
    out << "        Optional<String> owner,\n";
    out << "        Optional<String> description,\n";
    out << "        Optional<String> expires\n";
    out << "    ) {}\n\n";
    out << "    public record NamespaceDescriptor(\n";
    out << "        String name,\n";
    out << "        List<String> members\n";
    out << "    ) {}\n\n";
    out << "    public record ValueDescriptor(\n";
    out << "        String name,\n";
    out << "        String type,\n";
    out << "        Optional<String> constraint\n";
    out << "    ) {}\n\n";
    out << "    public record EnumMemberDescriptor(\n";
    out << "        String name,\n";
    out << "        Optional<String> value\n";
    out << "    ) {}\n\n";
    out << "    public record EnumDescriptor(\n";
    out << "        String name,\n";
    out << "        List<EnumMemberDescriptor> members\n";
    out << "    ) {}\n\n";
    out << "    public record EventDescriptor(\n";
    out << "        String name,\n";
    out << "        List<FieldDescriptor> fields\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemPropertyDescriptor(\n";
    out << "        String name,\n";
    out << "        String value\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemMetadataDescriptor(\n";
    out << "        String entity,\n";
    out << "        String profileField,\n";
    out << "        List<String> requiredFields\n";
    out << "    ) {}\n\n";
    out << "    public record ExternalSystemDescriptor(\n";
    out << "        String name,\n";
    out << "        List<ExternalSystemPropertyDescriptor> properties,\n";
    out << "        Optional<ExternalSystemMetadataDescriptor> metadata\n";
    out << "    ) {}\n\n";
    out << "    public record ApiDescriptor(\n";
    out << "        String name,\n";
    out << "        Optional<String> method,\n";
    out << "        Optional<String> path,\n";
    out << "        Optional<String> input,\n";
    out << "        Optional<String> output,\n";
    out << "        Optional<String> error,\n";
    out << "        Optional<String> startsWorkflow,\n";
    out << "        Optional<String> enqueues\n";
    out << "    ) {}\n\n";
    out << "    public record ApiServerDescriptor(\n";
    out << "        String name,\n";
    out << "        List<String> serves,\n";
    out << "        int concurrency\n";
    out << "    ) {}\n\n";
    out << "    public record ApiRouteDescriptor(\n";
    out << "        String name,\n";
    out << "        String serverName,\n";
    out << "        String apiName,\n";
    out << "        Optional<String> method,\n";
    out << "        Optional<String> path,\n";
    out << "        Optional<String> input,\n";
    out << "        Optional<String> output,\n";
    out << "        Optional<String> error\n";
    out << "    ) {}\n\n";
    out << "    public record ApiRequestContext(\n";
    out << "        String serverName,\n";
    out << "        String apiName,\n";
    out << "        Optional<String> method,\n";
    out << "        Optional<String> path,\n";
    out << "        Json body\n";
    out << "    ) {}\n\n";
    out << "    public record ApiResponse(\n";
    out << "        int statusCode,\n";
    out << "        Json body\n";
    out << "    ) {}\n\n";
    out << "    public interface ApiHandler {\n";
    out << "        ApiResponse handle(ApiRequestContext context) throws Exception;\n";
    out << "    }\n\n";
    out << "    public record WorkerDescriptor(\n";
    out << "        String name,\n";
    out << "        boolean singleton,\n";
    out << "        Optional<String> lease,\n";
    out << "        Optional<String> polls,\n";
    out << "        Optional<String> executes,\n";
    out << "        int concurrency\n";
    out << "    ) {}\n\n";
    out << "    public record WorkerContext(\n";
    out << "        String workerName,\n";
    out << "        boolean singleton,\n";
    out << "        Optional<String> lease,\n";
    out << "        Optional<String> polls,\n";
    out << "        Optional<String> executes,\n";
    out << "        int concurrency\n";
    out << "    ) {}\n\n";
    out << "    public interface Worker {\n";
    out << "        void run(WorkerContext context) throws Exception;\n";
    out << "    }\n\n";
    out << "    public record PolicyRuleDescriptor(\n";
    out << "        String action,\n";
    out << "        String condition\n";
    out << "    ) {}\n\n";
    out << "    public record QuotaDescriptor(\n";
    out << "        String name,\n";
    out << "        String expression\n";
    out << "    ) {}\n\n";
    out << "    public record PolicyDescriptor(\n";
    out << "        String name,\n";
    out << "        Optional<String> tenantScopedBy,\n";
    out << "        List<PolicyRuleDescriptor> allows,\n";
    out << "        List<PolicyRuleDescriptor> denies,\n";
    out << "        List<QuotaDescriptor> quotas,\n";
    out << "        List<String> audits\n";
    out << "    ) {}\n\n";
    out << "    public record ShapeDescriptor(\n";
    out << "        String name,\n";
    out << "        List<FieldDescriptor> fields\n";
    out << "    ) {}\n\n";
    out << "    public record LogDefinition(\n";
    out << "        String name,\n";
    out << "        String level,\n";
    out << "        String eventName,\n";
    out << "        List<FieldDescriptor> fields\n";
    out << "    ) {}\n\n";
    out << "    public record MetricDefinition(\n";
    out << "        String name,\n";
    out << "        String kind,\n";
    out << "        String backendName,\n";
    out << "        String unit,\n";
    out << "        List<FieldDescriptor> labels\n";
    out << "    ) {}\n\n";
    out << "    public record GarbageCollectionPolicy(\n";
    out << "        String after,\n";
    out << "        String mode\n";
    out << "    ) {}\n\n";
    out << "    public record EntityStateDescriptor(\n";
    out << "        String name,\n";
    out << "        boolean terminal,\n";
    out << "        Optional<GarbageCollectionPolicy> garbageCollection\n";
    out << "    ) {}\n\n";
    out << "    public record EntityOwnershipDescriptor(\n";
    out << "        String authority,\n";
    out << "        String systemOfRecord,\n";
    out << "        String lifecycle\n";
    out << "    ) {}\n\n";
    out << "    public record EntityRelationDescriptor(\n";
    out << "        String kind,\n";
    out << "        String name,\n";
    out << "        String target,\n";
    out << "        boolean optional,\n";
    out << "        Optional<String> relationKind,\n";
    out << "        Optional<String> onParentDelete,\n";
    out << "        List<String> parentMustBeIn,\n";
    out << "        List<String> uniqueWithinParent\n";
    out << "    ) {}\n\n";
    out << "    public record EntityChildDescriptor(\n";
    out << "        String name,\n";
    out << "        String targetEntity,\n";
    out << "        String relation\n";
    out << "    ) {}\n\n";
    out << "    public record EntityInvariantDescriptor(\n";
    out << "        String name,\n";
    out << "        String expression\n";
    out << "    ) {}\n\n";
    out << "    public record EntityDescriptor(\n";
    out << "        String name,\n";
    out << "        List<String> keyFields,\n";
    out << "        Optional<EntityOwnershipDescriptor> ownership,\n";
    out << "        List<EntityRelationDescriptor> relations,\n";
    out << "        List<EntityChildDescriptor> children,\n";
    out << "        List<EntityInvariantDescriptor> invariants,\n";
    out << "        List<EntityStateDescriptor> states,\n";
    out << "        Optional<String> initialState,\n";
    out << "        List<String> terminalStates\n";
    out << "    ) {}\n\n";

    out << "    public static List<FeatureFlagDefinition> featureFlagDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t i = 0; i < system.feature_flags.size(); ++i)
    {
        const auto& flag = system.feature_flags[i];
        out << "            new FeatureFlagDefinition(\n";
        out << "                " << java_string(flag.name) << ",\n";
        out << "                " << java_string(flag.type) << ",\n";
        out << "                " << java_string(flag.default_value) << ",\n";
        out << "                " << java_string(flag.scope) << ",\n";
        out << "                " << optional_string_expr(flag.owner) << ",\n";
        out << "                " << optional_string_expr(flag.description) << ",\n";
        out << "                " << optional_string_expr(flag.expires) << "\n";
        out << "            )" << (i + 1 < system.feature_flags.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<NamespaceDescriptor> namespaceDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t namespace_index = 0; namespace_index < system.namespaces.size();
         ++namespace_index)
    {
        const auto& namespace_decl = system.namespaces[namespace_index];
        out << "            new NamespaceDescriptor(\n";
        out << "                " << java_string(namespace_decl.name) << ",\n";
        out << "                List.of(";
        for (std::size_t i = 0; i < namespace_decl.members.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_string(namespace_decl.members[i]);
        }
        out << ")\n";
        out << "            )" << (namespace_index + 1 < system.namespaces.size() ? "," : "")
            << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<ValueDescriptor> valueDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t value_index = 0; value_index < system.values.size(); ++value_index)
    {
        const auto& value = system.values[value_index];
        out << "            new ValueDescriptor(\n";
        out << "                " << java_string(value.name) << ",\n";
        out << "                " << java_string(value.type) << ",\n";
        out << "                " << optional_string_expr(value.constraint) << "\n";
        out << "            )" << (value_index + 1 < system.values.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<EnumDescriptor> enumDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t enum_index = 0; enum_index < system.enums.size(); ++enum_index)
    {
        const auto& enum_decl = system.enums[enum_index];
        out << "            new EnumDescriptor(\n";
        out << "                " << java_string(enum_decl.name) << ",\n";
        out << "                List.of(\n";
        for (std::size_t member_index = 0; member_index < enum_decl.members.size(); ++member_index)
        {
            const auto& member = enum_decl.members[member_index];
            out << "                    new EnumMemberDescriptor(" << java_string(member.name)
                << ", " << optional_string_expr(member.value) << ")";
            out << (member_index + 1 < enum_decl.members.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (enum_index + 1 < system.enums.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<EventDescriptor> eventDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t event_index = 0; event_index < system.events.size(); ++event_index)
    {
        const auto& event = system.events[event_index];
        out << "            new EventDescriptor(\n";
        out << "                " << java_string(event.name) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "                    new FieldDescriptor(" << java_string(field.name) << ", "
                << java_string(strip_optional_suffix(field.type)) << ", "
                << (is_optional_type(field.type) ? "false" : "true") << ")";
            out << (i + 1 < event.fields.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (event_index + 1 < system.events.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

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
            out << "                    " << java_string(external_system.metadata->profile_field)
                << ",\n";
            out << "                    List.of(";
            for (std::size_t i = 0; i < external_system.metadata->required_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << java_string(external_system.metadata->required_fields[i]);
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

    out << "    public static List<ApiDescriptor> apiDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t api_index = 0; api_index < system.apis.size(); ++api_index)
    {
        const auto& api = system.apis[api_index];
        out << "            new ApiDescriptor(\n";
        out << "                " << java_string(api.name) << ",\n";
        out << "                " << optional_string_expr(api.method) << ",\n";
        out << "                " << optional_string_expr(api.path) << ",\n";
        out << "                " << optional_string_expr(api.input) << ",\n";
        out << "                " << optional_string_expr(api.output) << ",\n";
        out << "                " << optional_string_expr(api.error) << ",\n";
        out << "                " << optional_string_expr(api.starts_workflow) << ",\n";
        out << "                " << optional_string_expr(api.enqueues) << "\n";
        out << "            )" << (api_index + 1 < system.apis.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<ApiServerDescriptor> apiServerDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t server_index = 0; server_index < system.api_servers.size(); ++server_index)
    {
        const auto& api_server = system.api_servers[server_index];
        out << "            new ApiServerDescriptor(\n";
        out << "                " << java_string(api_server.name) << ",\n";
        out << "                List.of(";
        for (std::size_t i = 0; i < api_server.serves.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_string(api_server.serves[i]);
        }
        out << "),\n";
        out << "                " << api_server.concurrency.value_or(1) << "\n";
        out << "            )" << (server_index + 1 < system.api_servers.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    std::size_t api_route_count = 0;
    for (const auto& api_server : system.api_servers)
    {
        api_route_count += api_server.serves.size();
    }
    out << "    public static List<ApiRouteDescriptor> apiRouteDescriptors() {\n";
    out << "        return List.of(\n";
    std::size_t api_route_index = 0;
    for (const auto& api_server : system.api_servers)
    {
        for (const auto& api_name : api_server.serves)
        {
            const auto* api = find_api(system, api_name);
            out << "            new ApiRouteDescriptor(\n";
            out << "                " << java_string(api_server.name + "." + api_name) << ",\n";
            out << "                " << java_string(api_server.name) << ",\n";
            out << "                " << java_string(api_name) << ",\n";
            out << "                "
                << optional_string_expr(optional_api_field(api, &IrApi::method)) << ",\n";
            out << "                " << optional_string_expr(optional_api_field(api, &IrApi::path))
                << ",\n";
            out << "                "
                << optional_string_expr(optional_api_field(api, &IrApi::input)) << ",\n";
            out << "                "
                << optional_string_expr(optional_api_field(api, &IrApi::output)) << ",\n";
            out << "                "
                << optional_string_expr(optional_api_field(api, &IrApi::error)) << "\n";
            out << "            )" << (api_route_index + 1 < api_route_count ? "," : "") << "\n";
            ++api_route_index;
        }
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<WorkerDescriptor> workerDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t worker_index = 0; worker_index < system.workers.size(); ++worker_index)
    {
        const auto& worker = system.workers[worker_index];
        out << "            new WorkerDescriptor(\n";
        out << "                " << java_string(worker.name) << ",\n";
        out << "                " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
        out << "                " << optional_string_expr(worker.lease) << ",\n";
        out << "                " << optional_string_expr(worker.polls) << ",\n";
        out << "                " << optional_string_expr(worker.executes) << ",\n";
        out << "                " << worker.concurrency.value_or(1) << "\n";
        out << "            )" << (worker_index + 1 < system.workers.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<WorkerContext> workerContexts() {\n";
    out << "        return List.of(\n";
    for (std::size_t worker_index = 0; worker_index < system.workers.size(); ++worker_index)
    {
        const auto& worker = system.workers[worker_index];
        out << "            new WorkerContext(\n";
        out << "                " << java_string(worker.name) << ",\n";
        out << "                " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
        out << "                " << optional_string_expr(worker.lease) << ",\n";
        out << "                " << optional_string_expr(worker.polls) << ",\n";
        out << "                " << optional_string_expr(worker.executes) << ",\n";
        out << "                " << worker.concurrency.value_or(1) << "\n";
        out << "            )" << (worker_index + 1 < system.workers.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<PolicyDescriptor> policyDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t policy_index = 0; policy_index < system.policies.size(); ++policy_index)
    {
        const auto& policy = system.policies[policy_index];
        out << "            new PolicyDescriptor(\n";
        out << "                " << java_string(policy.name) << ",\n";
        out << "                " << optional_string_expr(policy.tenant_scoped_by) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < policy.allows.size(); ++i)
        {
            const auto& allow = policy.allows[i];
            out << "                    new PolicyRuleDescriptor(" << java_string(allow.action)
                << ", " << java_string(allow.condition) << ")";
            out << (i + 1 < policy.allows.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < policy.denies.size(); ++i)
        {
            const auto& deny = policy.denies[i];
            out << "                    new PolicyRuleDescriptor(" << java_string(deny.action)
                << ", " << java_string(deny.condition) << ")";
            out << (i + 1 < policy.denies.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < policy.quotas.size(); ++i)
        {
            const auto& quota = policy.quotas[i];
            out << "                    new QuotaDescriptor(" << java_string(quota.name) << ", "
                << java_string(quota.expression) << ")";
            out << (i + 1 < policy.quotas.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                List.of(";
        for (std::size_t i = 0; i < policy.audits.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_string(policy.audits[i]);
        }
        out << ")\n";
        out << "            )" << (policy_index + 1 < system.policies.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<ShapeDescriptor> shapeDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t shape_index = 0; shape_index < system.shapes.size(); ++shape_index)
    {
        const auto& shape = system.shapes[shape_index];
        out << "            new ShapeDescriptor(\n";
        out << "                " << java_string(shape.name) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < shape.fields.size(); ++i)
        {
            const auto& field = shape.fields[i];
            out << "                    new FieldDescriptor(" << java_string(field.name) << ", "
                << java_string(strip_optional_suffix(field.type)) << ", "
                << (is_optional_type(field.type) ? "false" : "true") << ")";
            out << (i + 1 < shape.fields.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (shape_index + 1 < system.shapes.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<LogDefinition> logDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t log_index = 0; log_index < system.logs.size(); ++log_index)
    {
        const auto& log = system.logs[log_index];
        out << "            new LogDefinition(\n";
        out << "                " << java_string(log.name) << ",\n";
        out << "                " << java_string(log.level) << ",\n";
        out << "                " << java_string(log.event_name) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < log.fields.size(); ++i)
        {
            const auto& field = log.fields[i];
            out << "                    new FieldDescriptor(" << java_string(field.name) << ", "
                << java_string(strip_optional_suffix(field.type)) << ", "
                << (is_optional_type(field.type) ? "false" : "true") << ")";
            out << (i + 1 < log.fields.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (log_index + 1 < system.logs.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<MetricDefinition> metricDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t metric_index = 0; metric_index < system.metrics.size(); ++metric_index)
    {
        const auto& metric = system.metrics[metric_index];
        out << "            new MetricDefinition(\n";
        out << "                " << java_string(metric.name) << ",\n";
        out << "                " << java_string(metric.kind) << ",\n";
        out << "                " << java_string(metric.backend_name) << ",\n";
        out << "                " << java_string(metric.unit) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < metric.labels.size(); ++i)
        {
            const auto& label = metric.labels[i];
            out << "                    new FieldDescriptor(" << java_string(label.name) << ", "
                << java_string(strip_optional_suffix(label.type)) << ", "
                << (is_optional_type(label.type) ? "false" : "true") << ")";
            out << (i + 1 < metric.labels.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (metric_index + 1 < system.metrics.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<EntityDescriptor> entityDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t entity_index = 0; entity_index < system.entities.size(); ++entity_index)
    {
        const auto& entity = system.entities[entity_index];
        out << "            new EntityDescriptor(\n";
        out << "                " << java_string(entity.name) << ",\n";
        out << "                List.of(";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_string(entity.key_fields[i]);
        }
        out << "),\n";
        if (entity.ownership.has_value())
        {
            out << "                Optional.of(new EntityOwnershipDescriptor(\n";
            out << "                    " << java_string(entity.ownership->authority) << ",\n";
            out << "                    " << java_string(entity.ownership->system_of_record)
                << ",\n";
            out << "                    " << java_string(entity.ownership->lifecycle) << "\n";
            out << "                )),\n";
        }
        else
        {
            out << "                Optional.empty(),\n";
        }
        out << "                List.of(\n";
        for (std::size_t i = 0; i < entity.relations.size(); ++i)
        {
            const auto& relation = entity.relations[i];
            out << "                    new EntityRelationDescriptor(\n";
            out << "                        " << java_string(relation.kind) << ",\n";
            out << "                        " << java_string(relation.name) << ",\n";
            out << "                        " << java_string(relation.target) << ",\n";
            out << "                        " << (relation.optional ? "true" : "false") << ",\n";
            out << "                        " << optional_string_expr(relation.relation_kind)
                << ",\n";
            out << "                        " << optional_string_expr(relation.on_parent_delete)
                << ",\n";
            out << "                        List.of(";
            for (std::size_t state_index = 0; state_index < relation.parent_must_be_in.size();
                 ++state_index)
            {
                if (state_index > 0)
                {
                    out << ", ";
                }
                out << java_string(relation.parent_must_be_in[state_index]);
            }
            out << "),\n";
            out << "                        List.of(";
            for (std::size_t field_index = 0; field_index < relation.unique_within_parent.size();
                 ++field_index)
            {
                if (field_index > 0)
                {
                    out << ", ";
                }
                out << java_string(relation.unique_within_parent[field_index]);
            }
            out << ")\n";
            out << "                    )" << (i + 1 < entity.relations.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < entity.children.size(); ++i)
        {
            const auto& child = entity.children[i];
            out << "                    new EntityChildDescriptor(" << java_string(child.name)
                << ", " << java_string(child.target_entity) << ", " << java_string(child.relation)
                << ")";
            out << (i + 1 < entity.children.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < entity.invariants.size(); ++i)
        {
            const auto& invariant = entity.invariants[i];
            out << "                    new EntityInvariantDescriptor("
                << java_string(invariant.name) << ", " << java_string(invariant.expression) << ")";
            out << (i + 1 < entity.invariants.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < entity.states.size(); ++i)
        {
            const auto& state = entity.states[i];
            out << "                    new EntityStateDescriptor(\n";
            out << "                        " << java_string(state.name) << ",\n";
            out << "                        " << (state.terminal ? "true" : "false") << ",\n";
            if (state.garbage_collection.has_value())
            {
                out << "                        Optional.of(new GarbageCollectionPolicy("
                    << java_string(state.garbage_collection->after) << ", "
                    << java_string(state.garbage_collection->mode) << "))\n";
            }
            else
            {
                out << "                        Optional.empty()\n";
            }
            out << "                    )" << (i + 1 < entity.states.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                " << optional_string_expr(entity.initial_state) << ",\n";
        out << "                List.of(";
        for (std::size_t i = 0; i < entity.terminal_states.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_string(entity.terminal_states[i]);
        }
        out << ")\n";
        out << "            )" << (entity_index + 1 < system.entities.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<CollectionDescriptor> collectionDescriptors() {\n";
    out << "        return List.of(\n";

    if (!system.entities.empty())
    {
        for (std::size_t entity_index = 0; entity_index < system.entities.size(); ++entity_index)
        {
            const auto& entity = system.entities[entity_index];
            out << "            new CollectionDescriptor(\n";
            out << "                " << java_string(entity.name) << ",\n";
            out << "                List.of(\n";
            for (std::size_t i = 0; i < entity.fields.size(); ++i)
            {
                const auto& field = entity.fields[i];
                out << "                    new FieldDescriptor(" << java_string(field.name) << ", "
                    << java_string(strip_optional_suffix(field.type)) << ", "
                    << (is_optional_type(field.type) ? "false" : "true") << ")";
                out << (i + 1 < entity.fields.size() ? "," : "") << "\n";
            }
            out << "                ),\n";
            out << "                List.of(";
            for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << java_string(entity.key_fields[i]);
            }
            out << "),\n";
            out << "                List.of(\n";
            for (std::size_t i = 0; i < entity.indexes.size(); ++i)
            {
                const auto& index = entity.indexes[i];
                out << "                    new IndexDescriptor(\n";
                out << "                        " << java_string(index.name) << ",\n";
                out << "                        List.of(";
                for (std::size_t field_index = 0; field_index < index.fields.size(); ++field_index)
                {
                    if (field_index > 0)
                    {
                        out << ", ";
                    }
                    out << java_string(index.fields[field_index]);
                }
                out << "),\n";
                out << "                        " << (index.unique ? "true" : "false") << "\n";
                out << "                    )" << (i + 1 < entity.indexes.size() ? "," : "")
                    << "\n";
            }
            out << "                ),\n";
            out << "                1L\n";
            out << "            )";
            out << (entity_index + 1 < system.entities.size() ? "," : "") << "\n";
        }
    }

    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<QueueDefinition> queueDefinitions() {\n";
    out << "        return List.of(\n";
    if (!system.queues.empty())
    {
        for (std::size_t i = 0; i < system.queues.size(); ++i)
        {
            const auto& queue = system.queues[i];
            out << "            new QueueDefinition(\n";
            out << "                " << java_string(queue.name) << ",\n";
            out << "                " << java_string(queue.channel.value_or("default")) << ",\n";
            out << "                Duration.ofSeconds("
                << parse_duration_seconds(queue.visibility_timeout) << "L),\n";
            out << "                " << queue.max_attempts.value_or(1) << ",\n";
            out << "                " << optional_string_expr(queue.dead_letter) << ",\n";
            out << "                \"{}\"\n";
            out << "            )" << (i + 1 < system.queues.size() ? "," : "") << "\n";
        }
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<LeaseDefinition> leaseDefinitions() {\n";
    out << "        return List.of(\n";
    if (!system.leases.empty())
    {
        for (std::size_t i = 0; i < system.leases.size(); ++i)
        {
            const auto& lease = system.leases[i];
            out << "            new LeaseDefinition(\n";
            out << "                " << java_string(lease.name) << ",\n";
            out << "                " << optional_string_expr(lease.resource) << ",\n";
            out << "                Duration.ofSeconds(" << parse_duration_seconds(lease.ttl)
                << "L),\n";
            out << "                " << optional_duration_expr(lease.renew_every) << ",\n";
            out << "                " << optional_string_expr(lease.holder) << ",\n";
            out << "                " << (lease.fencing_token.value_or(false) ? "true" : "false")
                << ",\n";
            out << "                " << optional_duration_expr(lease.max_ttl) << "\n";
            out << "            )" << (i + 1 < system.leases.size() ? "," : "") << "\n";
        }
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<WorkflowDefinition> workflowDefinitions() {\n";
    out << "        return List.of(\n";
    if (!system.workflows.empty())
    {
        for (std::size_t i = 0; i < system.workflows.size(); ++i)
        {
            const auto& workflow = system.workflows[i];
            out << "            new WorkflowDefinition(\n";
            out << "                " << java_string(workflow.name) << ",\n";
            out << "                " << workflow.version.value_or(1) << "L,\n";
            out << "                " << java_string(workflow.start_step.value_or("")) << ",\n";
            out << "                Duration.ofSeconds("
                << parse_duration_seconds(workflow.expected_execution_time) << "L),\n";
            out << "                " << (workflow.singleton.value_or(false) ? "true" : "false")
                << ",\n";
            out << "                List.of(\n";
            for (std::size_t step_index = 0; step_index < workflow.steps.size(); ++step_index)
            {
                const auto& step = workflow.steps[step_index];
                out << "                    new WorkflowStepDefinition(" << java_string(step.name)
                    << ", Duration.ofSeconds("
                    << parse_duration_seconds(step.expected_execution_time) << "L), "
                    << step.max_retries.value_or(0) << ")";
                out << (step_index + 1 < workflow.steps.size() ? "," : "") << "\n";
            }
            out << "                ),\n";
            out << "                \"{}\"\n";
            out << "            )" << (i + 1 < system.workflows.size() ? "," : "") << "\n";
        }
    }
    out << "        );\n";
    out << "    }\n";

    out << "\n    private static Log.Level logLevelFromDescriptor(String level) {\n";
    out << "        return switch (level.toLowerCase(Locale.ROOT)) {\n";
    out << "            case \"debug\" -> Log.Level.DEBUG;\n";
    out << "            case \"warn\" -> Log.Level.WARN;\n";
    out << "            case \"error\" -> Log.Level.ERROR;\n";
    out << "            default -> Log.Level.INFO;\n";
    out << "        };\n";
    out << "    }\n\n";

    out << "    private static Metric.Kind metricKindFromDescriptor(String kind) {\n";
    out << "        return switch (kind.toLowerCase(Locale.ROOT)) {\n";
    out << "            case \"gauge\" -> Metric.Kind.GAUGE;\n";
    out << "            case \"histogram\" -> Metric.Kind.HISTOGRAM;\n";
    out << "            default -> Metric.Kind.COUNTER;\n";
    out << "        };\n";
    out << "    }\n\n";

    out << "    private static FeatureFlag.Type featureFlagTypeFromDescriptor(String type) {\n";
    out << "        return switch (type) {\n";
    out << "            case \"string\" -> FeatureFlag.Type.STRING;\n";
    out << "            case \"int\" -> FeatureFlag.Type.INT;\n";
    out << "            case \"decimal\" -> FeatureFlag.Type.DECIMAL;\n";
    out << "            default -> FeatureFlag.Type.BOOL;\n";
    out << "        };\n";
    out << "    }\n\n";

    out << "    private static FeatureFlag.ScopeKind featureFlagScopeFromDescriptor(String scope) "
           "{\n";
    out << "        if (scope.equals(\"system\")) {\n";
    out << "            return FeatureFlag.ScopeKind.SYSTEM;\n";
    out << "        }\n";
    out << "        if (scope.equals(\"user\")) {\n";
    out << "            return FeatureFlag.ScopeKind.USER;\n";
    out << "        }\n";
    out << "        if (scope.startsWith(\"entity \")) {\n";
    out << "            return FeatureFlag.ScopeKind.ENTITY;\n";
    out << "        }\n";
    out << "        return FeatureFlag.ScopeKind.TENANT;\n";
    out << "    }\n\n";

    out << "    private static FeatureFlag.Value featureFlagValueFromDescriptor(\n";
    out << "        FeatureFlagDefinition definition\n";
    out << "    ) {\n";
    out << "        return switch (definition.type()) {\n";
    out << "            case \"string\" -> new "
           "FeatureFlag.Value.StringValue(definition.defaultValue());\n";
    out << "            case \"int\" -> new "
           "FeatureFlag.Value.IntValue(Long.parseLong(definition.defaultValue()));\n";
    out << "            case \"decimal\" -> new "
           "FeatureFlag.Value.DecimalValue(definition.defaultValue());\n";
    out << "            default -> new "
           "FeatureFlag.Value.BoolValue(Boolean.parseBoolean(definition.defaultValue()));\n";
    out << "        };\n";
    out << "    }\n\n";

    out << "    private static Lease.LeaseDefinition leaseDefinitionFromDescriptor(\n";
    out << "        LeaseDefinition definition\n";
    out << "    ) {\n";
    out << "        return new Lease.LeaseDefinition(\n";
    out << "            new Lease.LeaseDefinitionId(definition.name(), 1L),\n";
    out << "            definition.resource().orElse(definition.name()),\n";
    out << "            definition.ttl(),\n";
    out << "            definition.renewEvery().orElse(definition.ttl()),\n";
    out << "            definition.maxTtl(),\n";
    out << "            definition.fencingToken()\n";
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static void ensureSystemCollections(Backend backend)\n";
    out << "        throws Backend.BackendException {\n";
    out << "        backend.ensureCollections(collectionDescriptors());\n";
    out << "    }\n\n";

    out << "    public static void registerFeatureFlagDefinitionsTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        FeatureFlag store\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        for (var definition : featureFlagDefinitions()) {\n";
    out << "            store.registerDefinitionTx(\n";
    out << "                tx,\n";
    out << "                new FeatureFlag.Definition(\n";
    out << "                    definition.name(),\n";
    out << "                    featureFlagTypeFromDescriptor(definition.type()),\n";
    out << "                    featureFlagValueFromDescriptor(definition),\n";
    out << "                    featureFlagScopeFromDescriptor(definition.scope()),\n";
    out << "                    definition.owner(),\n";
    out << "                    definition.description(),\n";
    out << "                    definition.expires()\n";
    out << "                )\n";
    out << "            );\n";
    out << "        }\n";
    out << "    }\n\n";

    out << "    public static void createQueueDefinitionsTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        Queue store\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        for (var definition : queueDefinitions()) {\n";
    out << "            store.createTx(tx, new Queue.CreateQueueRequest(definition));\n";
    out << "        }\n";
    out << "    }\n\n";

    out << "    public static void registerLeaseDefinitionsTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        Lease store\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        for (var definition : leaseDefinitions()) {\n";
    out << "            store.registerDefinitionTx(tx, "
           "leaseDefinitionFromDescriptor(definition));\n";
    out << "        }\n";
    out << "    }\n\n";

    out << "    public static void registerLogDefinitionsTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        Log sink\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        for (var definition : logDefinitions()) {\n";
    out << "            sink.registerDefinitionTx(\n";
    out << "                tx,\n";
    out << "                new Log.Definition(\n";
    out << "                    definition.name(),\n";
    out << "                    logLevelFromDescriptor(definition.level()),\n";
    out << "                    definition.eventName(),\n";
    out << "                    definition.fields()\n";
    out << "                )\n";
    out << "            );\n";
    out << "        }\n";
    out << "    }\n\n";

    out << "    public static void registerMetricDefinitionsTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        Metric sink\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        for (var definition : metricDefinitions()) {\n";
    out << "            sink.registerDefinitionTx(\n";
    out << "                tx,\n";
    out << "                new Metric.Definition(\n";
    out << "                    definition.name(),\n";
    out << "                    metricKindFromDescriptor(definition.kind()),\n";
    out << "                    definition.backendName(),\n";
    out << "                    definition.unit(),\n";
    out << "                    definition.labels()\n";
    out << "                )\n";
    out << "            );\n";
    out << "        }\n";
    out << "    }\n\n";

    out << "    public static void registerObservabilityCatalogTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        Log logSink,\n";
    out << "        Metric metricSink\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        registerLogDefinitionsTx(tx, logSink);\n";
    out << "        registerMetricDefinitionsTx(tx, metricSink);\n";
    out << "    }\n\n";

    out << "    public static void registerWorkflowDefinitionsTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        Workflow store\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        for (var definition : workflowDefinitions()) {\n";
    out << "            store.registerDefinitionTx(\n";
    out << "                tx,\n";
    out << "                new Workflow.RegisterWorkflowDefinitionRequest(definition)\n";
    out << "            );\n";
    out << "        }\n";
    out << "    }\n";
    out << "\n    public static void registerRuntimeCatalogTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        FeatureFlag featureFlagStore,\n";
    out << "        Queue queueStore,\n";
    out << "        Lease leaseStore,\n";
    out << "        Workflow workflowStore,\n";
    out << "        Log logSink,\n";
    out << "        Metric metricSink\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        registerFeatureFlagDefinitionsTx(tx, featureFlagStore);\n";
    out << "        createQueueDefinitionsTx(tx, queueStore);\n";
    out << "        registerLeaseDefinitionsTx(tx, leaseStore);\n";
    out << "        registerWorkflowDefinitionsTx(tx, workflowStore);\n";
    out << "        registerObservabilityCatalogTx(tx, logSink, metricSink);\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

} // namespace

GenerationResult generate_java_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/java/com/statespec/backend"};
    const std::filesystem::path output_root{"com/statespec/backend"};

    add_template_file(
        result, options.output_dir, template_root / "Json.java", output_root / "Json.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "Backend.java", output_root / "Backend.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "FeatureFlag.java",
        output_root / "FeatureFlag.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "Lease.java", output_root / "Lease.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "Log.java", output_root / "Log.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "Metric.java", output_root / "Metric.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "Queue.java", output_root / "Queue.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "Workflow.java", output_root / "Workflow.java",
        diagnostics
    );

    if (!diagnostics.has_errors())
    {
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "com/statespec/generated/Descriptors.java").string(),
                generate_descriptors_java(system),
            }
        );
    }

    return result;
}

} // namespace statespec
