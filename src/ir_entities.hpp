#pragma once

#include "statespec/ir.hpp"

namespace statespec
{

void lower_ir_entities(
    const SemanticSystem& system,
    IrSystem& ir
);

} // namespace statespec
