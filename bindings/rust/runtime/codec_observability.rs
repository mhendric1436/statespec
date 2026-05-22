use crate::backend::{FieldDescriptor, FieldType};
use crate::json::Json;

use super::codec_core::{bool_from_json, object, object_values, string_from_json};

fn field_type_to_string(field_type: FieldType) -> String {
    format!("{field_type:?}")
}

fn field_type_from_json(value: &Json) -> FieldType {
    match string_from_json(value).as_str() {
        "Bool" => FieldType::Bool,
        "Int" => FieldType::Int,
        "Int32" => FieldType::Int32,
        "Int64" => FieldType::Int64,
        "Long" => FieldType::Long,
        "Double" => FieldType::Double,
        "Decimal" => FieldType::Decimal,
        "Json" => FieldType::Json,
        "Timestamp" => FieldType::Timestamp,
        "Duration" => FieldType::Duration,
        "Uuid" => FieldType::Uuid,
        "Named" => FieldType::Named,
        "List" => FieldType::List,
        "Set" => FieldType::Set,
        "Map" => FieldType::Map,
        "Optional" => FieldType::Optional,
        "Reference" => FieldType::Reference,
        _ => FieldType::String,
    }
}

fn field_descriptor_to_json(field: &FieldDescriptor) -> Json {
    object(vec![
        ("name", Json::String(field.name.clone())),
        ("type", Json::String(field_type_to_string(field.field_type))),
        ("type_name", Json::String(field.type_name.clone())),
        ("required", Json::Bool(field.required)),
    ])
}

fn field_descriptor_from_json(value: &Json) -> FieldDescriptor {
    let values = object_values(value);
    FieldDescriptor {
        name: string_from_json(values.get("name").unwrap_or(&Json::Null)),
        field_type: field_type_from_json(values.get("type").unwrap_or(&Json::Null)),
        type_name: string_from_json(values.get("type_name").unwrap_or(&Json::Null)),
        required: bool_from_json(values.get("required").unwrap_or(&Json::Null)),
    }
}

pub(crate) fn fields_to_json(fields: &[FieldDescriptor]) -> Json {
    Json::Array(fields.iter().map(field_descriptor_to_json).collect())
}

pub(crate) fn fields_from_json(value: &Json) -> Vec<FieldDescriptor> {
    match value {
        Json::Array(values) => values.iter().map(field_descriptor_from_json).collect(),
        _ => Vec::new(),
    }
}
