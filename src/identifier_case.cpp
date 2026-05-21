#include "identifier_case.hpp"

#include <cctype>

namespace statespec
{

std::string pascal_identifier(
    const std::string& value,
    std::string_view fallback
)
{
    std::string result;
    bool upper_next = true;
    for (const auto ch : value)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0)
        {
            upper_next = true;
            continue;
        }
        result.push_back(
            upper_next ? static_cast<char>(std::toupper(static_cast<unsigned char>(ch))) : ch
        );
        upper_next = false;
    }
    return result.empty() ? std::string{fallback} : result;
}

std::string lower_camel_identifier(
    const std::string& value,
    std::string_view fallback
)
{
    auto identifier = pascal_identifier(value, fallback);
    if (!identifier.empty())
    {
        identifier[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(identifier[0])));
    }
    return identifier.empty() ? std::string{fallback} : identifier;
}

std::string snake_identifier(
    const std::string& value,
    std::string_view fallback
)
{
    std::string result;
    bool previous_was_separator = true;
    for (const auto ch : value)
    {
        if (std::isalnum(static_cast<unsigned char>(ch)) == 0)
        {
            if (!result.empty() && !previous_was_separator)
            {
                result.push_back('_');
            }
            previous_was_separator = true;
            continue;
        }
        if (std::isupper(static_cast<unsigned char>(ch)) != 0 && !previous_was_separator &&
            !result.empty())
        {
            result.push_back('_');
        }
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        previous_was_separator = false;
    }
    return result.empty() ? std::string{fallback} : result;
}

} // namespace statespec
