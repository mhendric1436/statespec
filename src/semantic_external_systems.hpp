#pragma once

#include "statespec/semantic.hpp"

namespace statespec
{

void resolve_semantic_external_systems(
    const SystemDecl& system,
    SemanticSystem& resolved
);

} // namespace statespec
