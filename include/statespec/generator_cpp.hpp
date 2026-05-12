#pragma once

#include "statespec/generator_bindings.hpp"

namespace statespec
{

GenerationResult generate_cpp_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

} // namespace statespec
