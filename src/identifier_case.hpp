#pragma once

#include <string>
#include <string_view>

namespace statespec
{

std::string pascal_identifier(
    const std::string& value,
    std::string_view fallback = "GeneratedShape"
);

std::string lower_camel_identifier(
    const std::string& value,
    std::string_view fallback = "generatedShape"
);

std::string snake_identifier(
    const std::string& value,
    std::string_view fallback = "generated_event"
);

} // namespace statespec
