#pragma once

#include "statespec/generator_bindings.hpp"

#include <optional>
#include <string>

namespace statespec
{

std::string rust_string(const std::string& value);
std::string rust_field_descriptor_expr(const IrField& field);
std::string rust_shape_type(const std::string& type);
long long parse_rust_duration_seconds(const std::optional<std::string>& value);
std::string rust_optional_string_expr(const std::optional<std::string>& value);
std::string rust_optional_duration_expr(const std::optional<std::string>& value);
const IrApi* find_rust_api(
    const IrSystem& system,
    const std::string& name
);

} // namespace statespec
