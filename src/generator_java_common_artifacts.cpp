#include "generator_java_artifact_builders.hpp"

namespace statespec
{

void add_java_common_runtime_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_java_common_runtime_artifacts_impl(result, options, templates, system, diagnostics);
}

} // namespace statespec
