#pragma once

#include "statespec/ast.hpp"
#include "statespec/ir_system.hpp"
#include "statespec/semantic.hpp"

namespace statespec
{

IrSystem lower_to_ir(const SemanticSystem& system);
IrSystem lower_to_ir(const Spec& spec);

} // namespace statespec
