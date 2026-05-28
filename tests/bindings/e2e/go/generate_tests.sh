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
api/backend/codecs/api_codecs.go
api/backend/codecs/provision_callback_request.go
api/backend/codecs/provision_callback_response.go
api/backend/codecs/start_provision_request.go
api/backend/codecs/start_provision_response.go
api/backend/descriptors/catalog.go
api/backend/descriptors/report_provision_ready.go
api/backend/descriptors/start_provision.go
api/backend/external_system_operator_metadata_api.go
api/backend/shapes/provision_callback_request.go
api/backend/shapes/provision_callback_response.go
api/backend/shapes/start_provision_request.go
api/backend/shapes/start_provision_response.go
api/cmd/api/main.go
common/backend/backend.go
common/backend/descriptors.go
common/backend/descriptors/core.go
common/backend/descriptors/runtime.go
common/backend/descriptors/shapes.go
common/backend/descriptortypes/types.go
common/backend/entity_repository.go
common/backend/events.go
common/backend/external_system.go
common/backend/external_systems.go
common/backend/feature_flag.go
common/backend/json.go
common/backend/lease.go
common/backend/log.go
common/backend/memory/backend.go
common/backend/memory/transaction.go
common/backend/metric.go
common/backend/provision_callback_request_shape_descriptors.go
common/backend/provision_callback_response_shape_descriptors.go
common/backend/queue.go
common/backend/runtime/codec.go
common/backend/runtime/codec_leases.go
common/backend/runtime/codec_queues.go
common/backend/runtime/codec_workflows.go
common/backend/runtime/entity_gc_descriptors.go
common/backend/runtime/entity_gc_registration.go
common/backend/runtime/entity_gc_repository.go
common/backend/runtime/entity_gc_workers.go
common/backend/runtime/leases.go
common/backend/runtime/queues.go
common/backend/runtime/workflows.go
common/backend/runtime_registration.go
common/backend/runtime_registration_leases.go
common/backend/runtime_registration_queues.go
common/backend/runtime_registration_workflows.go
common/backend/schema_compatibility.go
common/backend/shape_descriptors.go
common/backend/start_provision_request_shape_descriptors.go
common/backend/start_provision_response_shape_descriptors.go
common/backend/workflow.go
common/backend/workflows/provision_service.go
common/backend/workflows/workflows.go
common/entities/service_instance/model.go
common/entities/service_instance/persistence.go
common/entities/service_instance/schema.go
go.mod
worker/backend/descriptors/catalog.go
worker/backend/descriptors/provision_worker.go
worker/backend/registry/provision_worker.go
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
worker/backend/workflows/provision_service.go
worker/backend/workflows/workflows.go
worker/cmd/worker/main.go
EOF

run_expect_status 0 "$CLI" generate bindings --lang go "$E2E_SPEC" --out "$TMPDIR/out-e2e-go"
assert_file_contains "$TMPDIR/out-e2e-go/api/backend/descriptors/catalog.go" "func ApiRouteDescriptors"
assert_file_contains "$TMPDIR/out-e2e-go/api/backend/descriptors/catalog.go" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-go/api/backend/descriptors/catalog.go" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/external_systems.go" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/external_systems.go" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "ExternalSystemOperatorMetadataAPIHandler"
cp "$SCRIPT_DIR/api_process_fixture_test.go" "$TMPDIR/out-e2e-go/api/backend/api_process_fixture_test.go"
run_expect_status 0 make -C "$TMPDIR/out-e2e-go" check-api
run_expect_status 0 env GOCACHE="$TMPDIR/go-cache" make -C "$TMPDIR/out-e2e-go" build-api

run_expect_status 0 "$CLI" generate bindings --lang go "$API_ENTITY_SPEC" --out "$TMPDIR/out-api-entities-go"
assert_file_not_exists "$TMPDIR/out-api-entities-go/worker/backend/worker_descriptors.go"
assert_file_not_exists "$TMPDIR/out-api-entities-go/worker/backend/worker_registry.go"
assert_file_not_contains "$TMPDIR/out-api-entities-go/Makefile" "check-worker"
assert_file_not_contains "$TMPDIR/out-api-entities-go/Makefile" "build-worker"
assert_file_not_exists "$TMPDIR/out-api-entities-go/common/backend/account_descriptors.go"
assert_file_not_exists "$TMPDIR/out-api-entities-go/common/backend/project_descriptors.go"
assert_file_not_exists "$TMPDIR/out-api-entities-go/common/backend/task_descriptors.go"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "repository := account.DefaultAccountRepository{}"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_project.go" "repository := project.DefaultProjectRepository{}"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_task.go" "repository := task.DefaultTaskRepository{}"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "handler.Backend.Begin(ctx)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "repository.CreateTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "account.AccountFieldCreatedAt"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "account.AccountFieldUpdatedAt"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "account.AccountFieldStatus"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "document[account.AccountFieldCreatedAt] = common.JSONString(time.Now().UTC().Format(time.RFC3339))"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "document[account.AccountFieldStatus] = common.JSONString(account.AccountStatusActive)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_project.go" "missing required parent Account"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_task.go" "missing required parent Project"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "extractAPIPathParameters"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "repository.GetTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "repository.ListByTenantAccountTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "repository.ListByAccountStatusTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_project.go" "repository.ListByProjectStatusTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/project/persistence.go" "ListByTenantProjectTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/task/persistence.go" "ListByTenantTaskTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/task/persistence.go" "ListByAccountPriorityTx"
assert_file_not_exists "$TMPDIR/out-api-entities-go/common/backend/descriptors/entities/account.go"
assert_file_not_exists "$TMPDIR/out-api-entities-go/common/backend/descriptors/entities/project.go"
assert_file_not_exists "$TMPDIR/out-api-entities-go/common/backend/descriptors/entities/task.go"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/model.go" "AccountEntityName = \"Account\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/model.go" "AccountFieldTenantId = \"tenant_id\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/model.go" "AccountFieldCreatedAtTypeName = \"timestamp\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/model.go" "AccountFieldStatusTypeName = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/model.go" "AccountIndexByTenantAccount = \"by_tenant_account\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/model.go" "{Name: AccountFieldCreatedAt, Type: backend.FieldTypeTimestamp, TypeName: AccountFieldCreatedAtTypeName, Required: true}"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/model.go" "{Name: AccountFieldStatus, Type: backend.FieldTypeString, TypeName: AccountFieldStatusTypeName, Required: true}"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/create_account_request_shape_descriptors.go" "CreateAccountRequestShapeName = \"CreateAccountRequest\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/create_account_request_shape_descriptors.go" "CreateAccountRequestFieldDisplayName = \"display_name\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/create_account_request_shape_descriptors.go" "CreateAccountRequestFieldDisplayNameTypeName = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/create_account_request_shape_descriptors.go" "{Name: CreateAccountRequestFieldDisplayName, Type: FieldTypeString, TypeName: CreateAccountRequestFieldDisplayNameTypeName, Required: true}"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/schema.go" "KeyFields: []string{AccountFieldTenantId, AccountFieldAccountId}"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "builder.WriteString(value.CanonicalString())"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "object[keyValue.Field] = keyValue.Value"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "StatusCode: 404"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_account.go" "common.JSONArray(items)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_project.go" "repository.UpdateTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_project.go" "invalid entity status transition"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_project.go" "updatedDocument[project.ProjectFieldUpdatedAt]"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_project.go" "requestedStatus := project.ProjectStatusDeleted"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_project.go" "invalid entity delete transition"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry_project.go" "StatusCode: 204"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/project/persistence.go" "UpdateTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/project/model.go" "ProjectStatusDeleted"
cp "$SCRIPT_DIR/api_persistence_fixture_test.go" "$TMPDIR/out-api-entities-go/api/backend/api_persistence_fixture_test.go"
run_expect_status 0 make -C "$TMPDIR/out-api-entities-go" check-api
run_expect_status 0 env GOCACHE="$TMPDIR/go-cache" make -C "$TMPDIR/out-api-entities-go" build-api

run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang go "$APP_SPEC" --out "$TMPDIR/out-app-go"
assert_file_manifest_equals "$TMPDIR/out-app-go" "$APP_MANIFEST"
assert_file_exists "$TMPDIR/out-app-go/common/backend/descriptors.go"
assert_file_exists "$TMPDIR/out-app-go/api/backend/api_descriptors.go"
assert_file_exists "$TMPDIR/out-app-go/worker/backend/worker_descriptors.go"
assert_tree_files_not_contains "$TMPDIR/out-app-go/api/backend/descriptors" "*.go" "\"statespec-generated/common/backend\""
assert_tree_files_not_contains "$TMPDIR/out-app-go/worker/backend/descriptors" "*.go" "\"statespec-generated/common/backend\""
assert_file_contains "$TMPDIR/out-app-go/api/backend/descriptors/start_provision.go" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/descriptors/provision_worker.go" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/descriptors/provision_worker.go" "\"ProvisionWorker\""
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
assert_file_contains "$TMPDIR/out-app-go/api/backend/codecs/start_provision_request.go" "func DecodeStartProvisionRequest"
assert_file_contains "$TMPDIR/out-app-go/api/backend/codecs/start_provision_response.go" "func EncodeStartProvisionResponse"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_handler_registry.go" "type DefaultAPITierHandler struct"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_handlers.go" "type BusinessAPITierHandler interface"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_handler_registry.go" "BusinessHandler BusinessAPITierHandler"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_handler_registry.go" "handler.BusinessHandler.HandleStartProvision"
assert_file_not_exists "$TMPDIR/out-app-go/api/backend/api_handler_registry_operations.go"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "signal.NotifyContext"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "api.NewAPIProcess"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "runtime.RegisterEntityGCWorkers"
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
assert_file_contains "$TMPDIR/out-app-go/worker/cmd/worker/main.go" "runtimegc.RegisterEntityGCWorkers"
assert_file_contains "$TMPDIR/out-app-go/worker/cmd/worker/main.go" "process.Start(ctx)"
assert_file_contains "$TMPDIR/out-app-go/worker/cmd/worker/main.go" "process.Join()"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_runner.go" "KeepAliveStep"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "type DefaultWorkflowStepHandler struct"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "\"ProvisionService.wait_for_remote_service\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "HandleProvisionServiceValidateRequest"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_runner.go" "workflowmodules.DispatchProvisionServiceStep"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflows/provision_service.go" "HandleProvisionServiceValidateRequest"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflows/provision_service.go" "nextStep := \"create_remote_service\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflows/provision_service.go" "nextStep := \"wait_for_remote_service\""
cp "$SCRIPT_DIR/api_linking_fixture_test.go" "$TMPDIR/out-app-go/api/backend/api_linking_fixture_test.go"
cp "$SCRIPT_DIR/registration_restart_fixture_test.go" "$TMPDIR/out-app-go/common/backend/registration_restart_fixture_test.go"
cp "$SCRIPT_DIR/worker_linking_fixture_test.go" "$TMPDIR/out-app-go/worker/backend/worker_linking_fixture_test.go"
cp "$SCRIPT_DIR/worker_process_fixture_test.go" "$TMPDIR/out-app-go/worker/backend/worker_process_fixture_test.go"
run_expect_status 0 make -C "$TMPDIR/out-app-go" check
run_expect_status 0 env GOCACHE="$TMPDIR/go-cache" make -C "$TMPDIR/out-app-go" build
