package backend_test

import (
	"context"
	"testing"
	"time"

	api "statespec-generated/api/backend"
	"statespec-generated/common/backend/memory"
)

func TestGeneratedAPIProcessConstructsMultipleServersAndStops(t *testing.T) {
	backend := memory.NewBackend()
	handler := api.DefaultAPITierHandler{Backend: backend}
	transport := api.NewLocalBlockingAPITierTransport()
	config := api.DefaultAPIProcessConfig()
	if len(config.ServerNames) != 2 {
		t.Fatalf("expected two API server descriptors, got %d", len(config.ServerNames))
	}

	process, err := api.NewAPIProcess(config, backend, handler, transport)
	if err != nil {
		t.Fatal(err)
	}
	if len(process.Applications) != 2 {
		t.Fatalf("expected two API applications, got %d", len(process.Applications))
	}

	notStarted, err := api.NewAPIProcess(config, backend, handler, api.NewLocalBlockingAPITierTransport())
	if err != nil {
		t.Fatal(err)
	}
	notStarted.RequestStop()
	if err := notStarted.Join(); err == nil {
		t.Fatal("join before start should fail")
	}

	if err := process.Start(context.Background()); err != nil {
		t.Fatal(err)
	}
	if !process.Running() {
		t.Fatal("API process did not report running")
	}
	if err := process.Start(context.Background()); err == nil {
		t.Fatal("API process allowed double start")
	}
	process.RequestStop()

	done := make(chan error, 1)
	go func() {
		done <- process.Join()
	}()

	select {
	case err := <-done:
		if err != nil {
			t.Fatal(err)
		}
	case <-time.After(2 * time.Second):
		t.Fatal("API process did not stop")
	}
	if process.Running() {
		t.Fatal("API process still reports running after join")
	}
}
