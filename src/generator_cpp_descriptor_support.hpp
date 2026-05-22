#pragma once

#include "statespec/generator_bindings.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace statespec
{

std::string cpp_string(const std::string& value);
std::string cpp_entity_state_constant_name(
    const std::string& entity_name,
    const std::string& state_name
);
std::string cpp_field_descriptor_expr(const IrField& field);
std::string cpp_shape_type(const std::string& type);
std::int64_t parse_duration_seconds(const std::optional<std::string>& value);
std::string optional_string_expr(const std::optional<std::string>& value);
const IrApi* find_api(
    const IrSystem& system,
    const std::string& name
);

} // namespace statespec
