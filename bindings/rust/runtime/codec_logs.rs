use std::collections::BTreeMap;

use crate::json::Json;
use crate::log::{LogDefinition, LogEvent, LogLevel};

use super::codec_core::{object, object_values, string_from_json};
use super::codec_observability::{fields_from_json, fields_to_json};

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
