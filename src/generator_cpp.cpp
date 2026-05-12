#include "statespec/generator_cpp.hpp"

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

std::string generate_system_descriptors_header(const Spec& spec)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"backend.hpp\"\n\n";
    out << "#include <vector>\n\n";
    out << "namespace statespec_generated\n";
    out << "{\n\n";
    out << "inline std::vector<statespec::backend::CollectionDescriptor> collection_descriptors()\n";
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
                out << "                statespec::backend::FieldDescriptor{" << cpp_string(field.name)
                    << ", " << cpp_string(strip_optional_suffix(field.type)) << ", "
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

    add_template_file(result, options.output_dir, template_root / "backend.hpp", "backend.hpp", diagnostics);
    add_template_file(result, options.output_dir, template_root / "lease.hpp", "lease.hpp", diagnostics);
    add_template_file(result, options.output_dir, template_root / "queue.hpp", "queue.hpp", diagnostics);
    add_template_file(result, options.output_dir, template_root / "workflow.hpp", "workflow.hpp", diagnostics);

    if (!diagnostics.has_errors())
    {
        result.files.push_back(GeneratedFile{
            (options.output_dir / "system_descriptors.hpp").string(),
            generate_system_descriptors_header(spec),
        });
    }

    return result;
}

} // namespace statespec
