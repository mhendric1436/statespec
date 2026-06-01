#include "generator_rust_package_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_rust_artifact_manifest.hpp"
#include "generator_support.hpp"

namespace statespec
{

void add_rust_package_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Cargo.toml.tmpl"),
        artifact_path(GeneratedArtifactCargoToml), diagnostics, GeneratedArtifactTier::Common, {},
        common_artifact_path(GeneratedArtifactCargoToml)
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("lib.rs.tmpl"),
        artifact_path(GeneratedArtifactRustLib), diagnostics, GeneratedArtifactTier::Common,
        rust_lib_values(options.tier, system), common_artifact_path(GeneratedArtifactRustLib)
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Makefile.tmpl"),
        artifact_path(GeneratedArtifactMakefile), diagnostics, GeneratedArtifactTier::Common,
        rust_makefile_values(options.tier, system), common_artifact_path(GeneratedArtifactMakefile)
    );
}

} // namespace statespec
