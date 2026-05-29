#pragma once

#include "statespec/generator_bindings.hpp"

#include <optional>
#include <string>

namespace statespec
{

std::string go_string(const std::string& value);
std::string go_entity_name_constant_name(const std::string& entity_name);
std::string go_entity_plural_name_constant_name(const std::string& entity_name);
std::string go_entity_plural_api_field_name(const std::string& entity_name);
std::string go_entity_field_constant_name(
    const std::string& entity_name,
    const std::string& field_name
);
std::string go_entity_field_type_name_constant_name(
    const std::string& entity_name,
    const std::string& field_name
);
std::string go_api_codec_field_name_expr(
    const IrEntity& entity,
    const std::string& field_name
);
std::string go_entity_index_constant_name(
    const std::string& entity_name,
    const std::string& index_name
);
std::string go_entity_state_constant_name(
    const std::string& entity_name,
    const std::string& state_name
);
std::string go_shape_name_constant_name(const std::string& shape_name);
std::string go_shape_field_constant_name(
    const std::string& shape_name,
    const std::string& field_name
);
std::string go_shape_field_type_name_constant_name(
    const std::string& shape_name,
    const std::string& field_name
);
std::string go_field_descriptor_expr(const IrField& field);
std::string go_field_type_enum_expr(const std::string& type);
bool go_field_required(const std::string& type);
std::string go_entity_field_descriptor_expr(
    const std::string& entity_name,
    const IrField& field
);
std::string go_shape_field_descriptor_expr(
    const std::string& shape_name,
    const IrField& field
);
std::string go_shape_type(const std::string& type);
long long parse_go_duration_seconds(const std::optional<std::string>& value);
std::string string_ptr_expr(const std::optional<std::string>& value);
std::string duration_ptr_expr(const std::optional<std::string>& value);
const IrApi* find_go_api(
    const IrSystem& system,
    const std::string& name
);

} // namespace statespec
