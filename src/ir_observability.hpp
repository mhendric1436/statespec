#pragma once

#include "statespec/ir.hpp"

namespace statespec
{

void lower_ir_observability(
    const SemanticSystem& system,
    IrSystem& ir
);

} // namespace statespec
