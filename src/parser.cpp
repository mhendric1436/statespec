#include "statespec/parser.hpp"

#include <utility>

namespace statespec
{

Parser::Parser(std::vector<Token> tokens)
    : ParserContext(std::move(tokens))
{
}

Spec Parser::parse(DiagnosticBag& diagnostics)
{
    return parse_spec(diagnostics);
}

} // namespace statespec
