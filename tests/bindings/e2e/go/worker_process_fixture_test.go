package backend_test

import (
	"context"
	"errors"
	"testing"
	"time"

	"statespec-generated/common/backend/memory"
	worker "statespec-generated/worker/backend"
)

func TestGeneratedWorkerProcessLifecycle(t *testing.T) {
	backend := memory.NewBackend()
	runtime := worker.NewWorkerTierRuntime(backend)
	handler := worker.DefaultWorkflowStepHandler{}
	process := worker.NewWorkerProcess(runtime, handler)

	if err := process.Join(); err == nil {
		t.Fatal("joining a WorkerProcess before start should fail")
	}
	process.RequestStop()

	ctx, cancel := context.WithCancel(context.Background())
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

	process.RequestStop()
	err := process.Join()
	if err != nil && !errors.Is(err, context.Canceled) {
		t.Fatalf("stopped WorkerProcess should join cleanly: %v", err)
	}
}
