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
        diagnostics.error(SourceRange{}, "SSPEC5201", "failed to read binding template: " + path.string());
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

    result.files.push_back(GeneratedFile{
        (output_dir / relative_output_path).string(),
        content,
    });
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

std::string optional_duration_expr(const std::optional<std::string>& value)
{
    return value.has_value()
        ? "Optional.of(Duration.ofSeconds(" + std::to_string(parse_duration_seconds(value)) + "L))"
        : "Optional.empty()";
}

std::string generate_descriptors_java(const Spec& spec)
{
    std::ostringstream out;
    out << "package com.statespec.generated;\n\n";
    out << "import com.statespec.backend.Backend.CollectionDescriptor;\n";
    out << "import com.statespec.backend.Backend.FieldDescriptor;\n";
    out << "import com.statespec.backend.Queue.QueueDefinition;\n";
    out << "import com.statespec.backend.Workflow.WorkflowDefinition;\n";
    out << "import com.statespec.backend.Workflow.WorkflowStepDefinition;\n";
    out << "import java.time.Duration;\n";
    out << "import java.util.List;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class Descriptors {\n";
    out << "    private Descriptors() {}\n\n";
    out << "    public record LeaseDefinition(\n";
    out << "        String name,\n";
    out << "        Optional<String> resource,\n";
    out << "        Duration ttl,\n";
    out << "        Optional<Duration> renewEvery,\n";
    out << "        Optional<String> holder,\n";
    out << "        boolean fencingToken,\n";
    out << "        Optional<Duration> maxTtl\n";
    out << "    ) {}\n\n";

    out << "    public static List<CollectionDescriptor> collectionDescriptors() {\n";
    out << "        return List.of(\n";

    if (spec.system.has_value())
    {
        for (std::size_t entity_index = 0; entity_index < spec.system->entities.size(); ++entity_index)
        {
            const auto& entity = spec.system->entities[entity_index];
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
            out << "                List.of(),\n";
            out << "                1L\n";
            out << "            )";
            out << (entity_index + 1 < spec.system->entities.size() ? "," : "") << "\n";
        }
    }

    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<QueueDefinition> queueDefinitions() {\n";
    out << "        return List.of(\n";
    if (spec.system.has_value())
    {
        for (std::size_t i = 0; i < spec.system->queues.size(); ++i)
        {
            const auto& queue = spec.system->queues[i];
            out << "            new QueueDefinition(\n";
            out << "                " << java_string(queue.name) << ",\n";
            out << "                " << java_string(queue.channel.value_or("default")) << ",\n";
            out << "                Duration.ofSeconds(" << parse_duration_seconds(queue.visibility_timeout) << "L),\n";
            out << "                " << queue.max_attempts.value_or(1) << ",\n";
            out << "                " << optional_string_expr(queue.dead_letter) << ",\n";
            out << "                \"{}\"\n";
            out << "            )" << (i + 1 < spec.system->queues.size() ? "," : "") << "\n";
        }
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<LeaseDefinition> leaseDefinitions() {\n";
    out << "        return List.of(\n";
    if (spec.system.has_value())
    {
        for (std::size_t i = 0; i < spec.system->leases.size(); ++i)
        {
            const auto& lease = spec.system->leases[i];
            out << "            new LeaseDefinition(\n";
            out << "                " << java_string(lease.name) << ",\n";
            out << "                " << optional_string_expr(lease.resource) << ",\n";
            out << "                Duration.ofSeconds(" << parse_duration_seconds(lease.ttl) << "L),\n";
            out << "                " << optional_duration_expr(lease.renew_every) << ",\n";
            out << "                " << optional_string_expr(lease.holder) << ",\n";
            out << "                " << (lease.fencing_token.value_or(false) ? "true" : "false") << ",\n";
            out << "                " << optional_duration_expr(lease.max_ttl) << "\n";
            out << "            )" << (i + 1 < spec.system->leases.size() ? "," : "") << "\n";
        }
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<WorkflowDefinition> workflowDefinitions() {\n";
    out << "        return List.of(\n";
    if (spec.system.has_value())
    {
        for (std::size_t i = 0; i < spec.system->workflows.size(); ++i)
        {
            const auto& workflow = spec.system->workflows[i];
            out << "            new WorkflowDefinition(\n";
            out << "                " << java_string(workflow.name) << ",\n";
            out << "                " << workflow.version.value_or(1) << "L,\n";
            out << "                " << java_string(workflow.start_step.value_or("")) << ",\n";
            out << "                Duration.ofSeconds(" << parse_duration_seconds(workflow.expected_execution_time) << "L),\n";
            out << "                " << (workflow.singleton.value_or(false) ? "true" : "false") << ",\n";
            out << "                List.of(\n";
            for (std::size_t step_index = 0; step_index < workflow.steps.size(); ++step_index)
            {
                const auto& step = workflow.steps[step_index];
                out << "                    new WorkflowStepDefinition(" << java_string(step.name)
                    << ", Duration.ofSeconds(" << parse_duration_seconds(step.expected_execution_time)
                    << "L), " << step.max_retries.value_or(0) << ")";
                out << (step_index + 1 < workflow.steps.size() ? "," : "") << "\n";
            }
            out << "                ),\n";
            out << "                \"{}\"\n";
            out << "            )" << (i + 1 < spec.system->workflows.size() ? "," : "") << "\n";
        }
    }
    out << "        );\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

} // namespace

GenerationResult generate_java_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/java/com/statespec/backend"};
    const std::filesystem::path output_root{"com/statespec/backend"};

    add_template_file(result, options.output_dir, template_root / "Backend.java", output_root / "Backend.java", diagnostics);
    add_template_file(result, options.output_dir, template_root / "Lease.java", output_root / "Lease.java", diagnostics);
    add_template_file(result, options.output_dir, template_root / "Queue.java", output_root / "Queue.java", diagnostics);
    add_template_file(result, options.output_dir, template_root / "Workflow.java", output_root / "Workflow.java", diagnostics);

    if (!diagnostics.has_errors())
    {
        result.files.push_back(GeneratedFile{
            (options.output_dir / "com/statespec/generated/Descriptors.java").string(),
            generate_descriptors_java(spec),
        });
    }

    return result;
}

} // namespace statespec
