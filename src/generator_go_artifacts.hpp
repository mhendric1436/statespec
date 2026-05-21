#pragma once

#include "statespec/generator_bindings.hpp"
#include "statespec/template_renderer.hpp"

namespace statespec
{

void add_go_common_runtime_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
);

void add_go_api_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    DiagnosticBag& diagnostics
);

void add_go_worker_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
);

} // namespace statespec
