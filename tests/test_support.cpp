#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/lexer.hpp"
#include "statespec/parser.hpp"
#include "statespec/source.hpp"

#include <utility>

namespace statespec::test
{

void require(
    bool condition,
    const char* message
)
{
    INFO(message);
    REQUIRE(condition);
}

std::vector<Token> lex_text(
    const std::string& text,
    DiagnosticBag& diagnostics
)
{
    SourceFile source{"test.sspec", text};
    Lexer lexer{source};
    return lexer.lex(diagnostics);
}

std::vector<Token> lex_text(const std::string& text)
{
    DiagnosticBag diagnostics;
    auto tokens = lex_text(text, diagnostics);
    require(!diagnostics.has_errors(), "lexer should not produce diagnostics for valid input");
    return tokens;
}

Spec parse_text(
    const std::string& text,
    DiagnosticBag& diagnostics
)
{
    auto tokens = lex_text(text, diagnostics);
    Parser parser{std::move(tokens)};
    return parser.parse(diagnostics);
}

Spec parse_text(const std::string& text)
{
    DiagnosticBag diagnostics;
    auto spec = parse_text(text, diagnostics);
    require(!diagnostics.has_errors(), "parser should not produce errors for valid input");
    return spec;
}

} // namespace statespec::test
