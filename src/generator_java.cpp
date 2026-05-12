#include "statespec/generator_java.hpp"

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

std::string generate_descriptors_java(const Spec& spec)
{
    std::ostringstream out;
    out << "package com.statespec.generated;\n\n";
    out << "import com.statespec.backend.Backend.CollectionDescriptor;\n";
    out << "import com.statespec.backend.Backend.FieldDescriptor;\n";
    out << "import java.util.List;\n\n";
    out << "public final class Descriptors {\n";
    out << "    private Descriptors() {}\n\n";
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
