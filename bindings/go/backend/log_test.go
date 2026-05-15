package backend

import "testing"

func TestLogEventsHoldTypedFields(t *testing.T) {
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
