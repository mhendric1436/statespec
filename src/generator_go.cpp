#include "statespec/generator_go.hpp"

#include "generator_go_artifacts.hpp"

namespace statespec
{

GenerationResult generate_go_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const TemplatePackage templates{resolve_binding_template_root(options)};

    add_go_common_runtime_artifacts(result, options, templates, system, diagnostics);
    if (diagnostics.has_errors())
    {
        return result;
    }

    add_go_api_artifacts(result, options, templates, diagnostics);
    add_go_worker_artifacts(result, options, templates, system, diagnostics);
    return result;
}

} // namespace statespec
