#include "statespec/binding_language.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/generator.hpp"
#include "statespec/generator_bindings.hpp"
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

struct GenerateBindingsArgs
{
    statespec::BindingLanguage language;
    std::string input_path;
    std::string output_dir;
};

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

template <typename T>
void write_named_array(
    std::ostream& out,
    const std::vector<T>& values
)
{
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        out << "{\"name\": ";
        write_json_string(out, values[i].name);
        out << '}';
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

void write_feature_flags(
    std::ostream& out,
    const std::vector<statespec::FeatureFlagDecl>& feature_flags
)
{
    out << '[';
    for (std::size_t i = 0; i < feature_flags.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        const auto& feature_flag = feature_flags[i];
        out << "{\"name\": ";
        write_json_string(out, feature_flag.name);
        out << ", \"type\": ";
        write_json_optional_string(out, feature_flag.type);
        out << ", \"default\": ";
        write_json_optional_string(out, feature_flag.default_value);
        out << ", \"default_kind\": ";
        write_json_optional_string(out, feature_flag.default_value_kind);
        out << ", \"scope\": ";
        write_json_optional_string(out, feature_flag.scope);
        out << ", \"owner\": ";
        write_json_optional_string(out, feature_flag.owner);
        out << ", \"description\": ";
        write_json_optional_string(out, feature_flag.description);
        out << ", \"expires\": ";
        write_json_optional_string(out, feature_flag.expires);
        out << '}';
    }
    out << ']';
}

void write_string_array(
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

    out << "    \"feature_flags\": ";
    write_feature_flags(out, system.feature_flags);
    out << ",\n";

    out << "    \"entities\": [";
    for (std::size_t i = 0; i < system.entities.size(); ++i)
    {
        if (i > 0)
        {
            out << ", ";
        }
        const auto& entity = system.entities[i];
        out << "{\"name\": ";
        write_json_string(out, entity.name);
        out << ", \"key_fields\": ";
        write_string_array(out, entity.key_fields);
        out << ", \"fields\": ";
        write_fields(out, entity.fields);
        out << '}';
    }
    out << "],\n";

    out << "    \"queues\": ";
    write_named_array(out, system.queues);
    out << ",\n";
    out << "    \"leases\": ";
    write_named_array(out, system.leases);
    out << ",\n";
    out << "    \"workers\": ";
    write_named_array(out, system.workers);
    out << ",\n";
    out << "    \"apis\": ";
    write_named_array(out, system.apis);
    out << ",\n";
    out << "    \"workflows\": ";
    write_named_array(out, system.workflows);
    out << ",\n";
    out << "    \"policies\": ";
    write_named_array(out, system.policies);
    out << "\n";
    out << "  }\n";
    out << "}\n";
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
    const auto spec = parse_file(path, diagnostics);
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

GenerateBindingsArgs parse_generate_bindings_args(
    int argc,
    char** argv
)
{
    std::optional<statespec::BindingLanguage> language;
    std::optional<std::string> input_path;
    std::optional<std::string> output_dir;

    for (int i = 3; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--lang")
        {
            if (i + 1 >= argc)
            {
                throw std::runtime_error(
                    "--lang requires one of: " + statespec::supported_binding_languages_text()
                );
            }
            language = statespec::parse_binding_language(argv[++i]);
        }
        else if (arg == "--out")
        {
            if (i + 1 >= argc)
            {
                throw std::runtime_error("--out requires a directory");
            }
            output_dir = argv[++i];
        }
        else if (!input_path.has_value())
        {
            input_path = arg;
        }
        else
        {
            throw std::runtime_error("unexpected argument for generate bindings: " + arg);
        }
    }

    if (!language.has_value())
    {
        throw std::runtime_error(
            "generate bindings requires --lang <" + statespec::supported_binding_languages_text() +
            ">"
        );
    }
    if (!input_path.has_value())
    {
        throw std::runtime_error("generate bindings requires an input .sspec file");
    }
    if (!output_dir.has_value())
    {
        output_dir = std::filesystem::path{"generated"} / statespec::to_string(*language);
    }

    return GenerateBindingsArgs{*language, *input_path, *output_dir};
}

int generate_bindings_file(const GenerateBindingsArgs& args)
{
    statespec::DiagnosticBag diagnostics;
    const auto spec = parse_and_validate_file(args.input_path, diagnostics);
    if (diagnostics.has_errors())
    {
        print_diagnostics(diagnostics);
        return 1;
    }

    const statespec::BindingGeneratorOptions options{
        args.language,
        std::filesystem::path{args.output_dir},
    };

    const auto result = statespec::generate_bindings(spec, options, diagnostics);
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

void print_usage(std::ostream& out)
{
    out << "usage:\n";
    out << "  statespec help\n";
    out << "  statespec validate <file.sspec>\n";
    out << "  statespec tokens <file.sspec>\n";
    out << "  statespec ast <file.sspec>\n";
    out << "  statespec generate bindings --lang <cpp|go|java|rust> <file.sspec> [--out DIR]\n";
}

void print_usage()
{
    print_usage(std::cerr);
}

} // namespace

int main(
    int argc,
    char** argv
)
{
    try
    {
        if (argc == 2)
        {
            const std::string command = argv[1];
            if (command == "help" || command == "--help" || command == "-h")
            {
                print_usage(std::cout);
                return 0;
            }
        }

        if (argc < 3)
        {
            print_usage();
            return 2;
        }

        const std::string command = argv[1];

        if (command == "validate")
        {
            const std::string path = argv[2];
            return argc == 3 ? validate_file(path) : (print_usage(), 2);
        }
        if (command == "tokens")
        {
            const std::string path = argv[2];
            return argc == 3 ? tokens_file(path) : (print_usage(), 2);
        }
        if (command == "ast")
        {
            const std::string path = argv[2];
            return argc == 3 ? ast_file(path) : (print_usage(), 2);
        }
        if (command == "generate")
        {
            const std::string generate_kind = argv[2];
            if (generate_kind == "bindings")
            {
                const auto args = parse_generate_bindings_args(argc, argv);
                return generate_bindings_file(args);
            }

            std::cerr << "statespec: unsupported generate kind: " << generate_kind << "\n";
            print_usage();
            return 2;
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
