#include "generator_rust_artifact_builders.hpp"

namespace statespec
{

void add_rust_api_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_rust_api_artifacts_impl(result, options, templates, system, diagnostics);
}

} // namespace statespec
