#include "statespec/template_renderer.hpp"

#include <cctype>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace statespec
{

namespace
{

std::string trim_copy(std::string_view value)
{
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0)
    {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
    {
        --end;
    }

    return std::string(value.substr(begin, end - begin));
}

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

        const auto key = trim_copy(template_text.substr(open + 2, close - open - 2));
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

} // namespace statespec
