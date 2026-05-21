#include "statespec/generator_cpp.hpp"

#include "generator_cpp_artifacts.hpp"

namespace statespec
{

GenerationResult generate_cpp_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    const TemplatePackage templates{resolve_binding_template_root(options)};

    add_cpp_common_runtime_artifacts(result, options, templates, system, diagnostics);
    if (diagnostics.has_errors())
    {
        return result;
    }

    add_cpp_api_artifacts(result, options, templates, diagnostics);
    if (diagnostics.has_errors())
    {
        return result;
    }

    add_cpp_worker_artifacts(result, options, templates, system, diagnostics);
    return result;
}

} // namespace statespec
