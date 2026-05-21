#pragma once

#include <string>
#include <string_view>

namespace statespec
{

std::string trim_wrapped_type(
    std::string_view value,
    std::string_view prefix
);

bool is_optional_type(const std::string& type);

std::string strip_optional_type(const std::string& type);

} // namespace statespec
