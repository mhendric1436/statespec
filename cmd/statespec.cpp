#include "statespec/diagnostic.hpp"
#include "statespec/generator.hpp"
#include "statespec/lexer.hpp"
#include "statespec/parser.hpp"
#include "statespec/source.hpp"
#include "statespec/token.hpp"
#include "statespec/validator.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

std::string read_file(const std::string& path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("failed to open file: " + path);
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

void print_diagnostics(const statespec::DiagnosticBag& diagnostics)
{
    for (const auto& diagnostic : diagnostics.all())
    {
        const char* severity = "error";
        if (diagnostic.severity == statespec::DiagnosticSeverity::Warning)
        {
            severity = "warning";
        }
        else if (diagnostic.severity == statespec::DiagnosticSeverity::Info)
        {
            severity = "info";
        }

        std::cerr << diagnostic.range.begin.line << ':' << diagnostic.range.begin.column << ": "
                  << severity << ' ' << diagnostic.code << ": " << diagnostic.message << '\n';
    }
}

std::vector<statespec::Token> lex_file(
    const std::string& path,
    statespec::DiagnosticBag& diagnostics
)
{
    statespec::SourceFile source{path, read_file(path)};
    statespec::Lexer lexer{source};
    return lexer.lex(diagnostics);
}

statespec::Spec parse_file(
    const std::string& path,
    statespec::DiagnosticBag& diagnostics
)
{
    auto tokens = lex_file(path, diagnostics);
    statespec::Parser parser{std::move(tokens)};
    return parser.parse(diagnostics);
}

statespec::Spec parse_and_validate_file(
    const std::string& path,
    statespec::DiagnosticBag& diagnostics
)
{
    auto spec = parse_file(path, diagnostics);
    if (!diagnostics.has_errors())
    {
        statespec::Validator validator;
        validator.validate(spec, diagnostics);
    }
    return spec;
}

std::string json_escape(const std::string& value)
{
    std::ostringstream out;
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
    return out.str();
}

void write_json_string(
    std::ostream& out,
    const std::string& value
)
{
    out << '"' << json_escape(value) << '"';
}

void write_json_optional_string(
    std::ostream& out,
    const std::optional<std::string>& value
)
{
    if (value.has_value())
    {
        write_json_string(out, *value);
    }
    else
    {
        out << "null";
    }
}

void write_json_string_array(
    std::ostream& out,
    const std::vector<std::string>& values
)
{
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        write_json_string(out, values[i]);
    }
    out << ']';
}

void write_fields(
    std::ostream& out,
    const std::vector<statespec::FieldDecl>& fields
)
{
    out << '[';
    for (std::size_t i = 0; i < fields.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        out << "{\"name\": ";
        write_json_string(out, fields[i].name);
        out << ", \"type\": ";
        write_json_string(out, fields[i].type);
        out << '}';
    }
    out << ']';
}

void write_spec_json(
    std::ostream& out,
    const statespec::Spec& spec
)
{
    out << "{\n";
    out << "  \"version\": ";
    write_json_optional_string(out, spec.version);
    out << ",\n";
    out << "  \"imports\": [],\n";
    out << "  \"system\": ";

    if (!spec.system.has_value())
    {
        out << "null\n}\n";
        return;
    }

    const auto& system = *spec.system;
    out << "{\n";
    out << "    \"name\": ";
    write_json_string(out, system.name);
    out << ",\n";

    out << "    \"entities\": [";
    for (std::size_t i = 0; i < system.entities.size(); ++i)
    {
        const auto& entity = system.entities[i];
        if (i > 0)
        {
            out << ", ";
        }
        out << "{\"name\": ";
        write_json_string(out, entity.name);
        out << ", \"key_fields\": ";
        write_json_string_array(out, entity.key_fields);
        out << ", \"fields\": ";
        write_fields(out, entity.fields);
        out << '}';
    }
    out << "],\n";

    out << "    \"queues\": [";
    for (std::size_t i = 0; i < system.queues.size(); ++i)
    {
        const auto& queue = system.queues[i];
        if (i > 0)
        {
            out << ", ";
        }
        out << "{\"name\": ";
        write_json_string(out, queue.name);
        out << ", \"messages\": [";
        for (std::size_t j = 0; j < queue.messages.size(); ++j)
        {
            if (j > 0)
            {
                out << ", ";
            }
            out << "{\"name\": ";
            write_json_string(out, queue.messages[j].name);
            out << '}';
        }
        out << "]}";
    }
    out << "],\n";

    out << "    \"leases\": [";
    for (std::size_t i = 0; i < system.leases.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        out << "{\"name\": ";
        write_json_string(out, system.leases[i].name);
        out << '}';
    }
    out << "],\n";

    out << "    \"workers\": [";
    for (std::size_t i = 0; i < system.workers.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        out << "{\"name\": ";
        write_json_string(out, system.workers[i].name);
        out << '}';
    }
    out << "],\n";

    out << "    \"apis\": [";
    for (std::size_t i = 0; i < system.apis.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        out << "{\"name\": ";
        write_json_string(out, system.apis[i].name);
        out << '}';
    }
    out << "],\n";

    out << "    \"workflows\": [";
    for (std::size_t i = 0; i < system.workflows.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        out << "{\"name\": ";
        write_json_string(out, system.workflows[i].name);
        out << '}';
    }
    out << "],\n";

    out << "    \"policies\": [";
    for (std::size_t i = 0; i < system.policies.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        out << "{\"name\": ";
        write_json_string(out, system.policies[i].name);
        out << '}';
    }
    out << "],\n";

    out << "    \"generators\": [";
    for (std::size_t i = 0; i < system.generators.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        out << "{\"target\": ";
        write_json_string(out, system.generators[i].target);
        out << ", \"out\": ";
        write_json_optional_string(out, system.generators[i].out);
        out << '}';
    }
    out << "]\n";

    out << "  }\n";
    out << "}\n";
}

std::string join_path(
    const std::string& left,
    const std::string& right
)
{
    if (left.empty())
    {
        return right;
    }
    return left.back() == '/' ? left + right : left + "/" + right;
}

std::string wf_output_root(
    const statespec::SystemDecl& system,
    const std::optional<std::string>& target,
    const std::optional<std::string>& out
)
{
    if (out.has_value())
    {
        return *out;
    }
    if (target.has_value())
    {
        return "generated/" + *target;
    }
    for (const auto& generator : system.generators)
    {
        if (generator.target == "wf")
        {
            return generator.out.value_or("generated/wf");
        }
    }
    return "generated/wf";
}

bool should_emit_wf_files(
    const statespec::SystemDecl& system,
    const std::optional<std::string>& target
)
{
    if (target.has_value())
    {
        return *target == "wf" || *target == "all";
    }
    for (const auto& generator : system.generators)
    {
        if (generator.target == "wf" || generator.target == "all")
        {
            return true;
        }
    }
    return false;
}

std::string generate_wf_workflows_header()
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include <cstddef>\n";
    out << "#include <string_view>\n\n";
    out << "namespace statespec_generated::wf\n{\n\n";
    out << "struct WorkflowStepMetadata\n{\n";
    out << "    std::string_view name;\n";
    out << "    std::string_view expected_execution_time;\n";
    out << "    int max_retries;\n";
    out << "};\n\n";
    out << "struct WorkflowMetadata\n{\n";
    out << "    std::string_view name;\n";
    out << "    int version;\n";
    out << "    bool singleton;\n";
    out << "    std::string_view expected_execution_time;\n";
    out << "    std::string_view start_step;\n";
    out << "    const WorkflowStepMetadata* steps;\n";
    out << "    std::size_t step_count;\n";
    out << "};\n\n";
    out << "const WorkflowMetadata* workflows();\n";
    out << "std::size_t workflow_count();\n";
    out << "const WorkflowMetadata* find_workflow(std::string_view name);\n";
    out << "const WorkflowStepMetadata* find_step(\n";
    out << "    std::string_view workflow_name,\n";
    out << "    std::string_view step_name\n";
    out << ");\n";
    out << "std::string_view start_step(std::string_view workflow_name);\n\n";
    out << "} // namespace statespec_generated::wf\n";
    return out.str();
}

std::string lower_name(std::string value)
{
    for (auto& ch : value)
    {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string generate_wf_metadata_source(const statespec::SystemDecl& system)
{
    std::ostringstream out;
    out << "#include \"wf_workflows.hpp\"\n\n";
    out << "#include <array>\n\n";
    out << "namespace statespec_generated::wf\n{\n";
    out << "namespace\n{\n\n";

    for (const auto& workflow : system.workflows)
    {
        const auto symbol = lower_name(workflow.name);
        out << "constexpr std::array<WorkflowStepMetadata, " << workflow.steps.size() << "> "
            << symbol << "_steps{{\n";
        for (const auto& step : workflow.steps)
        {
            out << "    WorkflowStepMetadata{\"" << step.name << "\", \""
                << step.expected_execution_time.value_or("") << "\", "
                << step.max_retries.value_or(0) << "},\n";
        }
        out << "}};\n\n";
    }

    out << "constexpr std::array<WorkflowMetadata, " << system.workflows.size()
        << "> all_workflows{{\n";
    for (const auto& workflow : system.workflows)
    {
        const auto symbol = lower_name(workflow.name);
        out << "    WorkflowMetadata{\"" << workflow.name << "\", "
            << workflow.version.value_or(0) << ", "
            << (workflow.singleton.value_or(false) ? "true" : "false") << ", \""
            << workflow.expected_execution_time.value_or("") << "\", \""
            << workflow.start_step.value_or("") << "\", " << symbol << "_steps.data(), "
            << symbol << "_steps.size()},\n";
    }
    out << "}};\n\n";
    out << "} // namespace\n\n";
    out << "const WorkflowMetadata* workflows()\n";
    out << "{\n";
    out << "    return all_workflows.data();\n";
    out << "}\n\n";
    out << "std::size_t workflow_count()\n";
    out << "{\n";
    out << "    return all_workflows.size();\n";
    out << "}\n\n";
    out << "const WorkflowMetadata* find_workflow(std::string_view name)\n";
    out << "{\n";
    out << "    for (std::size_t i = 0; i < all_workflows.size(); ++i)\n";
    out << "    {\n";
    out << "        if (all_workflows[i].name == name)\n";
    out << "        {\n";
    out << "            return &all_workflows[i];\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return nullptr;\n";
    out << "}\n\n";
    out << "const WorkflowStepMetadata* find_step(\n";
    out << "    std::string_view workflow_name,\n";
    out << "    std::string_view step_name\n";
    out << ")\n";
    out << "{\n";
    out << "    const auto* workflow = find_workflow(workflow_name);\n";
    out << "    if (workflow == nullptr)\n";
    out << "    {\n";
    out << "        return nullptr;\n";
    out << "    }\n";
    out << "    for (std::size_t i = 0; i < workflow->step_count; ++i)\n";
    out << "    {\n";
    out << "        if (workflow->steps[i].name == step_name)\n";
    out << "        {\n";
    out << "            return &workflow->steps[i];\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return nullptr;\n";
    out << "}\n\n";
    out << "std::string_view start_step(std::string_view workflow_name)\n";
    out << "{\n";
    out << "    const auto* workflow = find_workflow(workflow_name);\n";
    out << "    return workflow == nullptr ? std::string_view{} : workflow->start_step;\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::wf\n";
    return out.str();
}

void append_wf_generated_files(
    const statespec::Spec& spec,
    const std::optional<std::string>& target,
    const std::optional<std::string>& out,
    statespec::GenerationResult& result
)
{
    if (!spec.system.has_value() || !should_emit_wf_files(*spec.system, target))
    {
        return;
    }

    const auto root = wf_output_root(*spec.system, target, out);
    result.files.push_back(
        statespec::GeneratedFile{join_path(root, "wf_workflows.hpp"), generate_wf_workflows_header()}
    );
    result.files.push_back(
        statespec::GeneratedFile{
            join_path(root, "wf_metadata.cpp"), generate_wf_metadata_source(*spec.system)
        }
    );
}

void write_generated_file(const statespec::GeneratedFile& file)
{
    const std::filesystem::path path{file.path};
    const auto parent = path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }

    std::ofstream output(path);
    if (!output)
    {
        throw std::runtime_error("failed to write generated file: " + file.path);
    }
    output << file.content;
}

int tokens_file(const std::string& path)
{
    statespec::DiagnosticBag diagnostics;
    const auto tokens = lex_file(path, diagnostics);

    for (const auto& token : tokens)
    {
        std::cout << std::setw(4) << token.range.begin.line << ':' << std::setw(3)
                  << token.range.begin.column << "  " << statespec::token_kind_name(token.kind);
        if (!token.lexeme.empty())
        {
            std::cout << "  " << token.lexeme;
        }
        std::cout << '\n';
    }

    if (diagnostics.has_errors())
    {
        print_diagnostics(diagnostics);
        return 1;
    }

    return 0;
}

int ast_file(const std::string& path)
{
    statespec::DiagnosticBag diagnostics;
    auto spec = parse_file(path, diagnostics);
    if (diagnostics.has_errors())
    {
        print_diagnostics(diagnostics);
        return 1;
    }

    write_spec_json(std::cout, spec);
    return 0;
}

int validate_file(const std::string& path)
{
    statespec::DiagnosticBag diagnostics;
    parse_and_validate_file(path, diagnostics);

    if (diagnostics.has_errors())
    {
        std::cout << "invalid\n";
        print_diagnostics(diagnostics);
        return 1;
    }

    std::cout << "valid\n";
    return 0;
}

int generate_file(
    const std::string& path,
    const std::optional<std::string>& target,
    const std::optional<std::string>& out
)
{
    statespec::DiagnosticBag diagnostics;
    auto spec = parse_and_validate_file(path, diagnostics);
    if (diagnostics.has_errors())
    {
        print_diagnostics(diagnostics);
        return 1;
    }

    statespec::Generator generator;
    statespec::GenerationOptions options;
    options.target_override = target;
    options.out_override = out;

    auto result = generator.generate(spec, options, diagnostics);
    append_wf_generated_files(spec, target, out, result);
    if (diagnostics.has_errors())
    {
        print_diagnostics(diagnostics);
        return 1;
    }

    for (const auto& file : result.files)
    {
        write_generated_file(file);
        std::cout << "generated " << file.path << '\n';
    }

    return 0;
}

void print_usage()
{
    std::cerr << "usage:\n";
    std::cerr << "  statespec validate <file.sspec>\n";
    std::cerr << "  statespec tokens <file.sspec>\n";
    std::cerr << "  statespec ast <file.sspec>\n";
    std::cerr << "  statespec generate <file.sspec> [target] [--out DIR]\n";
}

} // namespace

int main(
    int argc,
    char** argv
)
{
    try
    {
        if (argc < 3)
        {
            print_usage();
            return 2;
        }

        const std::string command = argv[1];
        const std::string path = argv[2];

        if (command == "validate")
        {
            return argc == 3 ? validate_file(path) : (print_usage(), 2);
        }
        if (command == "tokens")
        {
            return argc == 3 ? tokens_file(path) : (print_usage(), 2);
        }
        if (command == "ast")
        {
            return argc == 3 ? ast_file(path) : (print_usage(), 2);
        }
        if (command == "generate")
        {
            std::optional<std::string> target;
            std::optional<std::string> out;
            for (int i = 3; i < argc; ++i)
            {
                const std::string arg = argv[i];
                if (arg == "--out")
                {
                    if (i + 1 >= argc)
                    {
                        std::cerr << "statespec: --out requires a directory\n";
                        return 2;
                    }
                    out = argv[++i];
                }
                else if (!target.has_value())
                {
                    target = arg;
                }
                else
                {
                    print_usage();
                    return 2;
                }
            }
            return generate_file(path, target, out);
        }

        std::cerr << "unknown command: " << command << "\n";
        print_usage();
        return 2;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "statespec: " << ex.what() << "\n";
        return 2;
    }
}
