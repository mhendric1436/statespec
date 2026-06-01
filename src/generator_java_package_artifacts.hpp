#pragma once

#include "statespec/generator_bindings.hpp"
#include "statespec/template_renderer.hpp"

namespace statespec
{

void add_java_package_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
);

} // namespace statespec
