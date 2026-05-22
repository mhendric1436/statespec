package runtime_test

import (
	"context"
	"errors"
	"strings"
	"testing"
	"time"

	common "statespec-generated/common/backend"
	"statespec-generated/common/backend/memory"
	"statespec-generated/common/backend/runtime"
)

func TestMemoryBackendStoresComposeInTransaction(t *testing.T) {
	ctx := context.Background()
	backend := memory.NewBackend()
	flags := runtime.NewFeatureFlagStore()
	queues := runtime.NewQueueStore()
	leases := runtime.NewLeaseStore()
	workflows := runtime.NewWorkflowStore()
	logs := runtime.NewLogSink()
	metrics := runtime.NewMetricSink()

	tx, err := backend.Begin(ctx)
	if err != nil {
		t.Fatal(err)
	}

	if _, err := flags.RegisterDefinitionTx(ctx, tx, common.FeatureFlagDefinition{
		Name:         "new_scheduler",
		Type:         common.FeatureFlagBool,
		DefaultValue: common.BoolFeatureFlagValue(true),
		Scope:        common.FeatureFlagScopeSystem,
	}); err != nil {
		t.Fatal(err)
	}
	if _, err := queues.RegisterDefinitionTx(ctx, tx, common.RegisterQueueDefinitionRequest{Definition: common.QueueDefinition{
		Queue:             "workflow",
		Channel:           "launch",
		VisibilityTimeout: time.Minute,
		MaxAttempts:       3,
		Metadata:          common.JSONObject(map[string]common.JSON{}),
	}}); err != nil {
		t.Fatal(err)
	}
	if _, err := leases.RegisterDefinitionTx(ctx, tx, common.LeaseDefinition{
		ID:              common.LeaseDefinitionID{Name: "workflow_step", Version: 1},
		ResourcePattern: "workflow/*",
		TTL:             time.Minute,
		RenewEvery:      30 * time.Second,
		FencingToken:    true,
	}); err != nil {
		t.Fatal(err)
	}
	if _, err := workflows.RegisterDefinitionTx(ctx, tx, common.RegisterWorkflowDefinitionRequest{Definition: common.WorkflowDefinition{
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
	}}); err != nil {
		t.Fatal(err)
	}
	if _, err := logs.RegisterDefinitionTx(ctx, tx, common.LogSignalDefinition{Name: "launch_decision", Level: common.LogLevelInfo, EventName: "workflow.launch.decision"}); err != nil {
		t.Fatal(err)
	}
	if _, err := metrics.RegisterDefinitionTx(ctx, tx, common.MetricInstrumentDefinition{Name: "launch_attempts", Kind: common.MetricCounter, BackendName: "workflow_launch_attempts_total", Unit: "1"}); err != nil {
		t.Fatal(err)
	}
	if err := backend.Commit(ctx, tx); err != nil {
		t.Fatal(err)
	}

	flag, err := flags.Evaluate(ctx, backend, common.FeatureFlagEvaluationRequest{Name: "new_scheduler"})
	if err != nil {
		t.Fatal(err)
	}
	if value, ok := flag.AsBool(); !ok || !value {
		t.Fatalf("unexpected flag value: %#v", flag)
	}

	now := time.Unix(100, 0)
	msg, err := queues.Enqueue(ctx, backend, common.EnqueueMessageRequest{MessageID: "msg-1", Queue: "workflow", Channel: "launch", Payload: common.JSONObject(map[string]common.JSON{})})
	if err != nil || msg.Status != "Pending" {
		t.Fatalf("enqueue failed: %v %#v", err, msg)
	}
	claimed, err := queues.Claim(ctx, backend, common.ClaimMessageRequest{Queue: "workflow", Channel: "launch", Claimant: "worker-1", Now: now, VisibilityTimeout: time.Minute, MaxMessages: 1})
	if err != nil || len(claimed) != 1 {
		t.Fatalf("claim failed: %v %#v", err, claimed)
	}

	lease, err := leases.Acquire(ctx, backend, common.LeaseAcquireRequest{DefinitionID: common.LeaseDefinitionID{Name: "workflow_step", Version: 1}, Resource: "workflow/msg-1", Holder: "worker-1", Now: now})
	if err != nil || !lease.Acquired || lease.Lease == nil {
		t.Fatalf("lease acquire failed: %v %#v", err, lease)
	}

	started, err := workflows.Start(ctx, backend, common.StartWorkflowRequest{WorkflowExecutionID: "wf-1", WorkflowName: "ProvisionService", WorkflowVersion: 1, StartStep: "validate_request", State: common.JSONObject(map[string]common.JSON{})})
	if err != nil || started.Status != "Running" {
		t.Fatalf("start failed: %v %#v", err, started)
	}
	steps, err := workflows.ClaimSteps(ctx, backend, common.ClaimWorkflowStepRequest{WorkflowExecutionID: "wf-1", WorkflowName: "ProvisionService", WorkflowVersion: 1, Worker: "worker-1", Now: now, LeaseDuration: time.Minute, MaxSteps: 1})
	if err != nil || len(steps) != 1 {
		t.Fatalf("workflow claim failed: %v %#v", err, steps)
	}
	if _, err := workflows.KeepAliveStep(ctx, backend, common.KeepAliveWorkflowStepRequest{WorkflowExecutionID: "wf-1", Worker: "worker-1", CurrentStep: "validate_request", Now: now, LeaseDuration: time.Minute}); err != nil {
		t.Fatal(err)
	}

	if err := logs.EmitLog(ctx, backend, common.LogEvent{Name: "launch_decision", Level: common.LogLevelInfo, EventName: "workflow.launch.decision"}); err != nil {
		t.Fatal(err)
	}
	if err := metrics.RecordMetric(ctx, backend, common.MetricSample{Name: "launch_attempts", Kind: common.MetricCounter, BackendName: "workflow_launch_attempts_total", Value: 1, Unit: "1"}); err != nil {
		t.Fatal(err)
	}
	events, err := logs.InspectEvents(ctx, backend)
	if err != nil || len(events) != 1 {
		t.Fatalf("log inspect failed: %v %#v", err, events)
	}
	samples, err := metrics.InspectSamples(ctx, backend)
	if err != nil || len(samples) != 1 {
		t.Fatalf("metric inspect failed: %v %#v", err, samples)
	}

	writeTx, err := backend.Begin(ctx)
	if err != nil {
		t.Fatal(err)
	}
	if err := backend.Put(ctx, writeTx, common.CollectionName("orders"), common.Key("order-1"), common.JSONObject(map[string]common.JSON{"status": common.JSONString("new")})); err != nil {
		t.Fatal(err)
	}
	if err := backend.Commit(ctx, writeTx); err != nil {
		t.Fatal(err)
	}
	readTx, err := backend.Begin(ctx)
	if err != nil {
		t.Fatal(err)
	}
	records, err := backend.Query(ctx, readTx, common.CollectionName("orders"), common.KeyPrefixQuery("order-"))
	if err != nil || len(records) != 1 {
		t.Fatalf("backend query failed: %v %#v", err, records)
	}
	if err := backend.Commit(ctx, readTx); err != nil {
		t.Fatal(err)
	}
}

func TestMemoryBackendEnforcesCompatibleCollectionRegistration(t *testing.T) {
	ctx := context.Background()
	backend := memory.NewBackend()
	base := testCollectionDescriptor()

	if err := backend.EnsureCollection(ctx, base); err != nil {
		t.Fatal(err)
	}
	if err := backend.EnsureCollection(ctx, base); err != nil {
		t.Fatal(err)
	}
	if err := backend.EnsureCollection(ctx, compatibleCollectionDescriptor()); err != nil {
		t.Fatal(err)
	}

	incompatible := compatibleCollectionDescriptor()
	incompatible.SchemaVersion = 3
	incompatible.KeyFields = []string{"tenant_id", "order_id"}

	err := backend.EnsureCollection(ctx, incompatible)
	var conflict common.ConflictError
	if !errors.As(err, &conflict) || conflict.Kind != common.SchemaConflict {
		t.Fatalf("expected schema conflict, got %T %v", err, err)
	}
	if conflict.Message == "" ||
		!strings.Contains(conflict.Message, "orders") ||
		!strings.Contains(conflict.Message, "KeyFieldsChanged") {
		t.Fatalf("expected structured schema conflict message, got %q", conflict.Message)
	}
}

func TestMemoryBackendAppliesCollectionBatchesAtomically(t *testing.T) {
	ctx := context.Background()
	backend := memory.NewBackend()
	if err := backend.EnsureCollection(ctx, testCollectionDescriptor()); err != nil {
		t.Fatal(err)
	}

	incompatible := testCollectionDescriptor()
	incompatible.SchemaVersion = 2
	incompatible.KeyFields = []string{"tenant_id", "order_id"}
	if err := backend.EnsureCollections(ctx, []common.CollectionDescriptor{
		compatibleCollectionDescriptor(),
		incompatible,
	}); err == nil {
		t.Fatal("expected schema conflict")
	}

	if err := backend.EnsureCollection(ctx, alternateCompatibleCollectionDescriptor()); err != nil {
		t.Fatalf("batch failure partially applied earlier descriptor: %v", err)
	}
}

func testCollectionDescriptor() common.CollectionDescriptor {
	return common.CollectionDescriptor{
		Name: "orders",
		Fields: []common.FieldDescriptor{
			{
				Name:     "tenant_id",
				Type:     common.FieldTypeString,
				TypeName: "string",
				Required: true,
			},
			{
				Name:     "status",
				Type:     common.FieldTypeString,
				TypeName: "string",
				Required: false,
			},
		},
		KeyFields: []string{"tenant_id"},
		Indexes: []common.IndexDescriptor{
			{
				Name:   "by_status",
				Fields: []string{"status"},
				Unique: false,
			},
		},
		SchemaVersion: 1,
	}
}

func compatibleCollectionDescriptor() common.CollectionDescriptor {
	descriptor := testCollectionDescriptor()
	descriptor.SchemaVersion = 2
	descriptor.Fields = append(descriptor.Fields, common.FieldDescriptor{
		Name:     "description",
		Type:     common.FieldTypeString,
		TypeName: "string",
		Required: false,
	})
	return descriptor
}

func alternateCompatibleCollectionDescriptor() common.CollectionDescriptor {
	descriptor := testCollectionDescriptor()
	descriptor.SchemaVersion = 2
	descriptor.Fields = append(descriptor.Fields, common.FieldDescriptor{
		Name:     "notes",
		Type:     common.FieldTypeString,
		TypeName: "string",
		Required: false,
	})
	return descriptor
}
