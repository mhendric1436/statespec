use std::collections::BTreeMap;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

use crate::json::Json;

pub(crate) fn definition_key(parts: &[String]) -> String {
    parts.join(":")
}

pub(crate) fn optional_string(value: &Option<String>) -> Json {
    value.clone().map(Json::String).unwrap_or(Json::Null)
}

pub(crate) fn optional_string_from_json(value: &Json) -> Option<String> {
    match value {
        Json::Null => None,
        Json::String(value) => Some(value.clone()),
        _ => None,
    }
}

pub(crate) fn string_from_json(value: &Json) -> String {
    match value {
        Json::String(value) => value.clone(),
        _ => String::new(),
    }
}

pub(crate) fn i64_from_json(value: &Json) -> i64 {
    match value {
        Json::Integer(value) => *value,
        _ => 0,
    }
}

pub(crate) fn u64_from_json(value: &Json) -> u64 {
    i64_from_json(value).max(0) as u64
}

pub(crate) fn u32_from_json(value: &Json) -> u32 {
    i64_from_json(value).max(0) as u32
}

pub(crate) fn f64_from_json(value: &Json) -> f64 {
    match value {
        Json::Decimal(value) => *value,
        Json::Integer(value) => *value as f64,
        _ => 0.0,
    }
}

pub(crate) fn bool_from_json(value: &Json) -> bool {
    match value {
        Json::Bool(value) => *value,
        _ => false,
    }
}

pub(crate) fn duration_to_json(value: Duration) -> Json {
    Json::Integer(value.as_nanos() as i64)
}

pub(crate) fn duration_from_json(value: &Json) -> Duration {
    Duration::from_nanos(u64_from_json(value))
}

pub(crate) fn optional_duration(value: &Option<Duration>) -> Json {
    value.map(duration_to_json).unwrap_or(Json::Null)
}

pub(crate) fn optional_duration_from_json(value: &Json) -> Option<Duration> {
    match value {
        Json::Null => None,
        _ => Some(duration_from_json(value)),
    }
}

pub(crate) fn time_to_json(value: SystemTime) -> Json {
    let nanos = value
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_nanos() as i64;
    Json::Integer(nanos)
}

pub(crate) fn time_from_json(value: &Json) -> SystemTime {
    UNIX_EPOCH + Duration::from_nanos(u64_from_json(value))
}

pub(crate) fn optional_time(value: &Option<SystemTime>) -> Json {
    value.map(time_to_json).unwrap_or(Json::Null)
}

pub(crate) fn optional_time_from_json(value: &Json) -> Option<SystemTime> {
    match value {
        Json::Null => None,
        _ => Some(time_from_json(value)),
    }
}

pub(crate) fn object(values: Vec<(&str, Json)>) -> Json {
    Json::Object(
        values
            .into_iter()
            .map(|(key, value)| (key.to_string(), value))
            .collect(),
    )
}

pub(crate) fn object_values(value: &Json) -> BTreeMap<String, Json> {
    match value {
        Json::Object(values) => values.clone(),
        _ => BTreeMap::new(),
    }
}
