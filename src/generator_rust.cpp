#include "statespec/generator_rust.hpp"

#include "generator_rust_artifacts.hpp"

namespace statespec
{

GenerationResult generate_rust_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const TemplatePackage templates{resolve_binding_template_root(options)};

    add_rust_common_runtime_artifacts(result, options, templates, system, diagnostics);
    if (diagnostics.has_errors())
    {
        return result;
    }

    add_rust_api_artifacts(result, options, templates, system, diagnostics);
    add_rust_worker_artifacts(result, options, templates, system, diagnostics);
    return result;
}

} // namespace statespec
