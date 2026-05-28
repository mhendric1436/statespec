#pragma once

#include "statespec/diagnostic.hpp"
#include "statespec/ir.hpp"

namespace statespec
{

bool validate_crud_handler_inputs(
    const IrSystem& system,
    DiagnosticBag& diagnostics
);

} // namespace statespec
