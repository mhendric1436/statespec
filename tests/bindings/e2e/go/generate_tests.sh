#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../../../.." && pwd)"
TESTS_DIR="$REPO_DIR/tests"
. "$TESTS_DIR/cli/common.sh"

APP_SPEC="$REPO_DIR/testdata/generators/canonical-api-worker-app.sspec"
E2E_SPEC="$REPO_DIR/testdata/generators/external-system-metadata-e2e.sspec"
API_ENTITY_SPEC="$REPO_DIR/examples/api-entities-only.sspec"
APP_MANIFEST="$TMPDIR/expected-go-manifest.txt"

cat > "$APP_MANIFEST" <<'EOF'
Makefile
api/backend/api_application.go
api/backend/api_codecs.go
api/backend/api_descriptors.go
api/backend/api_dispatcher.go
api/backend/api_handler_registry.go
api/backend/api_handlers.go
api/backend/api_process.go
api/backend/api_routes.go
api/backend/api_server.go
api/backend/api_transport.go
api/backend/external_system_operator_metadata_api.go
api/cmd/api/main.go
common/backend/backend.go
common/backend/descriptors.go
common/backend/descriptors/apis.go
common/backend/descriptors/core.go
common/backend/descriptors/entities/service_instance.go
common/backend/descriptors/runtime.go
common/backend/descriptors/shapes.go
common/backend/descriptors/workers.go
common/backend/external_system.go
common/backend/feature_flag.go
common/backend/json.go
common/backend/lease.go
common/backend/log.go
common/backend/memory/backend.go
common/backend/memory/transaction.go
common/backend/metric.go
common/backend/queue.go
common/backend/runtime/codec.go
common/backend/runtime/codec_leases.go
common/backend/runtime/codec_queues.go
common/backend/runtime/codec_workflows.go
common/backend/runtime/entity_gc_descriptors.go
common/backend/runtime/entity_gc_repository.go
common/backend/runtime/entity_gc_workers.go
common/backend/runtime/leases.go
common/backend/runtime/queues.go
common/backend/runtime/workflows.go
common/backend/schema_compatibility.go
common/backend/workflow.go
go.mod
worker/backend/worker_application.go
worker/backend/worker_contexts.go
worker/backend/worker_descriptors.go
worker/backend/worker_leases.go
worker/backend/worker_process.go
worker/backend/worker_queues.go
worker/backend/worker_registry.go
worker/backend/worker_runtime.go
worker/backend/worker_workflows.go
worker/backend/workflow_runner.go
worker/backend/workflow_step_handlers.go
worker/cmd/worker/main.go
EOF

run_expect_status 0 "$CLI" generate bindings --lang go "$E2E_SPEC" --out "$TMPDIR/out-e2e-go"
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "func ApiRouteDescriptors"
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "ExternalSystemOperatorMetadataAPIHandler"
cp "$SCRIPT_DIR/api_process_fixture_test.go" "$TMPDIR/out-e2e-go/api/backend/api_process_fixture_test.go"
run_expect_status 0 make -C "$TMPDIR/out-e2e-go" check-api

run_expect_status 0 "$CLI" generate bindings --lang go "$API_ENTITY_SPEC" --out "$TMPDIR/out-api-entities-go"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "repository := common.DefaultAccountRepository{}"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "repository := common.DefaultProjectRepository{}"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "repository := common.DefaultTaskRepository{}"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "handler.Backend.Begin(ctx)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "repository.CreateTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "common.AccountFieldCreatedAt"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "common.AccountFieldUpdatedAt"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "common.AccountFieldStatus"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "document[common.AccountFieldCreatedAt] = common.JSONString(time.Now().UTC().Format(time.RFC3339))"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "document[common.AccountFieldStatus] = common.JSONString(common.AccountStatusActive)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "missing required parent Account"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "missing required parent Project"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "extractAPIPathParameters"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "repository.GetTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "repository.ListByTenantAccountTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "repository.ListByAccountStatusTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "repository.ListByProjectStatusTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "ListByTenantProjectTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "ListByTenantTaskTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "ListByAccountPriorityTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "AccountEntityName = \"Account\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "AccountFieldTenantId = \"tenant_id\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "AccountFieldCreatedAtTypeName = \"timestamp\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "AccountFieldStatusTypeName = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "AccountIndexByTenantAccount = \"by_tenant_account\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "{Name: AccountFieldCreatedAt, Type: FieldTypeTimestamp, TypeName: AccountFieldCreatedAtTypeName, Required: true}"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "{Name: AccountFieldStatus, Type: FieldTypeString, TypeName: AccountFieldStatusTypeName, Required: true}"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "CreateAccountRequestShapeName = \"CreateAccountRequest\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "CreateAccountRequestFieldTenantId = \"tenant_id\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "CreateAccountRequestFieldTenantIdTypeName = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "{Name: CreateAccountRequestFieldTenantId, Type: FieldTypeString, TypeName: CreateAccountRequestFieldTenantIdTypeName, Required: true}"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "KeyFields: []string{AccountFieldTenantId, AccountFieldAccountId}"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "builder.WriteString(value.CanonicalString())"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "object[keyValue.Field] = keyValue.Value"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "StatusCode: 404"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "common.JSONArray(items)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "repository.UpdateTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "invalid entity status transition"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "updatedDocument[common.ProjectFieldUpdatedAt]"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "requestedStatus := common.ProjectStatusDeleted"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "invalid entity delete transition"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "StatusCode: 204"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "UpdateTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "ProjectStatusDeleted"
cp "$SCRIPT_DIR/api_persistence_fixture_test.go" "$TMPDIR/out-api-entities-go/api/backend/api_persistence_fixture_test.go"
run_expect_status 0 make -C "$TMPDIR/out-api-entities-go" check-api

run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang go "$APP_SPEC" --out "$TMPDIR/out-app-go"
assert_file_manifest_equals "$TMPDIR/out-app-go" "$APP_MANIFEST"
assert_file_contains "$TMPDIR/out-app-go/common/backend/descriptors.go" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-go/common/backend/descriptors.go" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-go/common/backend/descriptors.go" "\"ProvisionWorker\""
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_application.go" "type APITierApplication struct"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_process.go" "type APIProcess struct"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_process.go" "func (process *APIProcess) RequestStop()"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_process.go" "func (process *APIProcess) Start(ctx context.Context) error"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_process.go" "func (process *APIProcess) Join() error"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_process.go" "func (process *APIProcess) Running() bool"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_process.go" "EntityGCEnabled   bool"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_process.go" "func (process *APIProcess) AddEntityGCWorker"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_process.go" "startEntityGCWorkersLocked"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_transport.go" "type APITierTransport interface"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_transport.go" "return server.Handle(ctx, routeName, request)"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_transport.go" "type LocalBlockingAPITierTransport struct"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_transport.go" "RequestStop()"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_transport.go" "func (transport *LocalBlockingAPITierTransport) RequestStop()"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_process.go" "process.Transport.RequestStop()"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "api.NewLocalBlockingAPITierTransport()"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_codecs.go" "func DecodeStartProvisionRequest"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_codecs.go" "func EncodeStartProvisionResponse"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_handler_registry.go" "type DefaultAPITierHandler struct"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "signal.NotifyContext"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "api.NewAPIProcess"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "process.Start(ctx)"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "process.Join()"
assert_file_not_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "NewDefaultAPITierApplication"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_server.go" "type APITierServer struct"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "func DispatchAPITierRoute"
assert_file_not_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "func DispatchAPITierOperationRoute"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/worker_registry.go" "func FindWorkerTierDescriptor"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/worker_runtime.go" "type WorkerTierRuntime struct"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/worker_runtime.go" "type WorkerTierRuntimeConfig struct"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/worker_runtime.go" "func (runtime *WorkerTierRuntime) StartEntityGCWorkers"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/worker_runtime.go" "func (runtime *WorkerTierRuntime) RequestStop()"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/worker_process.go" "type WorkerProcess struct"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/worker_process.go" "func (process *WorkerProcess) Start(ctx context.Context) error"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/worker_process.go" "func (process *WorkerProcess) Join() error"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/worker_process.go" "func (process *WorkerProcess) Running() bool"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/worker_process.go" "func (process *WorkerProcess) RequestStop()"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/worker_process.go" "func (process *WorkerProcess) startWorkerLoops"
assert_file_contains "$TMPDIR/out-app-go/worker/cmd/worker/main.go" "signal.NotifyContext"
assert_file_contains "$TMPDIR/out-app-go/worker/cmd/worker/main.go" "worker.NewWorkerProcess"
assert_file_contains "$TMPDIR/out-app-go/worker/cmd/worker/main.go" "worker.DefaultWorkflowStepHandler"
assert_file_contains "$TMPDIR/out-app-go/worker/cmd/worker/main.go" "process.Start(ctx)"
assert_file_contains "$TMPDIR/out-app-go/worker/cmd/worker/main.go" "process.Join()"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_runner.go" "KeepAliveStep"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "type DefaultWorkflowStepHandler struct"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "\"ProvisionService.wait_for_remote_service\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "HandleProvisionServiceValidateRequest"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_runner.go" "HandleProvisionServiceValidateRequest"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_runner.go" "nextStepValue := \"create_remote_service\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_runner.go" "nextStepValue := \"wait_for_remote_service\""
cp "$SCRIPT_DIR/api_linking_fixture_test.go" "$TMPDIR/out-app-go/api/backend/api_linking_fixture_test.go"
cp "$SCRIPT_DIR/registration_restart_fixture_test.go" "$TMPDIR/out-app-go/common/backend/registration_restart_fixture_test.go"
cp "$SCRIPT_DIR/worker_linking_fixture_test.go" "$TMPDIR/out-app-go/worker/backend/worker_linking_fixture_test.go"
cp "$SCRIPT_DIR/worker_process_fixture_test.go" "$TMPDIR/out-app-go/worker/backend/worker_process_fixture_test.go"
run_expect_status 0 make -C "$TMPDIR/out-app-go" check
