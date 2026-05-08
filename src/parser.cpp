#include "statespec/parser.hpp"

#include <utility>

namespace statespec {

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens))
{
}

Spec Parser::parse(DiagnosticBag& diagnostics)
{
    Spec spec;
    diagnostics.error(SourceRange{}, "SSPEC0001", "parser implementation is not complete yet");
    return spec;
}

} // namespace statespec
