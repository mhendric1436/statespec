#[cfg(test)]
mod tests {
    use std::collections::BTreeMap;

    use crate::json::Json;
    use crate::metric::{MetricKind, MetricSample};

    #[test]
    fn metric_samples_hold_kind_value_and_labels() {
        let mut labels = BTreeMap::new();
        labels.insert(
            "workflow_name".to_string(),
            Json::String("OrderProcessing".to_string()),
        );

        let sample = MetricSample {
            name: "WorkflowLaunchAttempts".to_string(),
            kind: MetricKind::Counter,
            backend_name: "workflow_launch_attempts_total".to_string(),
            value: 1.0,
            unit: "count".to_string(),
            labels,
        };

        assert_eq!(sample.kind, MetricKind::Counter);
        assert_eq!(sample.backend_name, "workflow_launch_attempts_total");
        assert_eq!(sample.value, 1.0);
        assert_eq!(
            sample.labels.get("workflow_name"),
            Some(&Json::String("OrderProcessing".to_string()))
        );
    }
}
