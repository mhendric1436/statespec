#include "statespec/generator_openapi.hpp"

#include "openapi_render.hpp"

namespace statespec
{

GenerationResult generate_openapi(
    const Spec& spec,
    const OpenApiGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    return generate_openapi(lower_to_ir(spec), options, diagnostics);
}

GenerationResult generate_openapi(
    const IrSystem& system,
    const OpenApiGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    GenerationResult result;
    if (system.name.empty())
    {
        diagnostics.error(SourceRange{}, "SSPEC5301", "expected system declaration");
        return result;
    }
    if (options.output_dir.empty())
    {
        diagnostics.error(SourceRange{}, "SSPEC5302", "openapi generation requires output_dir");
        return result;
    }

    result.files.push_back(
        GeneratedFile{
            (options.output_dir / "openapi.json").string(),
            render_openapi(system),
            GeneratedArtifactTier::Api,
            "api/openapi.json",
        }
    );
    return result;
}

} // namespace statespec
