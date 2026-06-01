#include "generator_go_package_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_go_artifact_makefile.hpp"
#include "generator_support.hpp"

namespace statespec
{

void add_go_package_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("go.mod.tmpl"),
        artifact_path(GeneratedArtifactGoMod), diagnostics, GeneratedArtifactTier::Common, {},
        common_artifact_path(GeneratedArtifactGoMod)
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Makefile.tmpl"),
        artifact_path(GeneratedArtifactMakefile), diagnostics, GeneratedArtifactTier::Common,
        go_makefile_values(options.tier, system), common_artifact_path(GeneratedArtifactMakefile)
    );
}

} // namespace statespec
