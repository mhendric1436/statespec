#pragma once

#include "statespec/diagnostic.hpp"
#include "statespec/generator.hpp"
#include "statespec/template_renderer.hpp"

#include <filesystem>
#include <string>

namespace statespec
{

void add_template_file(
    GenerationResult& result,
    const std::filesystem::path& output_dir,
    const TemplatePackage& templates,
    const std::filesystem::path& relative_template_path,
    const std::filesystem::path& relative_output_path,
    DiagnosticBag& diagnostics,
    GeneratedArtifactTier tier = GeneratedArtifactTier::Common
);

std::string render_template_file(
    const TemplatePackage& templates,
    const std::filesystem::path& relative_template_path,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {}
);

void add_generated_template_file(
    GenerationResult& result,
    const std::filesystem::path& output_dir,
    const TemplatePackage& templates,
    const std::filesystem::path& relative_template_path,
    const std::filesystem::path& relative_output_path,
    DiagnosticBag& diagnostics,
    GeneratedArtifactTier tier,
    const TemplateRenderer::Values& values = {},
    const std::filesystem::path& relative_artifact_path = {}
);

} // namespace statespec
