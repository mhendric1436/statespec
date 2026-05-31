package backend_test

import (
	"context"
	"errors"
	"testing"
	"time"

	common "statespec-generated/common/backend"
	"statespec-generated/common/backend/memory"
	worker "statespec-generated/worker/backend"
)

type processWorkflowStepHandler struct {
	handled chan struct{}
}

func (h processWorkflowStepHandler) HandleValidateRequest(ctx context.Context, tx common.Transaction, step worker.WorkflowStepHandlerContext) (worker.WorkflowStepResult, error) {
	if !tx.IsOpen() {
		t := ctx.Value(testingContextKey{}).(*testing.T)
		t.Fatal("workflow handler received a closed transaction")
	}
	if step.WorkflowName != "ProvisionService" || step.StepName != "validate_request" {
		t := ctx.Value(testingContextKey{}).(*testing.T)
		t.Fatalf("unexpected workflow step: %#v", step)
	}
	select {
	case <-h.handled:
	default:
		close(h.handled)
	}
	nextStep := "create_remote_service"
	return worker.Complete(&nextStep), nil
}

func (h processWorkflowStepHandler) HandleCreateRemoteService(_ context.Context, tx common.Transaction, _ worker.WorkflowStepHandlerContext) (worker.WorkflowStepResult, error) {
	if !tx.IsOpen() {
		return worker.Fail("workflow handler received a closed transaction"), nil
	}
	nextStep := "wait_for_remote_service"
	return worker.Complete(&nextStep), nil
}

func (h processWorkflowStepHandler) HandleWaitForRemoteService(_ context.Context, tx common.Transaction, _ worker.WorkflowStepHandlerContext) (worker.WorkflowStepResult, error) {
	if !tx.IsOpen() {
		return worker.Fail("workflow handler received a closed transaction"), nil
	}
	return worker.Complete(nil), nil
}

func TestGeneratedWorkerProcessLifecycle(t *testing.T) {
	ctx := context.WithValue(context.Background(), testingContextKey{}, t)
	backend := memory.NewBackend()
	runtime := worker.NewWorkerTierRuntime(backend)
	handler := processWorkflowStepHandler{handled: make(chan struct{})}
	invokers := worker.WorkflowStepInvokerMap{}
	worker.RegisterProvisionServiceWorkflowStepInvokers(invokers, handler)
	config := worker.DefaultWorkerProcessConfig()
	config.WorkerPollInterval = time.Millisecond
	process := worker.NewWorkerProcessWithConfig(runtime, invokers, config)

	if err := process.Join(); err == nil {
		t.Fatal("joining a WorkerProcess before start should fail")
	}
	process.RequestStop()

	if err := runtime.Bootstrap(ctx); err != nil {
		t.Fatal(err)
	}
	_, err := runtime.Workflows.Start(ctx, backend, common.StartWorkflowRequest{
		WorkflowExecutionID: "wf-process-1",
		WorkflowName:        "ProvisionService",
		WorkflowVersion:     1,
		StartStep:           "validate_request",
		State:               common.JSONObject(map[string]common.JSON{}),
	})
	if err != nil {
		t.Fatal(err)
	}

	ctx, cancel := context.WithCancel(ctx)
	defer cancel()
	if err := process.Start(ctx); err != nil {
		t.Fatalf("starting WorkerProcess failed: %v", err)
	}
	if err := process.Start(ctx); err == nil {
		t.Fatal("starting WorkerProcess twice should fail")
	}
	time.Sleep(10 * time.Millisecond)
	if !process.Running() {
		t.Fatal("started WorkerProcess should report running")
	}

	select {
	case <-handler.handled:
	case <-time.After(2 * time.Second):
		t.Fatal("WorkerProcess did not execute a workflow step")
	}

	process.RequestStop()
	err = process.Join()
	if err != nil && !errors.Is(err, context.Canceled) {
		t.Fatalf("stopped WorkerProcess should join cleanly: %v", err)
	}
	if process.Running() {
		t.Fatal("joined WorkerProcess should not report running")
	}
}
