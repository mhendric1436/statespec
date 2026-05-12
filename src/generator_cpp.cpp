#include "statespec/generator_cpp.hpp"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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

std::string cpp_string(const std::string& value)
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

std::int64_t parse_duration_seconds(const std::optional<std::string>& value)
{
    if (!value.has_value() || value->empty())
    {
        return 0;
    }

    const auto& text = *value;
    if (text.size() >= 3 && text[0] == 'P' && text[1] == 'T')
    {
        std::int64_t total = 0;
        std::int64_t current = 0;
        for (std::size_t i = 2; i < text.size(); ++i)
        {
            const auto ch = text[i];
            if (std::isdigit(static_cast<unsigned char>(ch)))
            {
                current = current * 10 + static_cast<std::int64_t>(ch - '0');
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

    return 0;
}

std::string optional_string_expr(const std::optional<std::string>& value)
{
    if (!value.has_value())
    {
        return "std::nullopt";
    }
    return "std::optional<std::string>{" + cpp_string(*value) + "}";
}

std::string generate_system_descriptors_header(const Spec& spec)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"backend.hpp\"\n";
    out << "#include \"queue.hpp\"\n";
    out << "#include \"workflow.hpp\"\n\n";
    out << "#include <chrono>\n";
    out << "#include <optional>\n";
    out << "#include <string>\n";
    out << "#include <vector>\n\n";
    out << "namespace statespec_generated\n";
    out << "{\n\n";
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

    out << "inline std::vector<statespec::backend::CollectionDescriptor> "
           "collection_descriptors()\n";
    out << "{\n";
    out << "    return {\n";

    if (spec.system.has_value())
    {
        for (const auto& entity : spec.system->entities)
        {
            out << "        statespec::backend::CollectionDescriptor{\n";
            out << "            " << cpp_string(entity.name) << ",\n";
            out << "            {\n";
            for (const auto& field : entity.fields)
            {
                out << "                statespec::backend::FieldDescriptor{"
                    << cpp_string(field.name) << ", "
                    << cpp_string(strip_optional_suffix(field.type)) << ", "
                    << (is_optional_type(field.type) ? "false" : "true") << "},\n";
            }
            out << "            },\n";
            out << "            {";
            for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << cpp_string(entity.key_fields[i]);
            }
            out << "},\n";
            out << "            {},\n";
            out << "            1,\n";
            out << "        },\n";
        }
    }

    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<statespec::backend::QueueDefinition> queue_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    if (spec.system.has_value())
    {
        for (const auto& queue : spec.system->queues)
        {
            out << "        statespec::backend::QueueDefinition{\n";
            out << "            " << cpp_string(queue.name) << ",\n";
            out << "            " << cpp_string(queue.channel.value_or("default")) << ",\n";
            out << "            std::chrono::seconds{"
                << parse_duration_seconds(queue.visibility_timeout) << "},\n";
            out << "            " << queue.max_attempts.value_or(1) << ",\n";
            out << "            " << optional_string_expr(queue.dead_letter) << ",\n";
            out << "            \"{}\",\n";
            out << "        },\n";
        }
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<LeaseDefinition> lease_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    if (spec.system.has_value())
    {
        for (const auto& lease : spec.system->leases)
        {
            out << "        LeaseDefinition{\n";
            out << "            " << cpp_string(lease.name) << ",\n";
            out << "            " << optional_string_expr(lease.resource) << ",\n";
            out << "            std::chrono::seconds{" << parse_duration_seconds(lease.ttl)
                << "},\n";
            if (lease.renew_every.has_value())
            {
                out << "            std::optional<std::chrono::seconds>{std::chrono::seconds{"
                    << parse_duration_seconds(lease.renew_every) << "}},\n";
            }
            else
            {
                out << "            std::nullopt,\n";
            }
            out << "            " << optional_string_expr(lease.holder) << ",\n";
            out << "            " << (lease.fencing_token.value_or(false) ? "true" : "false")
                << ",\n";
            if (lease.max_ttl.has_value())
            {
                out << "            std::optional<std::chrono::seconds>{std::chrono::seconds{"
                    << parse_duration_seconds(lease.max_ttl) << "}},\n";
            }
            else
            {
                out << "            std::nullopt,\n";
            }
            out << "        },\n";
        }
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<statespec::backend::WorkflowDefinition> workflow_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    if (spec.system.has_value())
    {
        for (const auto& workflow : spec.system->workflows)
        {
            out << "        statespec::backend::WorkflowDefinition{\n";
            out << "            " << cpp_string(workflow.name) << ",\n";
            out << "            " << workflow.version.value_or(1) << ",\n";
            out << "            " << cpp_string(workflow.start_step.value_or("")) << ",\n";
            out << "            std::chrono::seconds{"
                << parse_duration_seconds(workflow.expected_execution_time) << "},\n";
            out << "            " << (workflow.singleton.value_or(false) ? "true" : "false")
                << ",\n";
            out << "            {\n";
            for (const auto& step : workflow.steps)
            {
                out << "                statespec::backend::WorkflowStepDefinition{"
                    << cpp_string(step.name) << ", std::chrono::seconds{"
                    << parse_duration_seconds(step.expected_execution_time) << "}, "
                    << step.max_retries.value_or(0) << "},\n";
            }
            out << "            },\n";
            out << "            \"{}\",\n";
            out << "        },\n";
        }
    }
    out << "    };\n";
    out << "}\n\n";

    out << "} // namespace statespec_generated\n";
    return out.str();
}

} // namespace

GenerationResult generate_cpp_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/cpp"};

    add_template_file(
        result, options.output_dir, template_root / "backend.hpp", "backend.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "lease.hpp", "lease.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "queue.hpp", "queue.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, template_root / "workflow.hpp", "workflow.hpp", diagnostics
    );

    if (!diagnostics.has_errors())
    {
        result.files.push_back(
            GeneratedFile{
                (options.output_dir / "system_descriptors.hpp").string(),
                generate_system_descriptors_header(spec),
            }
        );
    }

    return result;
}

} // namespace statespec
