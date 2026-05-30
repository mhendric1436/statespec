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
WORKFLOW_ENTITY_SPEC="$REPO_DIR/examples/workflow-entities-only.sspec"
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
api/backend/codecs/provision_callback_request.go
api/backend/codecs/provision_callback_response.go
api/backend/codecs/start_provision_request.go
api/backend/codecs/start_provision_response.go
api/backend/codecsupport/api_codecs.go
api/backend/descriptors/catalog.go
api/backend/descriptors/report_provision_ready.go
api/backend/descriptors/start_provision.go
api/backend/entity_gc_catalog.go
api/backend/external_system_operator_metadata_api.go
api/backend/servers/provision_api/catalog.go
api/backend/servers/provision_api/constants.go
api/backend/shapes/catalog.go
api/backend/shapes/provision_callback_request.go
api/backend/shapes/provision_callback_response.go
api/backend/shapes/start_provision_request.go
api/backend/shapes/start_provision_response.go
api/cmd/api/main.go
common/backend/backend.go
common/backend/descriptors.go
common/backend/descriptors/core.go
common/backend/descriptors/runtime.go
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
common/backend/policy_descriptors.go
common/backend/queue.go
common/backend/runtime/codec.go
common/backend/runtime/codec_leases.go
common/backend/runtime/codec_queues.go
common/backend/runtime/codec_workflows.go
common/backend/runtime/entity_gc_registration.go
common/backend/runtime/entity_gc_repository.go
common/backend/runtime/entity_gc_types.go
common/backend/runtime/entity_gc_workers.go
common/backend/runtime/entitygc/types.go
common/backend/runtime/leases.go
common/backend/runtime/queues.go
common/backend/runtime/workflows.go
common/backend/runtime_definitions.go
common/backend/runtime_registration.go
common/backend/runtime_registration_leases.go
common/backend/runtime_registration_queues.go
common/backend/runtime_registration_workflows.go
common/backend/schema_compatibility.go
common/backend/shape_types.go
common/backend/values_enums_descriptors.go
common/backend/workflow.go
common/backend/workflows/provision_service.go
common/backend/workflows/workflows.go
common/entities/service_instance/constants.go
common/entities/service_instance/gc.go
common/entities/service_instance/model.go
common/entities/service_instance/persistence.go
common/entities/service_instance/schema.go
go.mod
worker/backend/descriptors/catalog.go
worker/backend/descriptors/provision_worker.go
worker/backend/entity_gc_catalog.go
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
worker/backend/workflows/context/context.go
worker/backend/workflows/provision_service.go
worker/backend/workflows/provision_service/handlers.go
worker/backend/workflows/workflows.go
worker/cmd/worker/main.go
EOF

run_expect_status 0 "$CLI" generate bindings --lang go "$E2E_SPEC" --out "$TMPDIR/out-e2e-go"
assert_file_contains "$TMPDIR/out-e2e-go/api/backend/descriptors/catalog.go" "func ApiRouteDescriptors"
assert_file_contains "$TMPDIR/out-e2e-go/api/backend/servers/operator_api/catalog.go" "Name: OperatorApiServerName"
assert_file_contains "$TMPDIR/out-e2e-go/api/backend/servers/operator_api/catalog.go" "\"UpsertExternalSystemEndpoint\""
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
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/account/registry.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/project/registry.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/task/handlers.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/task/registry.go"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "repository := account.DefaultAccountRepository{}"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go" "repository := project.DefaultProjectRepository{}"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/task/handlers.go" "repository := task.DefaultTaskRepository{}"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "handler.Backend.Begin(ctx)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "repository.CreateTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "account.AccountFieldCreatedAt"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "account.AccountFieldUpdatedAt"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "account.AccountFieldStatus"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "createTimestamp := time.Now().UTC().Format(time.RFC3339)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "document[account.AccountFieldCreatedAt] = common.JSONString(createTimestamp)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "document[account.AccountFieldUpdatedAt] = common.JSONString(createTimestamp)"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "document[account.AccountFieldCreatedAt] = common.JSONString(time.Now().UTC().Format(time.RFC3339))"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "document[account.AccountFieldStatus] = common.JSONString(account.AccountStatusActive)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "record.Document.Find(account.AccountFieldDisplayName)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "body[account.AccountFieldTenantId] = pathParameterJSON(pathParameters, account.AccountFieldTenantId)"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "body[\"tenant_id\"]"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go" "missing required parent Account"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/task/handlers.go" "missing required parent Project"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "extractAPIPathParameters"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "repository.GetTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "repository.ListByTenantAccountTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go" "repository.ListByAccountStatusTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/task/handlers.go" "repository.ListByProjectStatusTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/project/persistence.go" "ListByTenantProjectTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/task/persistence.go" "ListByTenantTaskTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/task/persistence.go" "ListByAccountPriorityTx"
assert_file_not_exists "$TMPDIR/out-api-entities-go/common/backend/descriptors/entities/account.go"
assert_file_not_exists "$TMPDIR/out-api-entities-go/common/backend/descriptors/entities/project.go"
assert_file_not_exists "$TMPDIR/out-api-entities-go/common/backend/descriptors/entities/task.go"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/constants.go" "AccountEntityName = \"Account\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/constants.go" "AccountFieldTenantId = \"tenant_id\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/constants.go" "AccountFieldCreatedAtTypeName = \"timestamp\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/constants.go" "AccountFieldStatusTypeName = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/constants.go" "AccountIndexByTenantAccount = \"by_tenant_account\""
assert_file_not_contains "$TMPDIR/out-api-entities-go/common/entities/account/constants.go" "CreateAccountRequest"
assert_file_not_contains "$TMPDIR/out-api-entities-go/common/entities/account/constants.go" "AccountResponse"
assert_file_not_contains "$TMPDIR/out-api-entities-go/common/entities/account/constants.go" "DecodeCreateAccountRequest"
assert_file_not_contains "$TMPDIR/out-api-entities-go/common/entities/account/model.go" "AccountEntityName = \"Account\""
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/model.go" "{Name: AccountFieldCreatedAt, Type: backend.FieldTypeTimestamp, TypeName: AccountFieldCreatedAtTypeName, Required: true}"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/model.go" "{Name: AccountFieldStatus, Type: backend.FieldTypeString, TypeName: AccountFieldStatusTypeName, Required: true}"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/account/constants.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/project/constants.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/task/constants.go"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/constants.go" "CreateAccountAPIName = \"CreateAccount\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/constants.go" "EntityApiCreateAccountRouteName = \"EntityApi.CreateAccount\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/constants.go" "CreateAccountRequestAPIShapeName = \"CreateAccountRequest\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/constants.go" "AccountListResponseEnvelopeName = entityconstants.AccountEntityPluralName"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/constants.go" "ListAccountProjectsAPIName = \"ListAccountProjects\""
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/constants.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/catalog.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/entities/account/catalog.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/entities/project/catalog.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/entities/task/catalog.go"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/constants.go" "EntityApiServerName = \"EntityApi\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/constants.go" "EntityApiServerConcurrency = 1"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/constants.go" "CreateAccount"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/entities/account/catalog.go" "account.EntityAPINames()"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/entities/account/catalog.go" "case account.CreateAccountAPIName"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/catalog.go" "Name: EntityApiServerName"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/entities/account/catalog.go" "func AppendAPIServerNames"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/catalog.go" "serveraccount.AppendAPIServerNames(&serves)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/catalog.go" "serverproject.AppendAPIServerNames(&serves)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/catalog.go" "servertask.AppendAPIServerNames(&serves)"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/servers/entity_api/catalog.go" "func AppendAPIServerNames"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/account/shapes.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/account/catalog.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/project/shapes.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/project/codecs.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/project/catalog.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/task/shapes.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/task/codecs.go"
assert_file_exists "$TMPDIR/out-api-entities-go/api/backend/entities/task/catalog.go"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/shapes.go" "type CreateAccountRequest struct"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/shapes.go" "Name: entityconstants.AccountEntityPluralName"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/shapes.go" "ListAccountsResponseFieldAccounts = \"accounts\""
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/shapes.go" "\"accounts\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "entityconstants \"statespec-generated/common/entities/account\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/codecs.go" "entityconstants \"statespec-generated/common/entities/project\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/task/codecs.go" "entityconstants \"statespec-generated/common/entities/task\""
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "entityconstants.AccountEntityName"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "func DecodeCreateAccountRequest"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "codecsupport.RequireMember(request.Body, entityconstants.AccountFieldDisplayName)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "body[entityconstants.AccountFieldTenantId]"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "body[entityconstants.AccountFieldAccountId]"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "codecsupport.RequireMember(request.Body, entityconstants.AccountFieldStatus)"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/constants.go" "AccountEntityPluralName = \"accounts\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "body[entityconstants.AccountEntityPluralName]"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/codecs.go" "codecsupport.RequireMember(request.Body, entityconstants.ProjectFieldAccountId)"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/project/constants.go" "ProjectEntityPluralName = \"projects\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/codecs.go" "body[entityconstants.ProjectFieldProjectId]"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/task/codecs.go" "codecsupport.RequireMember(request.Body, entityconstants.TaskFieldProjectId)"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/task/constants.go" "TaskEntityPluralName = \"tasks\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/task/codecs.go" "codecsupport.RequireMember(request.Body, entityconstants.TaskFieldPriority)"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "codecsupport.RequireMember(request.Body, \"display_name\")"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "body[\"tenant_id\"]"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "\"account_id\""
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "\"accounts\""
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/codecs.go" "\"account_id\""
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/codecs.go" "\"projects\""
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/task/codecs.go" "\"project_id\""
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/task/codecs.go" "\"tasks\""
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "common/backend/descriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "common/entities/account/model"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/codecs.go" "common/backend/descriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/codecs.go" "common/entities/project/model"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/task/codecs.go" "common/backend/descriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/task/codecs.go" "common/entities/task/model"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/shapes.go" "type AccountResponse struct"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/catalog.go" "func EntityShapeDescriptors"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/catalog.go" "func EntityAPIDescriptors"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/catalog.go" "func EntityAPINames"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/catalog.go" "CreateAccountAPIName"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/catalog.go" "func EntityAPIRouteDescriptors"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/catalog.go" "func HandlerEntrypoints"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/catalog.go" "common/backend/descriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/catalog.go" "common/entities/account/model"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/catalog.go" "CreateAccountApiDescriptors()"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/catalog.go" "CreateAccountApiRouteDescriptors()"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/catalog.go" "HandleListAccountProjects"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/catalog.go" "HandleListAccountProjects"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/shapes/entities.go" "type CreateAccountRequest = account.CreateAccountRequest"
assert_file_not_exists "$TMPDIR/out-api-entities-go/api/backend/shapes/create_account_request.go"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/shapes/catalog.go" "account.EntityShapeDescriptors()"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/descriptors/catalog.go" "account.EntityAPIDescriptors()"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/descriptors/catalog.go" "project.EntityAPIRouteDescriptors()"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/descriptors/catalog.go" "\"CreateAccount\""
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/descriptors/catalog.go" "EntityAPINames()"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_descriptors.go" "entityApiserver.ApiServerDescriptors()"
assert_file_not_exists "$TMPDIR/out-api-entities-go/api/backend/descriptors/create_account.go"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/create_account_descriptor.go" "accountconstants \"statespec-generated/common/entities/account\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/create_account_descriptor.go" "Path: createAccountStringPtr(\"/v1/tenants/{\" + accountconstants.AccountFieldTenantId"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/create_account_descriptor.go" "common/backend/descriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/create_account_descriptor.go" "common/entities/account/model"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/create_project_descriptor.go" "projectconstants \"statespec-generated/common/entities/project\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/create_project_descriptor.go" "projectconstants.ProjectFieldProjectId"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/task/create_task_descriptor.go" "taskconstants \"statespec-generated/common/entities/task\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/task/create_task_descriptor.go" "taskconstants.TaskFieldTaskId"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-go/api/backend/descriptors" "*.go" "common/backend/descriptors"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-go/api/backend/descriptors" "*.go" "common/entities/account/model"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-go/api/backend/descriptors" "*.go" "common/entities/project/model"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-go/api/backend/descriptors" "*.go" "common/entities/task/model"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities" "catalog.go" "common/backend/descriptors"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities" "catalog.go" "common/entities/account/model"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities" "catalog.go" "common/entities/project/model"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities" "catalog.go" "common/entities/task/model"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/descriptors/catalog.go" "CreateAccountApiDescriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/descriptors/catalog.go" "CreateProjectApiDescriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/descriptors/catalog.go" "CreateTaskApiDescriptors"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "account.RegisterHandlerInvokers(registrar)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "project.RegisterHandlerInvokers(registrar)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "task.RegisterHandlerInvokers(registrar)"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "func (handler DefaultAPITierHandler) HandleCreateAccount"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "func (handler DefaultAPITierHandler) HandleGetAccount"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "func (handler DefaultAPITierHandler) HandleListAccounts"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "func (handler DefaultAPITierHandler) HandleUpdateAccountStatus"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "func (handler DefaultAPITierHandler) HandleDeleteAccount"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/api_handler_registry.go" "NewDefaultAccountAPIHandlerRegistry"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/registry.go" "func RegisterHandlerInvokers"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/registry.go" "registrar.RegisterHandlerInvoker(\"CreateAccount\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/registry.go" "registrar.RegisterHandlerInvoker(\"GetAccount\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/registry.go" "registrar.RegisterHandlerInvoker(\"ListAccounts\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/registry.go" "registrar.RegisterHandlerInvoker(\"UpdateAccountStatus\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/registry.go" "registrar.RegisterHandlerInvoker(\"DeleteAccount\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/registry.go" "handler := NewDefaultAccountAPIHandlerRegistry(backend)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/shapes.go" "CreateAccountRequestShapeName = \"CreateAccountRequest\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/shapes.go" "AccountResponseShapeName = \"AccountResponse\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/codecs.go" "func DecodeCreateAccountRequest"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/shapes.go" "entityconstants \"statespec-generated/common/entities/account\""
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/shapes.go" "CreateAccountRequestFieldDisplayName = \"display_name\""
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/shapes.go" "CreateAccountRequestFieldDisplayNameTypeName = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/shapes.go" "{Name: entityconstants.AccountFieldDisplayName, Type: common.FieldTypeString, TypeName: entityconstants.AccountFieldDisplayNameTypeName, Required: true}"
assert_file_not_exists "$TMPDIR/out-api-entities-go/common/backend/create_account_request_shape_descriptors.go"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/account/schema.go" "KeyFields: []string{AccountFieldTenantId, AccountFieldAccountId}"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "builder.WriteString(value.CanonicalString())"
assert_file_contains "$TMPDIR/out-api-entities-go/common/backend/descriptors.go" "object[keyValue.Field] = keyValue.Value"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "StatusCode: 404"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/account/handlers.go" "common.JSONArray(items)"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go" "repository.UpdateTx"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go" "invalid entity status transition"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go" "updatedDocument[project.ProjectFieldUpdatedAt]"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go" "requestedStatus := project.ProjectStatusDeleted"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go" "invalid entity delete transition"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go" "func projectStatusTransitionAllowed(currentStatus string, requestedStatus string) bool"
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go" "projectStatusTransitionAllowed(currentStatus, requestedStatus)"
assert_file_not_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go" "transitionAllowed :="
assert_file_contains "$TMPDIR/out-api-entities-go/api/backend/entities/project/handlers.go" "StatusCode: 204"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/project/persistence.go" "UpdateTx"
assert_file_contains "$TMPDIR/out-api-entities-go/common/entities/project/constants.go" "ProjectStatusDeleted"
assert_file_contains "$TMPDIR/out-api-entities-go/api/cmd/api/main.go" "runtime \"statespec-generated/common/backend/runtime\""
assert_file_contains "$TMPDIR/out-api-entities-go/api/cmd/api/main.go" "runtime.RegisterEntityGCWorkers"
assert_file_contains "$TMPDIR/out-api-entities-go/api/cmd/api/main.go" "api.APIEntityGCDescriptors()"
assert_file_contains "$TMPDIR/out-api-entities-go/api/cmd/api/main.go" "process.AddEntityGCWorker"
cp "$SCRIPT_DIR/api_persistence_fixture_test.go" "$TMPDIR/out-api-entities-go/api/backend/api_persistence_fixture_test.go"
run_expect_status 0 make -C "$TMPDIR/out-api-entities-go" check-api
run_expect_status 0 env GOCACHE="$TMPDIR/go-cache" make -C "$TMPDIR/out-api-entities-go" build-api

run_expect_status 0 "$CLI" generate bindings --lang go "$WORKFLOW_ENTITY_SPEC" --out "$TMPDIR/out-workflow-entities-go"
assert_file_not_exists "$TMPDIR/out-workflow-entities-go/api/backend/shapes/catalog.go"
assert_file_not_exists "$TMPDIR/out-workflow-entities-go/api/backend/api_handler_registry.go"
assert_file_not_exists "$TMPDIR/out-workflow-entities-go/api/backend/api_codecs.go"
assert_file_not_exists "$TMPDIR/out-workflow-entities-go/api/backend/api_descriptors.go"
assert_file_not_exists "$TMPDIR/out-workflow-entities-go/api/backend/codecsupport/api_codecs.go"
assert_file_not_exists "$TMPDIR/out-workflow-entities-go/api/backend/external_system_operator_metadata_api.go"
assert_file_not_exists "$TMPDIR/out-workflow-entities-go/api/backend/descriptors/catalog.go"
assert_file_not_exists "$TMPDIR/out-workflow-entities-go/api/backend/api_handlers.go"
assert_file_not_exists "$TMPDIR/out-workflow-entities-go/api/cmd/api/main.go"
assert_file_not_contains "$TMPDIR/out-workflow-entities-go/Makefile" "check-api"
assert_file_contains "$TMPDIR/out-workflow-entities-go/worker/cmd/worker/main.go" "runtimegc \"statespec-generated/common/backend/runtime\""
assert_file_contains "$TMPDIR/out-workflow-entities-go/worker/cmd/worker/main.go" "runtimegc.RegisterEntityGCWorkers"
assert_file_contains "$TMPDIR/out-workflow-entities-go/worker/cmd/worker/main.go" "worker.WorkerEntityGCDescriptors()"
assert_file_contains "$TMPDIR/out-workflow-entities-go/worker/cmd/worker/main.go" "runtime.AddEntityGCWorker"
run_expect_status 0 env GOCACHE="$TMPDIR/go-cache" make -C "$TMPDIR/out-workflow-entities-go" check-worker

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
assert_file_contains "$TMPDIR/out-app-go/common/entities/service_instance/gc.go" "func ServiceInstanceEntityGCDescriptor() entitygc.EntityGCDescriptor"
assert_file_contains "$TMPDIR/out-app-go/common/backend/runtime/entity_gc_types.go" "type EntityGCDescriptor = entitygc.EntityGCDescriptor"
assert_file_not_contains "$TMPDIR/out-app-go/common/backend/runtime/entity_gc_types.go" "ServiceInstanceEntityGCDescriptor"
assert_file_contains "$TMPDIR/out-app-go/common/backend/runtime/entity_gc_registration.go" "for _, descriptor := range descriptors"
assert_file_contains "$TMPDIR/out-app-go/api/backend/codecs/start_provision_request.go" "func DecodeStartProvisionRequest"
assert_file_contains "$TMPDIR/out-app-go/api/backend/codecs/start_provision_response.go" "func EncodeStartProvisionResponse"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_handler_registry.go" "type DefaultAPITierHandler struct"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_handlers.go" "type BusinessAPITierHandler interface"
assert_file_not_contains "$TMPDIR/out-app-go/api/backend/api_handlers.go" "type APITierHandler interface"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_handler_registry.go" "BusinessHandler BusinessAPITierHandler"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_handler_registry.go" "func RegisterHandlerInvokers"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_handler_registry.go" "func RegisterBusinessHandlerInvokers"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_handler_registry.go" "handler.HandleStartProvision"
assert_file_not_contains "$TMPDIR/out-app-go/api/backend/api_handler_registry.go" "func (handler DefaultAPITierHandler) HandleStartProvision"
assert_file_not_contains "$TMPDIR/out-app-go/api/backend/api_handler_registry.go" "NewDefaultServiceInstanceAPIHandlerRegistry"
assert_file_not_exists "$TMPDIR/out-app-go/api/backend/entities/operations/handlers.go"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "signal.NotifyContext"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "api.NewAPIProcess"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "runtime.RegisterEntityGCWorkers"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "api.APIEntityGCDescriptors()"
assert_file_contains "$TMPDIR/out-app-go/api/backend/entity_gc_catalog.go" "ServiceInstanceEntityGCDescriptor()"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "process.Start(ctx)"
assert_file_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "process.Join()"
assert_file_not_contains "$TMPDIR/out-app-go/api/cmd/api/main.go" "NewDefaultAPITierApplication"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_server.go" "type APITierServer struct"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "func DispatchAPITierRoute"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "func APITierRouteLookupMap"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "func APITierHandlerLookupMap"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "type APIHandlerInvoker func(context.Context, common.Backend"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "type BusinessAPIHandlerInvoker"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "var apiRouteDescriptorMap"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "var apiHandlerMap"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "var businessAPIHandlerMap"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "route, ok := apiRouteDescriptorMap[routeName]"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "invoker, ok := apiHandlerMap[route.ApiName]"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "businessInvoker, ok := businessAPIHandlerMap[route.ApiName]"
assert_file_not_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "switch route.ApiName"
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
assert_file_contains "$TMPDIR/out-app-go/worker/cmd/worker/main.go" "worker.WorkerEntityGCDescriptors()"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/entity_gc_catalog.go" "ServiceInstanceEntityGCDescriptor()"
assert_file_contains "$TMPDIR/out-app-go/worker/cmd/worker/main.go" "process.Start(ctx)"
assert_file_contains "$TMPDIR/out-app-go/worker/cmd/worker/main.go" "process.Join()"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_runner.go" "KeepAliveStep"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "type WorkflowStepKey struct"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "WorkflowName    string"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "WorkflowVersion int"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "StepName        string"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "func (key WorkflowStepKey) Encode() string"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "func WorkflowStepKeyString(workflowName string, workflowVersion int, stepName string) string"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "workflowName + \":\" + strconv.Itoa(workflowVersion) + \":\" + stepName"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "common \"statespec-generated/common/backend\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "type WorkflowStepInvoker func(context.Context, common.Backend, WorkflowStepHandlerContext) error"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "type DefaultWorkflowStepHandler struct"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "\"ProvisionService.wait_for_remote_service\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "ProvisionServiceV1StepHandler"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflows/provision_service/handlers.go" "type ProvisionServiceV1StepHandler interface"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflows/provision_service/handlers.go" "HandleValidateRequest"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_runner.go" "workflowmodules.DispatchProvisionServiceStep"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflows/provision_service.go" "HandleValidateRequest"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflows/provision_service.go" "nextStep := \"create_remote_service\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflows/provision_service.go" "nextStep := \"wait_for_remote_service\""
cp "$SCRIPT_DIR/api_linking_fixture_test.go" "$TMPDIR/out-app-go/api/backend/api_linking_fixture_test.go"
cp "$SCRIPT_DIR/registration_restart_fixture_test.go" "$TMPDIR/out-app-go/common/backend/registration_restart_fixture_test.go"
cp "$SCRIPT_DIR/worker_linking_fixture_test.go" "$TMPDIR/out-app-go/worker/backend/worker_linking_fixture_test.go"
cp "$SCRIPT_DIR/worker_process_fixture_test.go" "$TMPDIR/out-app-go/worker/backend/worker_process_fixture_test.go"
run_expect_status 0 make -C "$TMPDIR/out-app-go" check
run_expect_status 0 env GOCACHE="$TMPDIR/go-cache" make -C "$TMPDIR/out-app-go" build
