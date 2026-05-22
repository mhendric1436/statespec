#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace statespec
{

inline constexpr std::string_view GeneratedArtifactCommonDir = "common";
inline constexpr std::string_view GeneratedArtifactApiDir = "api";
inline constexpr std::string_view GeneratedArtifactWorkerDir = "worker";
inline constexpr std::string_view GeneratedArtifactBackendDir = "backend";
inline constexpr std::string_view GeneratedArtifactRuntimeDir = "runtime";
inline constexpr std::string_view GeneratedArtifactMemoryDir = "memory";
inline constexpr std::string_view GeneratedArtifactGeneratedTemplateDir = "generated";
inline constexpr std::string_view GeneratedArtifactCmdDir = "cmd";

inline constexpr std::string_view GeneratedArtifactMakefile = "Makefile";
inline constexpr std::string_view GeneratedArtifactGoMod = "go.mod";
inline constexpr std::string_view GeneratedArtifactCargoToml = "Cargo.toml";
inline constexpr std::string_view GeneratedArtifactRustLib = "lib.rs";
inline constexpr std::string_view GeneratedTemplateSuffix = ".tmpl";
inline constexpr std::string_view GeneratedJavaBackendPackagePath = "com/statespec/backend";
inline constexpr std::string_view GeneratedJavaOutputPackagePath = "com/statespec/generated";

inline std::filesystem::path artifact_path(std::string_view path)
{
    return std::filesystem::path{std::string{path}};
}

inline std::filesystem::path join_artifact_path(
    std::string_view left,
    std::string_view right
)
{
    return artifact_path(left) / artifact_path(right);
}

inline std::filesystem::path join_artifact_path(
    std::string_view first,
    std::string_view second,
    std::string_view third
)
{
    return join_artifact_path(first, second) / artifact_path(third);
}

inline std::filesystem::path join_artifact_path(
    std::string_view first,
    std::string_view second,
    std::string_view third,
    std::string_view fourth
)
{
    return join_artifact_path(first, second, third) / artifact_path(fourth);
}

inline std::filesystem::path generated_template_path(std::string_view filename)
{
    return join_artifact_path(GeneratedArtifactGeneratedTemplateDir, filename);
}

inline std::filesystem::path template_path_for_output(std::string_view output_path)
{
    return artifact_path(std::string{output_path} + std::string{GeneratedTemplateSuffix});
}

inline std::filesystem::path template_path_for_output(const std::filesystem::path& output_path)
{
    return artifact_path(output_path.generic_string() + std::string{GeneratedTemplateSuffix});
}

inline std::filesystem::path common_artifact_path(std::string_view path)
{
    return join_artifact_path(GeneratedArtifactCommonDir, path);
}

inline std::filesystem::path api_artifact_path(std::string_view path)
{
    return join_artifact_path(GeneratedArtifactApiDir, path);
}

inline std::filesystem::path worker_artifact_path(std::string_view path)
{
    return join_artifact_path(GeneratedArtifactWorkerDir, path);
}

inline std::string include_line(std::string_view path)
{
    return std::string{"#include \""} + std::string{path} + "\"\\n";
}

} // namespace statespec
