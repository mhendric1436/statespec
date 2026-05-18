#pragma once

#include "validator_context.hpp"

namespace statespec::validator_detail
{

void validate_queues(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

void validate_leases(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
);

void validate_workers(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

void validate_apis(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

void validate_api_servers(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
);

} // namespace statespec::validator_detail
