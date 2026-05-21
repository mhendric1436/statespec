#pragma once

#include <string>
#include <string_view>

namespace statespec
{

bool starts_with(
    std::string_view value,
    std::string_view prefix
);

bool ends_with(
    std::string_view value,
    std::string_view suffix
);

std::string lower_ascii(std::string value);

std::string trim_ascii_copy(std::string_view value);

} // namespace statespec
