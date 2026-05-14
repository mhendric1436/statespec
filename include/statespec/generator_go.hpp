#pragma once

#include "statespec/generator_bindings.hpp"

namespace statespec
{

GenerationResult generate_go_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

} // namespace statespec
