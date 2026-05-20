#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace statespec
{

class TemplateRenderer
{
  public:
    using Values = std::map<std::string, std::string>;

    static std::string load_template(const std::filesystem::path& path);

    std::string render(
        std::string_view template_text,
        const Values& values
    ) const;

    std::string render_file(
        const std::filesystem::path& path,
        const Values& values
    ) const;
};

class TemplatePackage
{
  public:
    explicit TemplatePackage(std::filesystem::path root);

    const std::filesystem::path& root() const noexcept;

    std::filesystem::path resolve(const std::filesystem::path& relative_path) const;

    std::string load(const std::filesystem::path& relative_path) const;

    std::string render(
        const std::filesystem::path& relative_path,
        const TemplateRenderer::Values& values
    ) const;

  private:
    std::filesystem::path root_;
    TemplateRenderer renderer_;
};

} // namespace statespec
