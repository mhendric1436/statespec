#include "statespec/generator_go.hpp"

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

std::string generate_descriptors_go(const Spec& spec)
{
    std::ostringstream out;
    out << "package backend\n\n";
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
                out << "\t\t\t\t{Name: " << go_string(field.name) << ", Type: "
                    << go_string(strip_optional_suffix(field.type)) << ", Required: "
                    << (is_optional_type(field.type) ? "false" : "true") << "},\n";
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

    add_template_file(result, options.output_dir, template_root / "backend.go", "backend/backend.go", diagnostics);
    add_template_file(result, options.output_dir, template_root / "lease.go", "backend/lease.go", diagnostics);
    add_template_file(result, options.output_dir, template_root / "queue.go", "backend/queue.go", diagnostics);
    add_template_file(result, options.output_dir, template_root / "workflow.go", "backend/workflow.go", diagnostics);

    if (!diagnostics.has_errors())
    {
        result.files.push_back(GeneratedFile{
            (options.output_dir / "backend/descriptors.go").string(),
            generate_descriptors_go(spec),
        });
    }

    return result;
}

} // namespace statespec
