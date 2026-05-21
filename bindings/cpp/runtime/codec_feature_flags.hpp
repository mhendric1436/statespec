#pragma once

#include "codec_core.hpp"

#include "../feature_flag.hpp"

namespace statespec::backend::runtime::detail
{

inline Json feature_flag_value_to_json(const FeatureFlagValue& value)
{
    switch (value.type())
    {
    case FeatureFlagType::Bool:
        return Json::object({{"type", "bool"}, {"value", *value.as_bool()}});
    case FeatureFlagType::String:
        return Json::object({{"type", "string"}, {"value", *value.as_string()}});
    case FeatureFlagType::Integer:
        return Json::object({{"type", "integer"}, {"value", *value.as_integer()}});
    case FeatureFlagType::Decimal:
        return Json::object({{"type", "decimal"}, {"value", *value.as_decimal()}});
    }
    throw BackendError("unknown feature flag value type");
}

inline FeatureFlagValue feature_flag_value_from_json(const Json& json)
{
    const auto type = json.at("type").as_string();
    if (type == "bool")
    {
        return FeatureFlagValue::bool_value(json.at("value").as_bool());
    }
    if (type == "string")
    {
        return FeatureFlagValue::string_value(json.at("value").as_string());
    }
    if (type == "integer")
    {
        return FeatureFlagValue::integer_value(json.at("value").as_int64());
    }
    if (type == "decimal")
    {
        return FeatureFlagValue::decimal_value(json.at("value").as_double());
    }
    throw BackendError("unknown feature flag value type: " + type);
}

inline std::string feature_flag_type_name(FeatureFlagType type)
{
    switch (type)
    {
    case FeatureFlagType::Bool:
        return "bool";
    case FeatureFlagType::String:
        return "string";
    case FeatureFlagType::Integer:
        return "integer";
    case FeatureFlagType::Decimal:
        return "decimal";
    }
    throw BackendError("unknown feature flag type");
}

inline FeatureFlagType feature_flag_type_from_string(const std::string& value)
{
    if (value == "bool")
    {
        return FeatureFlagType::Bool;
    }
    if (value == "string")
    {
        return FeatureFlagType::String;
    }
    if (value == "integer")
    {
        return FeatureFlagType::Integer;
    }
    if (value == "decimal")
    {
        return FeatureFlagType::Decimal;
    }
    throw BackendError("unknown feature flag type: " + value);
}

inline std::string feature_flag_scope_name(FeatureFlagScopeKind scope)
{
    switch (scope)
    {
    case FeatureFlagScopeKind::System:
        return "system";
    case FeatureFlagScopeKind::Tenant:
        return "tenant";
    case FeatureFlagScopeKind::User:
        return "user";
    case FeatureFlagScopeKind::Entity:
        return "entity";
    }
    throw BackendError("unknown feature flag scope");
}

inline FeatureFlagScopeKind feature_flag_scope_from_string(const std::string& value)
{
    if (value == "system")
    {
        return FeatureFlagScopeKind::System;
    }
    if (value == "tenant")
    {
        return FeatureFlagScopeKind::Tenant;
    }
    if (value == "user")
    {
        return FeatureFlagScopeKind::User;
    }
    if (value == "entity")
    {
        return FeatureFlagScopeKind::Entity;
    }
    throw BackendError("unknown feature flag scope: " + value);
}

inline Json feature_flag_definition_to_json(const FeatureFlagDefinition& definition)
{
    Json::Object object{
        {"name", definition.name},
        {"type", feature_flag_type_name(definition.type)},
        {"default_value", feature_flag_value_to_json(definition.default_value)},
        {"scope", feature_flag_scope_name(definition.scope)},
    };
    put_optional_string(object, "owner", definition.owner);
    put_optional_string(object, "description", definition.description);
    put_optional_string(object, "expires", definition.expires);
    return Json::object(std::move(object));
}

inline FeatureFlagDefinition feature_flag_definition_from_json(const Json& json)
{
    return FeatureFlagDefinition{
        .name = json.at("name").as_string(),
        .type = feature_flag_type_from_string(json.at("type").as_string()),
        .default_value = feature_flag_value_from_json(json.at("default_value")),
        .scope = feature_flag_scope_from_string(json.at("scope").as_string()),
        .owner = optional_string(json, "owner"),
        .description = optional_string(json, "description"),
        .expires = optional_string(json, "expires"),
    };
}

} // namespace statespec::backend::runtime::detail
