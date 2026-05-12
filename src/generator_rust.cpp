#include "statespec/generator_rust.hpp"

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

std::string rust_string(const std::string& value)
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

std::string generate_descriptors_rs(const Spec& spec)
{
    std::ostringstream out;
    out << "use crate::backend::{CollectionDescriptor, FieldDescriptor};\n\n";
    out << "pub fn collection_descriptors() -> Vec<CollectionDescriptor> {\n";
    out << "    vec![\n";

    if (spec.system.has_value())
    {
        for (const auto& entity : spec.system->entities)
        {
            out << "        CollectionDescriptor {\n";
            out << "            name: " << rust_string(entity.name) << ".to_string(),\n";
            out << "            fields: vec![\n";
            for (const auto& field : entity.fields)
            {
                out << "                FieldDescriptor { name: " << rust_string(field.name)
                    << ".to_string(), field_type: " << rust_string(strip_optional_suffix(field.type))
                    << ".to_string(), required: " << (is_optional_type(field.type) ? "false" : "true")
                    << " },\n";
            }
            out << "            ],\n";
            out << "            key_fields: vec![";
            for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_string(entity.key_fields[i]) << ".to_string()";
            }
            out << "],\n";
            out << "            indexes: vec![],\n";
            out << "            schema_version: 1,\n";
            out << "        },\n";
        }
    }

    out << "    ]\n";
    out << "}\n";
    return out.str();
}

} // namespace

GenerationResult generate_rust_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const std::filesystem::path template_root{"bindings/rust"};

    add_template_file(result, options.output_dir, template_root / "backend.rs", "backend.rs", diagnostics);
    add_template_file(result, options.output_dir, template_root / "lease.rs", "lease.rs", diagnostics);
    add_template_file(result, options.output_dir, template_root / "queue.rs", "queue.rs", diagnostics);
    add_template_file(result, options.output_dir, template_root / "workflow.rs", "workflow.rs", diagnostics);

    if (!diagnostics.has_errors())
    {
        result.files.push_back(GeneratedFile{
            (options.output_dir / "descriptors.rs").string(),
            generate_descriptors_rs(spec),
        });
    }

    return result;
}

} // namespace statespec
