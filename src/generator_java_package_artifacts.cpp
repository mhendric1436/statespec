#include "generator_java_package_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_java_artifact_makefile.hpp"
#include "generator_support.hpp"

namespace statespec
{

void add_java_package_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Makefile.tmpl"),
        artifact_path(GeneratedArtifactMakefile), diagnostics, GeneratedArtifactTier::Common,
        java_makefile_values(options.tier, system), common_artifact_path(GeneratedArtifactMakefile)
    );
}

} // namespace statespec
