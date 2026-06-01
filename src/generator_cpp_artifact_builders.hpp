#pragma once

#include "generator_cpp_artifacts.hpp"

namespace statespec
{

void add_cpp_common_runtime_artifacts_impl(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
);

void add_cpp_api_artifacts_impl(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
);

void add_cpp_worker_artifacts_impl(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
);

} // namespace statespec
