#include "statespec/generator_java.hpp"

#include "generator_java_artifacts.hpp"

namespace statespec
{

GenerationResult generate_java_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const TemplatePackage templates{resolve_binding_template_root(options)};

    add_java_common_runtime_artifacts(result, options, templates, system, diagnostics);
    if (diagnostics.has_errors())
    {
        return result;
    }

    add_java_api_artifacts(result, options, templates, diagnostics);
    add_java_worker_artifacts(result, options, templates, system, diagnostics);
    return result;
}

} // namespace statespec
