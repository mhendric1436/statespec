#pragma once

#include "statespec/semantic.hpp"

namespace statespec
{

void resolve_semantic_declarations(
    const SystemDecl& system,
    SemanticSystem& resolved
);

} // namespace statespec
