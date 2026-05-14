#pragma once

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

} // namespace statespec
