#include "statespec/template_renderer.hpp"

#include "string_utils.hpp"

#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace statespec
{

namespace
{

std::string path_string(const std::filesystem::path& path)
{
    return path.generic_string();
}

} // namespace

std::string TemplateRenderer::load_template(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("failed to load template '" + path_string(path) + "'");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string TemplateRenderer::render(
    std::string_view template_text,
    const Values& values
) const
{
    std::string output;
    std::set<std::string> used_values;

    std::size_t cursor = 0;
    while (cursor < template_text.size())
    {
        const auto open = template_text.find("{{", cursor);
        if (open == std::string_view::npos)
        {
            output.append(template_text.substr(cursor));
            break;
        }

        output.append(template_text.substr(cursor, open - cursor));

        const auto close = template_text.find("}}", open + 2);
        if (close == std::string_view::npos)
        {
            throw std::invalid_argument("template placeholder opened without closing delimiter");
        }

        const auto key = trim_ascii_copy(template_text.substr(open + 2, close - open - 2));
        if (key.empty())
        {
            throw std::invalid_argument("template placeholder name must not be empty");
        }

        const auto value = values.find(key);
        if (value == values.end())
        {
            throw std::invalid_argument("template placeholder '" + key + "' has no render value");
        }

        output.append(value->second);
        used_values.insert(key);
        cursor = close + 2;
    }

    for (const auto& [key, value] : values)
    {
        (void)value;
        if (used_values.find(key) == used_values.end())
        {
            throw std::invalid_argument("render value '" + key + "' was not used by template");
        }
    }

    return output;
}

std::string TemplateRenderer::render_file(
    const std::filesystem::path& path,
    const Values& values
) const
{
    return render(load_template(path), values);
}

TemplatePackage::TemplatePackage(std::filesystem::path root)
    : root_(std::move(root))
{
}

const std::filesystem::path& TemplatePackage::root() const noexcept
{
    return root_;
}

std::filesystem::path TemplatePackage::resolve(const std::filesystem::path& relative_path) const
{
    if (relative_path.is_absolute())
    {
        throw std::invalid_argument(
            "template package paths must be relative: " + relative_path.generic_string()
        );
    }
    return root_ / relative_path;
}

std::string TemplatePackage::load(const std::filesystem::path& relative_path) const
{
    return TemplateRenderer::load_template(resolve(relative_path));
}

std::string TemplatePackage::render(
    const std::filesystem::path& relative_path,
    const TemplateRenderer::Values& values
) const
{
    return renderer_.render_file(resolve(relative_path), values);
}

} // namespace statespec
