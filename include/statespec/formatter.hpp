#pragma once

#include "statespec/ast.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/token.hpp"

#include <string>
#include <vector>

namespace statespec
{

std::string format_tokens(
    const std::vector<Token>& tokens,
    DiagnosticBag& diagnostics
);

std::string format_spec_ast(
    const Spec& spec,
    DiagnosticBag& diagnostics
);

} // namespace statespec
