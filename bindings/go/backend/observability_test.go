package backend

import "testing"

func TestObservabilityEventsHoldTypedFields(t *testing.T) {
	event := LogEvent{
		Name:      "WorkflowLaunchDecision",
		Level:     LogLevelInfo,
		EventName: "workflow.launch.decision",
		Fields: map[string]JSON{
			"tenant_id": JSONString("tenant-a"),
			"decision":  JSONString("accepted"),
		},
	}

	if event.Level != LogLevelInfo || event.EventName != "workflow.launch.decision" {
		t.Fatalf("unexpected log event metadata")
	}
	if value, ok := event.Fields["tenant_id"].AsString(); !ok || value != "tenant-a" {
		t.Fatalf("expected typed tenant field")
	}
}

func TestMetricSamplesHoldKindValueAndLabels(t *testing.T) {
	sample := MetricSample{
		Name:        "WorkflowLaunchAttempts",
		Kind:        MetricCounter,
		BackendName: "workflow_launch_attempts_total",
		Value:       1,
		Unit:        "count",
		Labels: map[string]JSON{
			"workflow_name": JSONString("OrderProcessing"),
		},
	}

	if sample.Kind != MetricCounter || sample.BackendName != "workflow_launch_attempts_total" {
		t.Fatalf("unexpected metric metadata")
	}
	if sample.Value != 1 || sample.Unit != "count" {
		t.Fatalf("unexpected metric sample value")
	}
	if value, ok := sample.Labels["workflow_name"].AsString(); !ok || value != "OrderProcessing" {
		t.Fatalf("expected typed workflow label")
	}
}
