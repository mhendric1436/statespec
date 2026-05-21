use crate::json::Json;
use crate::queue::{QueueDefinition, QueueMessageRecord};

use super::codec_core::{
    duration_from_json, duration_to_json, object, object_values, optional_string,
    optional_string_from_json, optional_time, optional_time_from_json, string_from_json,
    u32_from_json, u64_from_json,
};

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
