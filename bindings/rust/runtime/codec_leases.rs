use crate::json::Json;
use crate::lease::{LeaseDefinition, LeaseDefinitionId, LeaseRecord};

use super::codec_core::{
    bool_from_json, duration_from_json, duration_to_json, object, object_values, optional_duration,
    optional_duration_from_json, optional_string, optional_string_from_json, string_from_json,
    time_from_json, time_to_json, u64_from_json,
};

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
