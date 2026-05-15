package backend

import "testing"

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
