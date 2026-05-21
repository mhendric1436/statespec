use crate::feature_flag::{
    FeatureFlagDefinition, FeatureFlagScopeKind, FeatureFlagType, FeatureFlagValue,
};
use crate::json::Json;

use super::codec_core::{
    bool_from_json, f64_from_json, i64_from_json, object, object_values, optional_string,
    optional_string_from_json, string_from_json,
};

pub(crate) fn feature_flag_value_to_json(value: &FeatureFlagValue) -> Json {
    let (flag_type, encoded) = match value {
        FeatureFlagValue::Bool(value) => ("Bool", Json::Bool(*value)),
        FeatureFlagValue::String(value) => ("String", Json::String(value.clone())),
        FeatureFlagValue::Integer(value) => ("Integer", Json::Integer(*value)),
        FeatureFlagValue::Decimal(value) => ("Decimal", Json::Decimal(*value)),
    };
    object(vec![
        ("type", Json::String(flag_type.to_string())),
        ("value", encoded),
    ])
}

pub(crate) fn feature_flag_value_from_json(value: &Json) -> FeatureFlagValue {
    let values = object_values(value);
    match string_from_json(values.get("type").unwrap_or(&Json::Null)).as_str() {
        "Bool" => {
            FeatureFlagValue::Bool(bool_from_json(values.get("value").unwrap_or(&Json::Null)))
        }
        "String" => {
            FeatureFlagValue::String(string_from_json(values.get("value").unwrap_or(&Json::Null)))
        }
        "Integer" => {
            FeatureFlagValue::Integer(i64_from_json(values.get("value").unwrap_or(&Json::Null)))
        }
        "Decimal" => {
            FeatureFlagValue::Decimal(f64_from_json(values.get("value").unwrap_or(&Json::Null)))
        }
        _ => FeatureFlagValue::Bool(false),
    }
}

pub(crate) fn feature_flag_definition_to_json(definition: &FeatureFlagDefinition) -> Json {
    object(vec![
        ("name", Json::String(definition.name.clone())),
        (
            "type",
            Json::String(
                match definition.flag_type {
                    FeatureFlagType::Bool => "Bool",
                    FeatureFlagType::String => "String",
                    FeatureFlagType::Integer => "Integer",
                    FeatureFlagType::Decimal => "Decimal",
                }
                .to_string(),
            ),
        ),
        (
            "default_value",
            feature_flag_value_to_json(&definition.default_value),
        ),
        (
            "scope",
            Json::String(
                match definition.scope {
                    FeatureFlagScopeKind::System => "System",
                    FeatureFlagScopeKind::Tenant => "Tenant",
                    FeatureFlagScopeKind::User => "User",
                    FeatureFlagScopeKind::Entity => "Entity",
                }
                .to_string(),
            ),
        ),
        ("owner", optional_string(&definition.owner)),
        ("description", optional_string(&definition.description)),
        ("expires", optional_string(&definition.expires)),
    ])
}

pub(crate) fn feature_flag_definition_from_json(value: &Json) -> FeatureFlagDefinition {
    let values = object_values(value);
    let flag_type = match string_from_json(values.get("type").unwrap_or(&Json::Null)).as_str() {
        "String" => FeatureFlagType::String,
        "Integer" => FeatureFlagType::Integer,
        "Decimal" => FeatureFlagType::Decimal,
        _ => FeatureFlagType::Bool,
    };
    let scope = match string_from_json(values.get("scope").unwrap_or(&Json::Null)).as_str() {
        "Tenant" => FeatureFlagScopeKind::Tenant,
        "User" => FeatureFlagScopeKind::User,
        "Entity" => FeatureFlagScopeKind::Entity,
        _ => FeatureFlagScopeKind::System,
    };
    FeatureFlagDefinition {
        name: string_from_json(values.get("name").unwrap_or(&Json::Null)),
        flag_type,
        default_value: feature_flag_value_from_json(
            values.get("default_value").unwrap_or(&Json::Null),
        ),
        scope,
        owner: optional_string_from_json(values.get("owner").unwrap_or(&Json::Null)),
        description: optional_string_from_json(values.get("description").unwrap_or(&Json::Null)),
        expires: optional_string_from_json(values.get("expires").unwrap_or(&Json::Null)),
    }
}
