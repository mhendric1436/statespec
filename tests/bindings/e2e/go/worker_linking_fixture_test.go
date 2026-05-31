package backend_test

import (
	"context"
	"testing"
	"time"

	common "statespec-generated/common/backend"
	"statespec-generated/common/backend/memory"
	"statespec-generated/common/backend/runtime"
	worker "statespec-generated/worker/backend"
)

type linkingWorkflowStepHandler struct {
	handled bool
}

func (h *linkingWorkflowStepHandler) HandleValidateRequest(ctx context.Context, tx common.Transaction, step worker.WorkflowStepHandlerContext) (worker.WorkflowStepResult, error) {
	if !tx.IsOpen() {
		t := ctx.Value(testingContextKey{}).(*testing.T)
		t.Fatal("workflow handler received a closed transaction")
	}
	if step.WorkflowName != "ProvisionService" || step.StepName != "validate_request" {
		t := ctx.Value(testingContextKey{}).(*testing.T)
		t.Fatalf("unexpected workflow step: %#v", step)
	}
	h.handled = true
	nextStep := "create_remote_service"
	return worker.Complete(&nextStep), nil
}

func (h *linkingWorkflowStepHandler) HandleCreateRemoteService(context.Context, common.Transaction, worker.WorkflowStepHandlerContext) (worker.WorkflowStepResult, error) {
	return worker.Fail("unexpected create_remote_service step"), nil
}

func (h *linkingWorkflowStepHandler) HandleWaitForRemoteService(context.Context, common.Transaction, worker.WorkflowStepHandlerContext) (worker.WorkflowStepResult, error) {
	return worker.Fail("unexpected wait_for_remote_service step"), nil
}

type testingContextKey struct{}

func TestGeneratedWorkerRunnerLinksWithMemoryBackend(t *testing.T) {
	ctx := context.WithValue(context.Background(), testingContextKey{}, t)
	backend := memory.NewBackend()
	workflows := runtime.NewWorkflowStore()

	_, err := workflows.RegisterDefinition(ctx, backend, common.RegisterWorkflowDefinitionRequest{
		Definition: common.WorkflowDefinition{
			WorkflowName:          "ProvisionService",
			WorkflowVersion:       1,
			StartStep:             "validate_request",
			ExpectedExecutionTime: time.Minute,
			Steps: []common.WorkflowStepDefinition{
				{Name: "validate_request", ExpectedExecutionTime: time.Second, MaxRetries: 3},
				{Name: "create_remote_service", ExpectedExecutionTime: time.Second, MaxRetries: 3},
				{Name: "wait_for_remote_service", ExpectedExecutionTime: time.Second, MaxRetries: 3},
			},
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
	invokers := worker.WorkflowStepInvokerMap{}
	worker.RegisterProvisionServiceWorkflowStepInvokers(invokers, handler)
	runner := worker.WorkflowRunner{
		Backend:       backend,
		WorkflowStore: workflows,
		Invokers:      invokers,
		WorkerName:    "ProvisionWorker",
		LeaseDuration: time.Minute,
		MaxAttempts:   3,
	}
	advanced, err := runner.RunOnce(ctx, "wf-1", "ProvisionService", 1)
	if err != nil {
		t.Fatal(err)
	}
	if !handler.handled || advanced == nil || advanced.Status != "Running" || advanced.CurrentStep != "create_remote_service" {
		t.Fatalf("worker runner did not advance linked workflow step: handled=%v advanced=%#v", handler.handled, advanced)
	}
}
