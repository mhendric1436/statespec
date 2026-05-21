#include "string_utils.hpp"

#include <algorithm>
#include <cctype>

namespace statespec
{

bool starts_with(
    std::string_view value,
    std::string_view prefix
)
{
    return value.rfind(prefix, 0) == 0;
}

bool ends_with(
    std::string_view value,
    std::string_view suffix
)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string lower_ascii(std::string value)
{
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); }
    );
    return value;
}

std::string trim_ascii_copy(std::string_view value)
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

    return std::string{value.substr(begin, end - begin)};
}

} // namespace statespec
