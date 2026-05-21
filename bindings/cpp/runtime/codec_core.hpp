#pragma once

#include "../backend.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace statespec::backend::runtime::detail
{

inline std::int64_t seconds_count(std::chrono::seconds value)
{
    return value.count();
}

inline std::chrono::seconds seconds_from(const Json& value)
{
    return std::chrono::seconds{value.as_int64()};
}

inline std::int64_t timestamp_millis(std::chrono::system_clock::time_point value)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(value.time_since_epoch()).count();
}

inline std::chrono::system_clock::time_point timestamp_from(const Json& value)
{
    return std::chrono::system_clock::time_point{std::chrono::milliseconds{value.as_int64()}};
}

inline void put_optional_string(
    Json::Object& object,
    const std::string& key,
    const std::optional<std::string>& value
)
{
    if (value.has_value())
    {
        object.emplace(key, *value);
    }
}

inline std::optional<std::string> optional_string(
    const Json& object,
    const std::string& key
)
{
    if (const auto* value = object.find(key))
    {
        return value->as_string();
    }
    return std::nullopt;
}

} // namespace statespec::backend::runtime::detail
