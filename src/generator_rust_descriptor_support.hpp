#pragma once

#include "statespec/generator_bindings.hpp"

#include <optional>
#include <string>

namespace statespec
{

std::string rust_string(const std::string& value);
std::string rust_entity_name_constant_name(const std::string& entity_name);
std::string rust_entity_field_constant_name(
    const std::string& entity_name,
    const std::string& field_name
);
std::string rust_entity_index_constant_name(
    const std::string& entity_name,
    const std::string& index_name
);
std::string rust_entity_state_constant_name(
    const std::string& entity_name,
    const std::string& state_name
);
std::string rust_field_descriptor_expr(const IrField& field);
std::string rust_entity_field_descriptor_expr(
    const std::string& entity_name,
    const IrField& field
);
std::string rust_shape_type(const std::string& type);
long long parse_rust_duration_seconds(const std::optional<std::string>& value);
std::string rust_optional_string_expr(const std::optional<std::string>& value);
std::string rust_optional_duration_expr(const std::optional<std::string>& value);
const IrApi* find_rust_api(
    const IrSystem& system,
    const std::string& name
);

} // namespace statespec
