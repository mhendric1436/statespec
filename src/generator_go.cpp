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

std::string generate_descriptors_go(const Spec& spec)
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import \"time\"\n\n";
    out << "type LeaseDefinition struct {\n";
    out << "\tName string\n";
    out << "\tResource *string\n";
    out << "\tTTL time.Duration\n";
    out << "\tRenewEvery *time.Duration\n";
    out << "\tHolder *string\n";
    out << "\tFencingToken bool\n";
    out << "\tMaxTTL *time.Duration\n";
    out << "}\n\n";
    out << "func stringPtr(value string) *string { return &value }\n";
    out << "func durationPtr(value time.Duration) *time.Duration { return &value }\n\n";

    out << "func CollectionDescriptors() []CollectionDescriptor {\n";
    out << "\treturn []CollectionDescriptor{\n";

    if (spec.system.has_value())
    {
        for (const auto& entity : spec.system->entities)
        {
            out << "\t\t{\n";
            out << "\t\t\tName: " << go_string(entity.name) << ",\n";
            out << "\t\t\tFields: []FieldDescriptor{\n";
            for (const auto& field : entity.fields)
            {
                out << "\t\t\t\t{Name: " << go_string(field.name)
                    << ", Type: " << go_string(strip_optional_suffix(field.type))
                    << ", Required: " << (is_optional_type(field.type) ? "false" : "true")
                    << "},\n";
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
            out << "\t\t\tIndexes: []IndexDescriptor{},\n";
            out << "\t\t\tSchemaVersion: 1,\n";
            out << "\t\t},\n";
        }
    }

    out << "\t}\n";
    out << "}\n\n";

    out << "func QueueDefinitions() []QueueDefinition {\n";
    out << "\treturn []QueueDefinition{\n";
    if (spec.system.has_value())
    {
        for (const auto& queue : spec.system->queues)
        {
            out << "\t\t{\n";
            out << "\t\t\tQueue: " << go_string(queue.name) << ",\n";
            out << "\t\t\tChannel: " << go_string(queue.channel.value_or("default")) << ",\n";
            out << "\t\t\tVisibilityTimeout: " << parse_duration_seconds(queue.visibility_timeout)
                << " * time.Second,\n";
            out << "\t\t\tMaxAttempts: " << queue.max_attempts.value_or(1) << ",\n";
            out << "\t\t\tDeadLetterQueue: " << string_ptr_expr(queue.dead_letter) << ",\n";
            out << "\t\t\tMetadata: JSON(`{}`),\n";
            out << "\t\t},\n";
        }
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func LeaseDefinitions() []LeaseDefinition {\n";
    out << "\treturn []LeaseDefinition{\n";
    if (spec.system.has_value())
    {
        for (const auto& lease : spec.system->leases)
        {
            out << "\t\t{\n";
            out << "\t\t\tName: " << go_string(lease.name) << ",\n";
            out << "\t\t\tResource: " << string_ptr_expr(lease.resource) << ",\n";
            out << "\t\t\tTTL: " << parse_duration_seconds(lease.ttl) << " * time.Second,\n";
            out << "\t\t\tRenewEvery: " << duration_ptr_expr(lease.renew_every) << ",\n";
            out << "\t\t\tHolder: " << string_ptr_expr(lease.holder) << ",\n";
            out << "\t\t\tFencingToken: "
                << (lease.fencing_token.value_or(false) ? "true" : "false") << ",\n";
            out << "\t\t\tMaxTTL: " << duration_ptr_expr(lease.max_ttl) << ",\n";
            out << "\t\t},\n";
        }
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func WorkflowDefinitions() []WorkflowDefinition {\n";
    out << "\treturn []WorkflowDefinition{\n";
    if (spec.system.has_value())
    {
        for (const auto& workflow : spec.system->workflows)
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
            out << "\t\t\tMetadata: JSON(`{}`),\n";
            out << "\t\t},\n";
        }
    }
    out << "\t}\n";
    out << "}\n";
    return out.str();
}

} // namespace

GenerationResult generate_go_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/go/backend"};

    add_template_file(
        result, options.output_dir, template_root / "backend.go", "backend/backend.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "lease.go", "backend/lease.go", diagnostics
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
                generate_descriptors_go(spec),
            }
        );
    }

    return result;
}

} // namespace statespec
