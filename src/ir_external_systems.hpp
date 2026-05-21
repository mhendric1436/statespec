#pragma once

#include "statespec/ir.hpp"

namespace statespec
{

void lower_ir_external_systems(
    const SemanticSystem& system,
    IrSystem& ir
);

} // namespace statespec
