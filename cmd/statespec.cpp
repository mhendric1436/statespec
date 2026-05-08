#include "statespec/diagnostic.hpp"
#include "statespec/lexer.hpp"
#include "statespec/parser.hpp"
#include "statespec/source.hpp"
#include "statespec/validator.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

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

        std::cerr << diagnostic.range.begin.line << ':' << diagnostic.range.begin.column
                  << ": " << severity << ' ' << diagnostic.code << ": "
                  << diagnostic.message << '\n';
    }
}

int validate_file(const std::string& path)
{
    statespec::SourceFile source{path, read_file(path)};
    statespec::DiagnosticBag diagnostics;

    statespec::Lexer lexer{source};
    auto tokens = lexer.lex(diagnostics);

    statespec::Parser parser{std::move(tokens)};
    auto spec = parser.parse(diagnostics);

    statespec::Validator validator;
    validator.validate(spec, diagnostics);

    if (diagnostics.has_errors())
    {
        std::cout << "invalid\n";
        print_diagnostics(diagnostics);
        return 1;
    }

    std::cout << "valid\n";
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        if (argc != 3)
        {
            std::cerr << "usage: statespec validate <file.sspec>\n";
            return 2;
        }

        const std::string command = argv[1];
        const std::string path = argv[2];

        if (command == "validate")
        {
            return validate_file(path);
        }

        std::cerr << "unknown command: " << command << "\n";
        return 2;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "statespec: " << ex.what() << "\n";
        return 2;
    }
}
