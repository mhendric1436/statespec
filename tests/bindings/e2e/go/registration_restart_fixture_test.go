package backend_test

import (
	"context"
	"testing"

	common "statespec-generated/common/backend"
	"statespec-generated/common/backend/memory"
	"statespec-generated/common/backend/runtime"
)

func TestGeneratedRuntimeCatalogBootstrapIsRestartSafe(t *testing.T) {
	ctx := context.Background()
	backend := memory.NewBackend()

	bootstrap := func() error {
		return common.BootstrapRuntimeCatalog(
			ctx,
			backend,
			runtime.NewQueueStore(),
			runtime.NewLeaseStore(),
			runtime.NewWorkflowStore(),
		)
	}

	if err := bootstrap(); err != nil {
		t.Fatal(err)
	}
	if err := bootstrap(); err != nil {
		t.Fatal(err)
	}
}
