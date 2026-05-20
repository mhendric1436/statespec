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

} // namespace statespec
