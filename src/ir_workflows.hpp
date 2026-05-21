#pragma once

#include "statespec/ir.hpp"

namespace statespec
{

void lower_ir_workflows(
    const SemanticSystem& system,
    IrSystem& ir
);

} // namespace statespec
