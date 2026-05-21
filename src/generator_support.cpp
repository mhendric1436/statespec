#include "generator_support.hpp"

#include <exception>

namespace statespec
{

void add_template_file(
    GenerationResult& result,
    const std::filesystem::path& output_dir,
    const TemplatePackage& templates,
    const std::filesystem::path& relative_template_path,
    const std::filesystem::path& relative_output_path,
    DiagnosticBag& diagnostics,
    GeneratedArtifactTier tier
)
{
    std::string content;
    try
    {
        content = templates.load(relative_template_path);
    }
    catch (const std::exception& error)
    {
        diagnostics.error(SourceRange{}, "SSPEC5201", error.what());
        return;
    }

    result.files.push_back(
        GeneratedFile{
            (output_dir / "common" / relative_output_path).string(),
            content,
            tier,
            (std::filesystem::path{"common"} / relative_output_path).generic_string(),
        }
    );
}

std::string render_template_file(
    const TemplatePackage& templates,
    const std::filesystem::path& relative_template_path,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values
)
{
    try
    {
        return templates.render(relative_template_path, values);
    }
    catch (const std::exception& error)
    {
        diagnostics.error(SourceRange{}, "SSPEC5201", error.what());
        return {};
    }
}

void add_generated_template_file(
    GenerationResult& result,
    const std::filesystem::path& output_dir,
    const TemplatePackage& templates,
    const std::filesystem::path& relative_template_path,
    const std::filesystem::path& relative_output_path,
    DiagnosticBag& diagnostics,
    GeneratedArtifactTier tier,
    const TemplateRenderer::Values& values,
    const std::filesystem::path& relative_artifact_path
)
{
    const auto content =
        render_template_file(templates, relative_template_path, diagnostics, values);
    if (diagnostics.has_errors())
    {
        return;
    }

    result.files.push_back(
        GeneratedFile{
            (output_dir / relative_output_path).string(),
            content,
            tier,
            (relative_artifact_path.empty() ? relative_output_path : relative_artifact_path)
                .generic_string(),
        }
    );
}

} // namespace statespec
