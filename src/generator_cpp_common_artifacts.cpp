#include "generator_cpp_artifact_builders.hpp"

namespace statespec
{

void add_cpp_common_runtime_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_cpp_common_runtime_artifacts_impl(result, options, templates, system, diagnostics);
}

} // namespace statespec
