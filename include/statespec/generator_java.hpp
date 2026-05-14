#pragma once

#include "statespec/generator_bindings.hpp"

namespace statespec
{

GenerationResult generate_java_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

} // namespace statespec
