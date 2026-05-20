package backend_test

import (
	"context"
	"testing"
	"time"

	worker "statespec-generated/worker/backend"
	common "statespec-generated/common/backend"
	"statespec-generated/common/backend/memory"
)

type linkingWorkflowStepHandler struct {
	handled bool
}

func (h *linkingWorkflowStepHandler) HandleWorkflowStep(ctx context.Context, step worker.WorkflowStepHandlerContext) error {
	if step.WorkflowName != "ProvisionService" || step.StepName != "validate_request" {
		t := ctx.Value(testingContextKey{}).(*testing.T)
		t.Fatalf("unexpected workflow step: %#v", step)
	}
	h.handled = true
	return nil
}

type testingContextKey struct{}

func TestGeneratedWorkerRunnerLinksWithMemoryBackend(t *testing.T) {
	ctx := context.WithValue(context.Background(), testingContextKey{}, t)
	backend := memory.NewBackend()
	workflows := memory.NewWorkflowStore()

	_, err := workflows.RegisterDefinition(ctx, backend, common.RegisterWorkflowDefinitionRequest{
		Definition: common.WorkflowDefinition{
			WorkflowName:          "ProvisionService",
			WorkflowVersion:       1,
			StartStep:             "validate_request",
			ExpectedExecutionTime: time.Minute,
			Steps: []common.WorkflowStepDefinition{{
				Name:                  "validate_request",
				ExpectedExecutionTime: time.Second,
				MaxRetries:            3,
			}},
			Metadata: common.JSONObject(map[string]common.JSON{}),
		},
	})
	if err != nil {
		t.Fatal(err)
	}
	_, err = workflows.Start(ctx, backend, common.StartWorkflowRequest{
		WorkflowExecutionID: "wf-1",
		WorkflowName:        "ProvisionService",
		WorkflowVersion:     1,
		StartStep:           "validate_request",
		State:               common.JSONObject(map[string]common.JSON{}),
	})
	if err != nil {
		t.Fatal(err)
	}

	handler := &linkingWorkflowStepHandler{}
	runner := worker.WorkflowRunner{
		Backend:       backend,
		WorkflowStore: workflows,
		Handler:       handler,
		WorkerName:    "ProvisionWorker",
		LeaseDuration: time.Minute,
		MaxAttempts:   3,
	}
	completed, err := runner.RunOnce(ctx, "wf-1", "ProvisionService", 1)
	if err != nil {
		t.Fatal(err)
	}
	if !handler.handled || completed == nil || completed.Status != "Completed" {
		t.Fatalf("worker runner did not complete linked workflow step: handled=%v completed=%#v", handler.handled, completed)
	}
}
