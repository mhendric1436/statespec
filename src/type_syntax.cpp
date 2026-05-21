#include "type_syntax.hpp"

#include "string_utils.hpp"

namespace statespec
{

std::string trim_wrapped_type(
    std::string_view value,
    std::string_view prefix
)
{
    return std::string{
        value.substr(prefix.size(), value.size() - prefix.size() - std::string_view{">"}.size())
    };
}

bool is_optional_type(const std::string& type)
{
    return (starts_with(type, "optional<") && ends_with(type, ">")) || ends_with(type, "?");
}

std::string strip_optional_type(const std::string& type)
{
    if (starts_with(type, "optional<") && ends_with(type, ">"))
    {
        return trim_wrapped_type(type, "optional<");
    }
    if (ends_with(type, "?"))
    {
        return type.substr(0, type.size() - 1);
    }
    return type;
}

} // namespace statespec
