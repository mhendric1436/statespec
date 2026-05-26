#pragma once

#include "statespec/generator_bindings.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace statespec
{

std::string cpp_string(const std::string& value);
std::string cpp_entity_name_constant_name(const std::string& entity_name);
std::string cpp_entity_field_constant_name(
    const std::string& entity_name,
    const std::string& field_name
);
std::string cpp_entity_field_type_name_constant_name(
    const std::string& entity_name,
    const std::string& field_name
);
std::string cpp_entity_index_constant_name(
    const std::string& entity_name,
    const std::string& index_name
);
std::string cpp_entity_state_constant_name(
    const std::string& entity_name,
    const std::string& state_name
);
std::string cpp_shape_name_constant_name(const std::string& shape_name);
std::string cpp_shape_field_constant_name(
    const std::string& shape_name,
    const std::string& field_name
);
std::string cpp_shape_field_type_name_constant_name(
    const std::string& shape_name,
    const std::string& field_name
);
std::string cpp_field_descriptor_expr(const IrField& field);
std::string cpp_field_type_enum_expr(const std::string& type);
bool cpp_field_required(const std::string& type);
std::string cpp_entity_field_descriptor_expr(
    const std::string& entity_name,
    const IrField& field
);
std::string cpp_shape_field_descriptor_expr(
    const std::string& shape_name,
    const IrField& field
);
std::string cpp_shape_type(const std::string& type);
std::int64_t parse_duration_seconds(const std::optional<std::string>& value);
std::string optional_string_expr(const std::optional<std::string>& value);
const IrApi* find_api(
    const IrSystem& system,
    const std::string& name
);

} // namespace statespec
