use std::collections::BTreeMap;
use std::time::{Duration, SystemTime, UNIX_EPOCH};

use crate::backend::{FieldDescriptor, FieldType};
use crate::feature_flag::{
    FeatureFlagDefinition, FeatureFlagScopeKind, FeatureFlagType, FeatureFlagValue,
};
use crate::json::Json;
use crate::lease::{LeaseDefinition, LeaseDefinitionId, LeaseRecord};
use crate::log::{LogDefinition, LogEvent, LogLevel};
use crate::metric::{MetricDefinition, MetricKind, MetricSample};
use crate::queue::{QueueDefinition, QueueMessageRecord};
use crate::workflow::{WorkflowDefinition, WorkflowExecutionRecord, WorkflowStepDefinition};

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

fn object(values: Vec<(&str, Json)>) -> Json {
    Json::Object(
        values
            .into_iter()
            .map(|(key, value)| (key.to_string(), value))
            .collect(),
    )
}

fn object_values(value: &Json) -> BTreeMap<String, Json> {
    match value {
        Json::Object(values) => values.clone(),
        _ => BTreeMap::new(),
    }
}

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

pub(crate) fn queue_definition_to_json(definition: &QueueDefinition) -> Json {
    object(vec![
        ("queue", Json::String(definition.queue.clone())),
        ("channel", Json::String(definition.channel.clone())),
        (
            "visibility_timeout",
            duration_to_json(definition.visibility_timeout),
        ),
        (
            "max_attempts",
            Json::Integer(definition.max_attempts as i64),
        ),
        (
            "dead_letter_queue",
            optional_string(&definition.dead_letter_queue),
        ),
        ("metadata", definition.metadata.clone()),
    ])
}

pub(crate) fn queue_definition_from_json(value: &Json) -> QueueDefinition {
    let values = object_values(value);
    QueueDefinition {
        queue: string_from_json(values.get("queue").unwrap_or(&Json::Null)),
        channel: string_from_json(values.get("channel").unwrap_or(&Json::Null)),
        visibility_timeout: duration_from_json(
            values.get("visibility_timeout").unwrap_or(&Json::Null),
        ),
        max_attempts: u32_from_json(values.get("max_attempts").unwrap_or(&Json::Null)),
        dead_letter_queue: optional_string_from_json(
            values.get("dead_letter_queue").unwrap_or(&Json::Null),
        ),
        metadata: values.get("metadata").cloned().unwrap_or(Json::Null),
    }
}

pub(crate) fn queue_message_to_json(message: &QueueMessageRecord) -> Json {
    object(vec![
        ("message_id", Json::String(message.message_id.clone())),
        ("queue", Json::String(message.queue.clone())),
        ("channel", Json::String(message.channel.clone())),
        ("status", Json::String(message.status.clone())),
        ("attempts", Json::Integer(message.attempts as i64)),
        ("claimed_by", optional_string(&message.claimed_by)),
        ("claim_expires_at", optional_time(&message.claim_expires_at)),
        ("payload", message.payload.clone()),
    ])
}

pub(crate) fn queue_message_from_json(value: &Json) -> QueueMessageRecord {
    let values = object_values(value);
    QueueMessageRecord {
        message_id: string_from_json(values.get("message_id").unwrap_or(&Json::Null)),
        queue: string_from_json(values.get("queue").unwrap_or(&Json::Null)),
        channel: string_from_json(values.get("channel").unwrap_or(&Json::Null)),
        status: string_from_json(values.get("status").unwrap_or(&Json::Null)),
        attempts: u64_from_json(values.get("attempts").unwrap_or(&Json::Null)),
        claimed_by: optional_string_from_json(values.get("claimed_by").unwrap_or(&Json::Null)),
        claim_expires_at: optional_time_from_json(
            values.get("claim_expires_at").unwrap_or(&Json::Null),
        ),
        payload: values.get("payload").cloned().unwrap_or(Json::Null),
    }
}

pub(crate) fn lease_definition_id_to_json(id: &LeaseDefinitionId) -> Json {
    object(vec![
        ("name", Json::String(id.name.clone())),
        ("version", Json::Integer(id.version as i64)),
    ])
}

pub(crate) fn lease_definition_id_from_json(value: &Json) -> LeaseDefinitionId {
    let values = object_values(value);
    LeaseDefinitionId {
        name: string_from_json(values.get("name").unwrap_or(&Json::Null)),
        version: u64_from_json(values.get("version").unwrap_or(&Json::Null)),
    }
}

pub(crate) fn lease_definition_to_json(definition: &LeaseDefinition) -> Json {
    object(vec![
        ("id", lease_definition_id_to_json(&definition.id)),
        (
            "resource_pattern",
            Json::String(definition.resource_pattern.clone()),
        ),
        ("ttl", duration_to_json(definition.ttl)),
        ("renew_every", duration_to_json(definition.renew_every)),
        ("max_ttl", optional_duration(&definition.max_ttl)),
        ("fencing_token", Json::Bool(definition.fencing_token)),
    ])
}

pub(crate) fn lease_definition_from_json(value: &Json) -> LeaseDefinition {
    let values = object_values(value);
    LeaseDefinition {
        id: lease_definition_id_from_json(values.get("id").unwrap_or(&Json::Null)),
        resource_pattern: string_from_json(values.get("resource_pattern").unwrap_or(&Json::Null)),
        ttl: duration_from_json(values.get("ttl").unwrap_or(&Json::Null)),
        renew_every: duration_from_json(values.get("renew_every").unwrap_or(&Json::Null)),
        max_ttl: optional_duration_from_json(values.get("max_ttl").unwrap_or(&Json::Null)),
        fencing_token: bool_from_json(values.get("fencing_token").unwrap_or(&Json::Null)),
    }
}

pub(crate) fn lease_record_to_json(record: &LeaseRecord) -> Json {
    object(vec![
        (
            "definition_id",
            lease_definition_id_to_json(&record.definition_id),
        ),
        ("resource", Json::String(record.resource.clone())),
        ("holder", optional_string(&record.holder)),
        ("expires_at", time_to_json(record.expires_at)),
        ("fencing_token", Json::Integer(record.fencing_token as i64)),
    ])
}

pub(crate) fn lease_record_from_json(value: &Json) -> LeaseRecord {
    let values = object_values(value);
    LeaseRecord {
        definition_id: lease_definition_id_from_json(
            values.get("definition_id").unwrap_or(&Json::Null),
        ),
        resource: string_from_json(values.get("resource").unwrap_or(&Json::Null)),
        holder: optional_string_from_json(values.get("holder").unwrap_or(&Json::Null)),
        expires_at: time_from_json(values.get("expires_at").unwrap_or(&Json::Null)),
        fencing_token: u64_from_json(values.get("fencing_token").unwrap_or(&Json::Null)),
    }
}

pub(crate) fn workflow_definition_to_json(definition: &WorkflowDefinition) -> Json {
    let steps = definition
        .steps
        .iter()
        .map(|step| {
            object(vec![
                ("name", Json::String(step.name.clone())),
                (
                    "expected_execution_time",
                    duration_to_json(step.expected_execution_time),
                ),
                ("max_retries", Json::Integer(step.max_retries as i64)),
            ])
        })
        .collect();
    object(vec![
        (
            "workflow_name",
            Json::String(definition.workflow_name.clone()),
        ),
        (
            "workflow_version",
            Json::Integer(definition.workflow_version),
        ),
        ("start_step", Json::String(definition.start_step.clone())),
        (
            "expected_execution_time",
            duration_to_json(definition.expected_execution_time),
        ),
        ("singleton", Json::Bool(definition.singleton)),
        ("steps", Json::Array(steps)),
        ("metadata", definition.metadata.clone()),
    ])
}

pub(crate) fn workflow_definition_from_json(value: &Json) -> WorkflowDefinition {
    let values = object_values(value);
    let steps = match values.get("steps").unwrap_or(&Json::Null) {
        Json::Array(values) => values
            .iter()
            .map(|value| {
                let step = object_values(value);
                WorkflowStepDefinition {
                    name: string_from_json(step.get("name").unwrap_or(&Json::Null)),
                    expected_execution_time: duration_from_json(
                        step.get("expected_execution_time").unwrap_or(&Json::Null),
                    ),
                    max_retries: u32_from_json(step.get("max_retries").unwrap_or(&Json::Null)),
                }
            })
            .collect(),
        _ => Vec::new(),
    };
    WorkflowDefinition {
        workflow_name: string_from_json(values.get("workflow_name").unwrap_or(&Json::Null)),
        workflow_version: i64_from_json(values.get("workflow_version").unwrap_or(&Json::Null)),
        start_step: string_from_json(values.get("start_step").unwrap_or(&Json::Null)),
        expected_execution_time: duration_from_json(
            values.get("expected_execution_time").unwrap_or(&Json::Null),
        ),
        singleton: bool_from_json(values.get("singleton").unwrap_or(&Json::Null)),
        steps,
        metadata: values.get("metadata").cloned().unwrap_or(Json::Null),
    }
}

pub(crate) fn workflow_execution_to_json(execution: &WorkflowExecutionRecord) -> Json {
    object(vec![
        (
            "workflow_execution_id",
            Json::String(execution.workflow_execution_id.clone()),
        ),
        (
            "workflow_name",
            Json::String(execution.workflow_name.clone()),
        ),
        (
            "workflow_version",
            Json::Integer(execution.workflow_version),
        ),
        ("current_step", Json::String(execution.current_step.clone())),
        ("status", Json::String(execution.status.clone())),
        ("attempt", Json::Integer(execution.attempt as i64)),
        ("claimed_by", optional_string(&execution.claimed_by)),
        (
            "claim_expires_at",
            optional_time(&execution.claim_expires_at),
        ),
        ("state", execution.state.clone()),
    ])
}

pub(crate) fn workflow_execution_from_json(value: &Json) -> WorkflowExecutionRecord {
    let values = object_values(value);
    WorkflowExecutionRecord {
        workflow_execution_id: string_from_json(
            values.get("workflow_execution_id").unwrap_or(&Json::Null),
        ),
        workflow_name: string_from_json(values.get("workflow_name").unwrap_or(&Json::Null)),
        workflow_version: i64_from_json(values.get("workflow_version").unwrap_or(&Json::Null)),
        current_step: string_from_json(values.get("current_step").unwrap_or(&Json::Null)),
        status: string_from_json(values.get("status").unwrap_or(&Json::Null)),
        attempt: u64_from_json(values.get("attempt").unwrap_or(&Json::Null)),
        claimed_by: optional_string_from_json(values.get("claimed_by").unwrap_or(&Json::Null)),
        claim_expires_at: optional_time_from_json(
            values.get("claim_expires_at").unwrap_or(&Json::Null),
        ),
        state: values.get("state").cloned().unwrap_or(Json::Null),
    }
}

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
