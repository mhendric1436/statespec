#include "statespec/diagnostic.hpp"
#include "statespec/lexer.hpp"
#include "statespec/parser.hpp"
#include "statespec/source.hpp"
#include "statespec/token.hpp"
#include "statespec/validator.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

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

std::vector<statespec::Token> lex_file(const std::string& path, statespec::DiagnosticBag& diagnostics)
{
    statespec::SourceFile source{path, read_file(path)};
    statespec::Lexer lexer{source};
    return lexer.lex(diagnostics);
}

int tokens_file(const std::string& path)
{
    statespec::DiagnosticBag diagnostics;
    const auto tokens = lex_file(path, diagnostics);

    for (const auto& token : tokens)
    {
        std::cout << std::setw(4) << token.range.begin.line << ':'
                  << std::setw(3) << token.range.begin.column << "  "
                  << statespec::token_kind_name(token.kind);
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

int validate_file(const std::string& path)
{
    statespec::DiagnosticBag diagnostics;
    auto tokens = lex_file(path, diagnostics);

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
            std::cerr << "usage: statespec <validate|tokens> <file.sspec>\n";
            return 2;
        }

        const std::string command = argv[1];
        const std::string path = argv[2];

        if (command == "validate")
        {
            return validate_file(path);
        }
        if (command == "tokens")
        {
            return tokens_file(path);
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
