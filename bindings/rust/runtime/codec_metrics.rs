use std::collections::BTreeMap;

use crate::json::Json;
use crate::metric::{MetricDefinition, MetricKind, MetricSample};

use super::codec_core::{f64_from_json, object, object_values, string_from_json};
use super::codec_observability::{fields_from_json, fields_to_json};

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
