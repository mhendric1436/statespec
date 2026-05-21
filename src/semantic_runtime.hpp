#pragma once

#include "statespec/semantic.hpp"

namespace statespec
{

void resolve_semantic_runtime(
    const SystemDecl& system,
    const SymbolTable& symbols,
    SemanticSystem& resolved
);

} // namespace statespec
