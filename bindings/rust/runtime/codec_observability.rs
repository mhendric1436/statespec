use std::collections::BTreeMap;

use crate::backend::{FieldDescriptor, FieldType};
use crate::json::Json;
use crate::log::{LogDefinition, LogEvent, LogLevel};
use crate::metric::{MetricDefinition, MetricKind, MetricSample};

use super::codec_core::{bool_from_json, f64_from_json, object, object_values, string_from_json};

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

fn fields_to_json(fields: &[FieldDescriptor]) -> Json {
    Json::Array(fields.iter().map(field_descriptor_to_json).collect())
}

fn fields_from_json(value: &Json) -> Vec<FieldDescriptor> {
    match value {
        Json::Array(values) => values.iter().map(field_descriptor_from_json).collect(),
        _ => Vec::new(),
    }
}

pub(crate) fn log_definition_to_json(definition: &LogDefinition) -> Json {
    object(vec![
        ("name", Json::String(definition.name.clone())),
        ("level", Json::String(format!("{:?}", definition.level))),
        ("event_name", Json::String(definition.event_name.clone())),
        ("fields", fields_to_json(&definition.fields)),
    ])
}

pub(crate) fn log_definition_from_json(value: &Json) -> LogDefinition {
    let values = object_values(value);
    LogDefinition {
        name: string_from_json(values.get("name").unwrap_or(&Json::Null)),
        level: match string_from_json(values.get("level").unwrap_or(&Json::Null)).as_str() {
            "Debug" => LogLevel::Debug,
            "Warn" => LogLevel::Warn,
            "Error" => LogLevel::Error,
            _ => LogLevel::Info,
        },
        event_name: string_from_json(values.get("event_name").unwrap_or(&Json::Null)),
        fields: fields_from_json(values.get("fields").unwrap_or(&Json::Null)),
    }
}

pub(crate) fn log_event_to_json(event: &LogEvent) -> Json {
    object(vec![
        ("name", Json::String(event.name.clone())),
        ("level", Json::String(format!("{:?}", event.level))),
        ("event_name", Json::String(event.event_name.clone())),
        ("fields", Json::Object(event.fields.clone())),
    ])
}

pub(crate) fn log_event_from_json(value: &Json) -> LogEvent {
    let values = object_values(value);
    LogEvent {
        name: string_from_json(values.get("name").unwrap_or(&Json::Null)),
        level: match string_from_json(values.get("level").unwrap_or(&Json::Null)).as_str() {
            "Debug" => LogLevel::Debug,
            "Warn" => LogLevel::Warn,
            "Error" => LogLevel::Error,
            _ => LogLevel::Info,
        },
        event_name: string_from_json(values.get("event_name").unwrap_or(&Json::Null)),
        fields: match values.get("fields").unwrap_or(&Json::Null) {
            Json::Object(fields) => fields.clone(),
            _ => BTreeMap::new(),
        },
    }
}

pub(crate) fn metric_definition_to_json(definition: &MetricDefinition) -> Json {
    object(vec![
        ("name", Json::String(definition.name.clone())),
        ("kind", Json::String(format!("{:?}", definition.kind))),
        (
            "backend_name",
            Json::String(definition.backend_name.clone()),
        ),
        ("unit", Json::String(definition.unit.clone())),
        ("labels", fields_to_json(&definition.labels)),
    ])
}

pub(crate) fn metric_definition_from_json(value: &Json) -> MetricDefinition {
    let values = object_values(value);
    MetricDefinition {
        name: string_from_json(values.get("name").unwrap_or(&Json::Null)),
        kind: match string_from_json(values.get("kind").unwrap_or(&Json::Null)).as_str() {
            "Gauge" => MetricKind::Gauge,
            "Histogram" => MetricKind::Histogram,
            _ => MetricKind::Counter,
        },
        backend_name: string_from_json(values.get("backend_name").unwrap_or(&Json::Null)),
        unit: string_from_json(values.get("unit").unwrap_or(&Json::Null)),
        labels: fields_from_json(values.get("labels").unwrap_or(&Json::Null)),
    }
}

pub(crate) fn metric_sample_to_json(sample: &MetricSample) -> Json {
    object(vec![
        ("name", Json::String(sample.name.clone())),
        ("kind", Json::String(format!("{:?}", sample.kind))),
        ("backend_name", Json::String(sample.backend_name.clone())),
        ("value", Json::Decimal(sample.value)),
        ("unit", Json::String(sample.unit.clone())),
        ("labels", Json::Object(sample.labels.clone())),
    ])
}

pub(crate) fn metric_sample_from_json(value: &Json) -> MetricSample {
    let values = object_values(value);
    MetricSample {
        name: string_from_json(values.get("name").unwrap_or(&Json::Null)),
        kind: match string_from_json(values.get("kind").unwrap_or(&Json::Null)).as_str() {
            "Gauge" => MetricKind::Gauge,
            "Histogram" => MetricKind::Histogram,
            _ => MetricKind::Counter,
        },
        backend_name: string_from_json(values.get("backend_name").unwrap_or(&Json::Null)),
        value: f64_from_json(values.get("value").unwrap_or(&Json::Null)),
        unit: string_from_json(values.get("unit").unwrap_or(&Json::Null)),
        labels: match values.get("labels").unwrap_or(&Json::Null) {
            Json::Object(labels) => labels.clone(),
            _ => BTreeMap::new(),
        },
    }
}
