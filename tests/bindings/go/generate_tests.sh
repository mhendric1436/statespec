#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TESTS_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
. "$TESTS_DIR/cli/common.sh"

SPEC="$TESTS_DIR/fixtures/bindings-full.sspec"

run_expect_status 0 "$CLI" generate bindings --lang go "$SPEC" --out "$TMPDIR/out-go"
assert_output_contains "generated $TMPDIR/out-go/common/backend/backend.go"
assert_file_exists "$TMPDIR/out-go/common/backend/json.go"
assert_file_exists "$TMPDIR/out-go/common/backend/backend.go"
assert_file_exists "$TMPDIR/out-go/common/backend/external_system.go"
assert_file_exists "$TMPDIR/out-go/common/backend/feature_flag.go"
assert_file_exists "$TMPDIR/out-go/common/backend/lease.go"
assert_file_exists "$TMPDIR/out-go/common/backend/log.go"
assert_file_exists "$TMPDIR/out-go/common/backend/metric.go"
assert_file_exists "$TMPDIR/out-go/common/backend/queue.go"
assert_file_exists "$TMPDIR/out-go/common/backend/workflow.go"
assert_file_exists "$TMPDIR/out-go/common/backend/memory/backend.go"
assert_file_exists "$TMPDIR/out-go/common/backend/memory/transaction.go"
assert_file_exists "$TMPDIR/out-go/common/backend/memory/codec.go"
assert_file_exists "$TMPDIR/out-go/common/backend/memory/feature_flags.go"
assert_file_exists "$TMPDIR/out-go/common/backend/memory/queues.go"
assert_file_exists "$TMPDIR/out-go/common/backend/memory/leases.go"
assert_file_exists "$TMPDIR/out-go/common/backend/memory/workflows.go"
assert_file_exists "$TMPDIR/out-go/common/backend/memory/logs.go"
assert_file_exists "$TMPDIR/out-go/common/backend/memory/metrics.go"
assert_file_exists "$TMPDIR/out-go/common/backend/descriptors.go"
assert_file_exists "$TMPDIR/out-go/go.mod"
assert_file_exists "$TMPDIR/out-go/Makefile"
assert_file_exists "$TMPDIR/out-go/api/backend/api_descriptors.go"
assert_file_exists "$TMPDIR/out-go/api/backend/api_handlers.go"
assert_file_exists "$TMPDIR/out-go/api/backend/api_dispatcher.go"
assert_file_exists "$TMPDIR/out-go/api/backend/api_server.go"
assert_file_exists "$TMPDIR/out-go/api/backend/api_routes.go"
assert_file_exists "$TMPDIR/out-go/api/backend/external_system_operator_metadata_api.go"
assert_file_exists "$TMPDIR/out-go/worker/backend/worker_descriptors.go"
assert_file_exists "$TMPDIR/out-go/worker/backend/worker_contexts.go"
assert_file_exists "$TMPDIR/out-go/worker/backend/worker_registry.go"
assert_file_exists "$TMPDIR/out-go/worker/backend/worker_application.go"
assert_file_exists "$TMPDIR/out-go/worker/backend/workflow_step_handlers.go"
assert_file_exists "$TMPDIR/out-go/worker/backend/workflow_runner.go"
assert_file_exists "$TMPDIR/out-go/worker/backend/worker_handlers.go"
assert_file_exists "$TMPDIR/out-go/worker/backend/worker_queues.go"
assert_file_exists "$TMPDIR/out-go/worker/backend/worker_leases.go"
assert_file_exists "$TMPDIR/out-go/worker/backend/worker_workflows.go"
assert_file_contains "$TMPDIR/out-go/go.mod" "module statespec-generated"
assert_file_contains "$TMPDIR/out-go/go.mod" "go 1.22"
assert_file_contains "$TMPDIR/out-go/Makefile" "GO ?= go"
assert_file_contains "$TMPDIR/out-go/Makefile" "build-api"
assert_file_contains "$TMPDIR/out-go/Makefile" "build-worker"
assert_file_contains "$TMPDIR/out-go/Makefile" "package-api"
assert_file_contains "$TMPDIR/out-go/Makefile" "package-worker"
assert_file_contains "$TMPDIR/out-go/api/backend/api_descriptors.go" "type APITierDescriptor = common.ApiDescriptor"
assert_file_contains "$TMPDIR/out-go/api/backend/api_descriptors.go" "func APITierDescriptors"
assert_file_contains "$TMPDIR/out-go/api/backend/api_handlers.go" "type APITierHandler = common.APIHandler"
assert_file_contains "$TMPDIR/out-go/api/backend/api_dispatcher.go" "func FindAPITierRoute"
assert_file_contains "$TMPDIR/out-go/api/backend/api_dispatcher.go" "func DispatchAPITierRoute"
assert_file_contains "$TMPDIR/out-go/api/backend/api_server.go" "type APITierServer struct"
assert_file_contains "$TMPDIR/out-go/api/backend/api_server.go" "func FindAPITierServer"
assert_file_contains "$TMPDIR/out-go/api/backend/api_routes.go" "func APITierRouteDescriptors"
assert_file_contains "$TMPDIR/out-go/api/backend/external_system_operator_metadata_api.go" "APITierExternalSystemOperatorMetadataHandler"
assert_file_contains "$TMPDIR/out-go/worker/backend/worker_descriptors.go" "type WorkerTierDescriptor = common.WorkerDescriptor"
assert_file_contains "$TMPDIR/out-go/worker/backend/worker_contexts.go" "func WorkerTierContexts"
assert_file_contains "$TMPDIR/out-go/worker/backend/worker_registry.go" "func FindWorkerTierDescriptor"
assert_file_contains "$TMPDIR/out-go/worker/backend/worker_application.go" "type WorkerTierApplication struct"
assert_file_contains "$TMPDIR/out-go/worker/backend/workflow_step_handlers.go" "type WorkflowStepHandler interface"
assert_file_contains "$TMPDIR/out-go/worker/backend/workflow_step_handlers.go" "func WorkflowStepHandlerKeys"
assert_file_contains "$TMPDIR/out-go/worker/backend/workflow_runner.go" "type WorkflowRunner struct"
assert_file_contains "$TMPDIR/out-go/worker/backend/workflow_runner.go" "KeepAliveStep"
assert_file_contains "$TMPDIR/out-go/worker/backend/worker_handlers.go" "type WorkerTierHandler = common.Worker"
assert_file_contains "$TMPDIR/out-go/worker/backend/worker_queues.go" "func WorkerTierQueueDefinitions"
assert_file_contains "$TMPDIR/out-go/worker/backend/worker_leases.go" "func WorkerTierLeaseDefinitions"
assert_file_contains "$TMPDIR/out-go/worker/backend/worker_workflows.go" "func WorkerTierWorkflowDefinitions"
assert_file_contains "$TMPDIR/out-go/common/backend/backend.go" "type FieldType string"
assert_file_contains "$TMPDIR/out-go/common/backend/backend.go" "FieldTypeString"
assert_file_contains "$TMPDIR/out-go/common/backend/backend.go" "Type     FieldType"
assert_file_contains "$TMPDIR/out-go/common/backend/backend.go" "TypeName string"
assert_file_contains "$TMPDIR/out-go/common/backend/workflow.go" "type KeepAliveWorkflowStepRequest struct"
assert_file_contains "$TMPDIR/out-go/common/backend/workflow.go" "KeepAliveStepTx"
assert_file_contains "$TMPDIR/out-go/common/backend/memory/backend.go" "type Backend struct"
assert_file_contains "$TMPDIR/out-go/common/backend/memory/transaction.go" "type Transaction struct"
assert_file_contains "$TMPDIR/out-go/common/backend/memory/codec.go" "featureFlagDefinitionToJSON"
assert_file_contains "$TMPDIR/out-go/common/backend/memory/feature_flags.go" "type FeatureFlagStore struct"
assert_file_contains "$TMPDIR/out-go/common/backend/memory/queues.go" "type QueueStore struct"
assert_file_contains "$TMPDIR/out-go/common/backend/memory/leases.go" "type LeaseStore struct"
assert_file_contains "$TMPDIR/out-go/common/backend/memory/workflows.go" "type WorkflowStore struct"
assert_file_contains "$TMPDIR/out-go/common/backend/memory/logs.go" "func (s *LogSink) InspectEvents"
assert_file_contains "$TMPDIR/out-go/common/backend/memory/metrics.go" "func (s *MetricSink) InspectSamples"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type FeatureFlagDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func FeatureFlagDefinitions() []FeatureFlagDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ShapeDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func ShapeDescriptors() []ShapeDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type StartOrderProcessingRequest struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "TenantId string"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ValueDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func ValueDescriptors() []ValueDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type EnumDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func EnumDescriptors() []EnumDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type EventDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func EventDescriptors() []EventDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type EventEnvelope struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func NewOrderAcceptedEvent"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ExternalSystemDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ExternalSystemMetadataDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ExternalSystemMetadataMappingDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ExternalSystemMetadataMappingPlan struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ExternalSystemOperatorMetadataUpsertRequest struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ExternalSystemOperatorMetadataRepository interface"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ExternalSystemMetadataMappingInputs struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ExternalSystemMetadataMissingMappingSource struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func MissingExternalSystemMetadataMappingSources"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func (i ExternalSystemMetadataMappingInputs) SourceValue"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ExternalSystemMetadataMappingApplicator interface"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func BuildExternalSystemMetadataMappingPlan"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "TenantField *string"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "KeyFields []string"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "TenantField: stringPtr(\"tenant_id\")"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Source: \"metadata.base_url\""
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Target: \"client.base_url\""
assert_file_contains "$TMPDIR/out-go/common/backend/external_system.go" "type ExternalSystemMetadataResolver interface"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func BuildExternalSystemMetadataLookup"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func ExternalSystemMetadataLookupByName"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func ResolveExternalSystemMetadataTx"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func ResolveExternalSystemMetadataByNameTx"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "KeyComplete()"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func ExternalSystemDescriptors() []ExternalSystemDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ApiDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func ApiDescriptors() []ApiDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ApiServerDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func ApiServerDescriptors() []ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ApiRouteDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type APIRequestContext struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type APIResponse struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type APIHandler interface"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type ExternalSystemOperatorMetadataAPIHandler interface"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func ApiRouteDescriptors() []ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Name: \"OrderApi\""
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Serves: []string{\"StartOrderProcessing\"}"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Name: \"OrderApi.StartOrderProcessing\""
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "ServerName: \"OrderApi\""
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type WorkerDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type WorkerContext struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type Worker interface"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func WorkerDescriptors() []WorkerDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func WorkerContexts() []WorkerContext"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Name: \"OrderWorker\""
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Polls: stringPtr(\"EmailDispatch.SendConfirmation\")"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type PolicyDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func PolicyDescriptors() []PolicyDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type GarbageCollectionPolicy struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type EntityStateDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type EntityOwnershipDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type EntityRelationDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type EntityChildDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type EntityInvariantDescriptor struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func EntityDescriptors() []EntityDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func CollectionDescriptors() []CollectionDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Indexes: []IndexDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func QueueDefinitions() []QueueDefinition"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func LeaseDefinitions() []LeaseDescriptor"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func WorkflowDefinitions() []WorkflowDefinition"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "EmailDispatch"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "OrderReconciler"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "WorkflowVersion: 2"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "validate_order"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "NewScheduler"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "DefaultValue: \"false\""
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "OrderAmount"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "OrderStatus"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "OrderAccepted"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Stripe"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "StartOrderProcessing"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Path: stringPtr(\"/v1/tenants/{tenantId}/orders/{order_id}/start\")"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "OrderAccess"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type LogDefinition struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func LogDefinitions() []LogDefinition"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "EventName: \"workflow.launch.decision\""
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "type MetricDefinition struct"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func MetricDefinitions() []MetricDefinition"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func EnsureSystemCollections(ctx context.Context, backend Backend) error"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func RegisterFeatureFlagDefinitionsTx"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func RegisterQueueDefinitionsTx"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func RegisterLeaseDefinitionsTx"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func RegisterRuntimeCatalogTx"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func RegisterObservabilityCatalogTx"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "func RegisterWorkflowDefinitionsTx"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "BackendName: \"workflow_launch_attempts_total\""
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Metadata: JSONObject(map[string]JSON{})"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "RelationKind: stringPtr(\"composition\")"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Name: \"valid_status\""
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Name: \"by_tenant_status\""
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Unique: true"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "GarbageCollection: &GarbageCollectionPolicy{After: \"P30D\", Mode: \"tombstone\"}"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Type: FieldTypeString"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Type: FieldTypeNamed"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Type: FieldTypeTimestamp"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "Type: FieldTypeOptional"
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "TypeName: \"OrderStatus\""
assert_file_contains "$TMPDIR/out-go/common/backend/descriptors.go" "TypeName: \"int?\""

cp "$SCRIPT_DIR/metadata_resolver_fixture_test.go" "$TMPDIR/out-go/common/backend/metadata_resolver_fixture_test.go"
cat > "$TMPDIR/out-go/common/backend/memory/memory_fixture_test.go" <<'EOF'
package memory_test

import (
	"context"
	"testing"
	"time"

	common "statespec-generated/common/backend"
	"statespec-generated/common/backend/memory"
)

func TestMemoryBackendStoresComposeInTransaction(t *testing.T) {
	ctx := context.Background()
	backend := memory.NewBackend()
	flags := memory.NewFeatureFlagStore()
	queues := memory.NewQueueStore()
	leases := memory.NewLeaseStore()
	workflows := memory.NewWorkflowStore()
	logs := memory.NewLogSink()
	metrics := memory.NewMetricSink()

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
}
EOF
(cd "$TMPDIR/out-go" && go test ./...)
