#pragma once

#include "validator_context.hpp"

namespace statespec::validator_detail
{

void validate_entities(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

} // namespace statespec::validator_detail
