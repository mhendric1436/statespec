package backend_test

import (
	"context"
	"sync/atomic"
	"testing"
	"time"

	common "statespec-generated/common/backend"
	"statespec-generated/common/backend/memory"
	"statespec-generated/common/backend/runtime"
	worker "statespec-generated/worker/backend"
)

type countingWorkflowStore struct {
	*runtime.WorkflowStore
	keepAliveCount atomic.Int32
}

func newCountingWorkflowStore() *countingWorkflowStore {
	return &countingWorkflowStore{WorkflowStore: runtime.NewWorkflowStore()}
}

func (s *countingWorkflowStore) KeepAliveStep(ctx context.Context, backend common.Backend, request common.KeepAliveWorkflowStepRequest) (common.WorkflowExecutionRecord, error) {
	s.keepAliveCount.Add(1)
	return s.WorkflowStore.KeepAliveStep(ctx, backend, request)
}

type longRunningWorkflowStepHandler struct {
	handled bool
}

func (h *longRunningWorkflowStepHandler) HandleValidateRequest(ctx context.Context, tx common.Transaction, step worker.WorkflowStepHandlerContext) (worker.WorkflowStepResult, error) {
	if !tx.IsOpen() {
		t := ctx.Value(testingContextKey{}).(*testing.T)
		t.Fatal("workflow handler received a closed transaction")
	}
	if step.WorkflowName != "ProvisionService" || step.StepName != "validate_request" {
		t := ctx.Value(testingContextKey{}).(*testing.T)
		t.Fatalf("unexpected workflow step: %#v", step)
	}
	time.Sleep(850 * time.Millisecond)
	h.handled = true
	return worker.Complete(nil), nil
}

func (h *longRunningWorkflowStepHandler) HandleCreateRemoteService(context.Context, common.Transaction, worker.WorkflowStepHandlerContext) (worker.WorkflowStepResult, error) {
	return worker.Fail("unexpected create_remote_service step"), nil
}

func (h *longRunningWorkflowStepHandler) HandleWaitForRemoteService(context.Context, common.Transaction, worker.WorkflowStepHandlerContext) (worker.WorkflowStepResult, error) {
	return worker.Fail("unexpected wait_for_remote_service step"), nil
}

func TestLongRunningWorkflowStepReceivesPeriodicKeepAlive(t *testing.T) {
	ctx := context.WithValue(context.Background(), testingContextKey{}, t)
	backend := memory.NewBackend()
	workflows := newCountingWorkflowStore()

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
		WorkflowExecutionID: "wf-long",
		WorkflowName:        "ProvisionService",
		WorkflowVersion:     1,
		StartStep:           "validate_request",
		State:               common.JSONObject(map[string]common.JSON{}),
	})
	if err != nil {
		t.Fatal(err)
	}

	handler := &longRunningWorkflowStepHandler{}
	invokers := worker.WorkflowStepInvokerMap{}
	worker.RegisterProvisionServiceWorkflowStepInvokers(invokers, handler)
	runner := worker.WorkflowRunner{
		Backend:       backend,
		WorkflowStore: workflows,
		Invokers:      invokers,
		WorkerName:    "ProvisionWorker",
		LeaseDuration: time.Second,
		MaxAttempts:   3,
	}
	completed, err := runner.RunOnce(ctx, "wf-long", "ProvisionService", 1)
	if err != nil {
		t.Fatal(err)
	}
	if !handler.handled || completed == nil || completed.Status != "Completed" {
		t.Fatalf("long-running workflow step did not complete: handled=%v completed=%#v", handler.handled, completed)
	}
	if workflows.keepAliveCount.Load() < 2 {
		t.Fatalf("long-running workflow step did not receive periodic keepalive; count=%d", workflows.keepAliveCount.Load())
	}
}
