use crate::json::Json;
use crate::workflow::{WorkflowDefinition, WorkflowExecutionRecord, WorkflowStepDefinition};

use super::codec_core::{
    bool_from_json, duration_from_json, duration_to_json, i64_from_json, object, object_values,
    optional_string, optional_string_from_json, optional_time, optional_time_from_json,
    string_from_json, u32_from_json, u64_from_json,
};

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
