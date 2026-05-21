#pragma once

#include "statespec/ast.hpp"
#include "statespec/semantic_system.hpp"

namespace statespec
{

SemanticSystem resolve_semantics(const Spec& spec);

} // namespace statespec
