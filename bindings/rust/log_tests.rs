#[cfg(test)]
mod tests {
    use std::collections::BTreeMap;

    use crate::json::Json;
    use crate::log::{LogEvent, LogLevel};

    #[test]
    fn log_events_hold_typed_json_fields() {
        let mut fields = BTreeMap::new();
        fields.insert(
            "tenant_id".to_string(),
            Json::String("tenant-a".to_string()),
        );
        fields.insert("decision".to_string(), Json::String("accepted".to_string()));

        let event = LogEvent {
            name: "WorkflowLaunchDecision".to_string(),
            level: LogLevel::Info,
            event_name: "workflow.launch.decision".to_string(),
            fields,
        };

        assert_eq!(event.level, LogLevel::Info);
        assert_eq!(event.event_name, "workflow.launch.decision");
        assert_eq!(
            event.fields.get("tenant_id"),
            Some(&Json::String("tenant-a".to_string()))
        );
    }
}
