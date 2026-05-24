#pragma once

#include "statespec/generator_bindings.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

std::string java_string(const std::string& value);
std::string java_entity_name_constant_name(const std::string& entity_name);
std::string java_entity_field_constant_name(
    const std::string& entity_name,
    const std::string& field_name
);
std::string java_entity_field_type_name_constant_name(
    const std::string& entity_name,
    const std::string& field_name
);
std::string java_entity_index_constant_name(
    const std::string& entity_name,
    const std::string& index_name
);
std::string java_entity_state_constant_name(
    const std::string& entity_name,
    const std::string& state_name
);
std::string java_field_descriptor_expr(const IrField& field);
std::string java_entity_field_descriptor_expr(
    const std::string& entity_name,
    const IrField& field
);
std::string java_shape_type(const std::string& type);
long long parse_java_duration_seconds(const std::optional<std::string>& value);
std::string java_optional_string_expr(const std::optional<std::string>& value);
std::string optional_duration_expr(const std::optional<std::string>& value);
const IrApi* find_java_api(
    const IrSystem& system,
    const std::string& name
);
std::string java_list_expr(const std::vector<std::string>& values);

} // namespace statespec
