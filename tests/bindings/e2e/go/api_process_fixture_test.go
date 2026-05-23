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

	done := make(chan error, 1)
	go func() {
		done <- process.Run(context.Background())
	}()
	process.RequestStop()

	select {
	case err := <-done:
		if err != nil {
			t.Fatal(err)
		}
	case <-time.After(2 * time.Second):
		t.Fatal("API process did not stop")
	}
}
