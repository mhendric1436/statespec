#include "generator_support.hpp"

#include "generator_artifact_paths.hpp"

#include <exception>
#include <string_view>

namespace statespec
{

namespace
{

std::string strip_generated_format_control_comments(std::string content)
{
    if (content.find("// clang-format off") == std::string::npos &&
        content.find("// clang-format on") == std::string::npos)
    {
        return content;
    }

    const auto strip_token = [&content](std::string_view token)
    {
        std::string::size_type position = 0;
        while ((position = content.find(token, position)) != std::string::npos)
        {
            content.erase(position, token.size());
        }
    };

    strip_token("// clang-format off");
    strip_token("// clang-format on");

    std::string normalized;
    normalized.reserve(content.size());
    std::string::size_type line_start = 0;
    for (const auto ch : content)
    {
        if (ch == '\n')
        {
            while (normalized.size() > line_start &&
                   (normalized.back() == ' ' || normalized.back() == '\t'))
            {
                normalized.pop_back();
            }
            normalized.push_back(ch);
            line_start = normalized.size();
            continue;
        }
        normalized.push_back(ch);
    }
    while (normalized.size() > line_start &&
           (normalized.back() == ' ' || normalized.back() == '\t'))
    {
        normalized.pop_back();
    }
    return normalized;
}

} // namespace

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
        content = strip_generated_format_control_comments(templates.load(relative_template_path));
    }
    catch (const std::exception& error)
    {
        diagnostics.error(SourceRange{}, "SSPEC5201", error.what());
        return;
    }

    result.files.push_back(
        GeneratedFile{
            (output_dir / artifact_path(GeneratedArtifactCommonDir) / relative_output_path)
                .string(),
            content,
            tier,
            (artifact_path(GeneratedArtifactCommonDir) / relative_output_path).generic_string(),
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
    const auto content = strip_generated_format_control_comments(
        render_template_file(templates, relative_template_path, diagnostics, values)
    );
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
