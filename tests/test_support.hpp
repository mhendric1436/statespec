#pragma once

#include "statespec/ast.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/token.hpp"

#include <string>
#include <vector>

namespace statespec::test
{

void require(
    bool condition,
    const char* message
);

std::vector<Token> lex_text(
    const std::string& text,
    DiagnosticBag& diagnostics
);

std::vector<Token> lex_text(const std::string& text);

Spec parse_text(
    const std::string& text,
    DiagnosticBag& diagnostics
);

Spec parse_text(const std::string& text);

} // namespace statespec::test
