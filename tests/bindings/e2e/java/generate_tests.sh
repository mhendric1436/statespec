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
APP_MANIFEST="$TMPDIR/expected-java-manifest.txt"

cat > "$APP_MANIFEST" <<'EOF'
Makefile
api/com/statespec/generated/ApiApplication.java
api/com/statespec/generated/ApiCodecs.java
api/com/statespec/generated/ApiDescriptors.java
api/com/statespec/generated/ApiDispatcher.java
api/com/statespec/generated/ApiHandlerRegistry.java
api/com/statespec/generated/ApiHandlerRegistryOperations.java
api/com/statespec/generated/ApiHandlers.java
api/com/statespec/generated/ApiMain.java
api/com/statespec/generated/ApiProcess.java
api/com/statespec/generated/ApiRoutes.java
api/com/statespec/generated/ApiServer.java
api/com/statespec/generated/ApiTransport.java
api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java
api/com/statespec/generated/codecs/ApiCodecsProvisionCallbackRequest.java
api/com/statespec/generated/codecs/ApiCodecsProvisionCallbackResponse.java
api/com/statespec/generated/codecs/ApiCodecsStartProvisionRequest.java
api/com/statespec/generated/codecs/ApiCodecsStartProvisionResponse.java
api/com/statespec/generated/descriptors/ReportProvisionReadyDescriptorModule.java
api/com/statespec/generated/descriptors/StartProvisionDescriptorModule.java
common/com/statespec/backend/Backend.java
common/com/statespec/backend/ExternalSystem.java
common/com/statespec/backend/FeatureFlag.java
common/com/statespec/backend/Json.java
common/com/statespec/backend/Lease.java
common/com/statespec/backend/Log.java
common/com/statespec/backend/Metric.java
common/com/statespec/backend/Queue.java
common/com/statespec/backend/SchemaCompatibility.java
common/com/statespec/backend/Workflow.java
common/com/statespec/backend/memory/InMemoryBackend.java
common/com/statespec/backend/memory/InMemoryTransaction.java
common/com/statespec/backend/runtime/Codec.java
common/com/statespec/backend/runtime/EntityGcDescriptors.java
common/com/statespec/backend/runtime/EntityGcRepository.java
common/com/statespec/backend/runtime/EntityGcWorkers.java
common/com/statespec/backend/runtime/LeaseCodec.java
common/com/statespec/backend/runtime/LeaseStore.java
common/com/statespec/backend/runtime/QueueCodec.java
common/com/statespec/backend/runtime/QueueStore.java
common/com/statespec/backend/runtime/WorkflowCodec.java
common/com/statespec/backend/runtime/WorkflowStore.java
common/com/statespec/generated/Descriptors.java
common/com/statespec/generated/descriptors/ApiDescriptorModule.java
common/com/statespec/generated/descriptors/CoreDescriptorModule.java
common/com/statespec/generated/descriptors/RuntimeDescriptorModule.java
common/com/statespec/generated/descriptors/ShapeDescriptorModule.java
common/com/statespec/generated/descriptors/WorkerDescriptorModule.java
common/com/statespec/generated/descriptors/entities/ServiceInstanceDescriptorModule.java
common/com/statespec/generated/shapes/ProvisionCallbackRequest.java
common/com/statespec/generated/shapes/ProvisionCallbackResponse.java
common/com/statespec/generated/shapes/StartProvisionRequest.java
common/com/statespec/generated/shapes/StartProvisionResponse.java
common/com/statespec/generated/workflows/ProvisionServiceDescriptorModule.java
worker/com/statespec/generated/WorkerApplication.java
worker/com/statespec/generated/WorkerContexts.java
worker/com/statespec/generated/WorkerDescriptors.java
worker/com/statespec/generated/WorkerLeases.java
worker/com/statespec/generated/WorkerMain.java
worker/com/statespec/generated/WorkerProcess.java
worker/com/statespec/generated/WorkerQueues.java
worker/com/statespec/generated/WorkerRegistry.java
worker/com/statespec/generated/WorkerRuntime.java
worker/com/statespec/generated/WorkerWorkflows.java
worker/com/statespec/generated/WorkflowRunner.java
worker/com/statespec/generated/WorkflowStepHandlers.java
EOF

run_expect_status 0 "$CLI" generate bindings --lang java "$E2E_SPEC" --out "$TMPDIR/out-e2e-java"
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "apiRouteDescriptors"
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "ExternalSystemOperatorMetadataApiHandler"
cp "$SCRIPT_DIR/ApiProcessFixture.java" "$TMPDIR/out-e2e-java/api/com/statespec/generated/ApiProcessFixture.java"
run_expect_status 0 make -C "$TMPDIR/out-e2e-java" build-api
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-e2e-java/build/classes" com.statespec.generated.ApiProcessFixture

run_expect_status 0 "$CLI" generate bindings --lang java "$API_ENTITY_SPEC" --out "$TMPDIR/out-api-entities-java"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "new AccountDescriptorModule.DefaultAccountRepository()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryProject.java" "new ProjectDescriptorModule.DefaultProjectRepository()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryTask.java" "new TaskDescriptorModule.DefaultTaskRepository()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "backend.begin()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "repository.createTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "AccountDescriptorModule.ACCOUNT_FIELD_CREATED_AT"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "AccountDescriptorModule.ACCOUNT_FIELD_UPDATED_AT"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "AccountDescriptorModule.ACCOUNT_FIELD_STATUS"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "document.put(AccountDescriptorModule.ACCOUNT_FIELD_CREATED_AT, com.statespec.backend.Json.string(java.time.Instant.now().toString()))"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "document.put(AccountDescriptorModule.ACCOUNT_FIELD_STATUS, com.statespec.backend.Json.string(AccountDescriptorModule.ACCOUNT_STATUS_ACTIVE))"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryProject.java" "missing required parent Account"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryTask.java" "missing required parent Project"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "extractApiPathParameters"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "repository.getTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "repository.listByTenantAccountTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryProject.java" "repository.listByAccountStatusTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryTask.java" "repository.listByProjectStatusTx"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/ProjectDescriptorModule.java" "listByTenantProjectTx"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/TaskDescriptorModule.java" "listByTenantTaskTx"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/TaskDescriptorModule.java" "listByAccountPriorityTx"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/AccountDescriptorModule.java" "public static final String ACCOUNT_ENTITY_NAME = \"Account\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/AccountDescriptorModule.java" "public static final String ACCOUNT_FIELD_TENANT_ID = \"tenant_id\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/AccountDescriptorModule.java" "public static final String ACCOUNT_FIELD_CREATED_AT_TYPE_NAME = \"timestamp\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/AccountDescriptorModule.java" "public static final String ACCOUNT_FIELD_STATUS_TYPE_NAME = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/AccountDescriptorModule.java" "public static final String ACCOUNT_INDEX_BY_TENANT_ACCOUNT = \"by_tenant_account\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/AccountDescriptorModule.java" "new FieldDescriptor(ACCOUNT_FIELD_CREATED_AT, FieldType.TIMESTAMP, ACCOUNT_FIELD_CREATED_AT_TYPE_NAME, true)"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/AccountDescriptorModule.java" "new FieldDescriptor(ACCOUNT_FIELD_STATUS, FieldType.STRING, ACCOUNT_FIELD_STATUS_TYPE_NAME, true)"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/ShapeDescriptorModule.java" "public static final String CREATE_ACCOUNT_REQUEST_SHAPE_NAME = \"CreateAccountRequest\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/ShapeDescriptorModule.java" "public static final String CREATE_ACCOUNT_REQUEST_FIELD_TENANT_ID = \"tenant_id\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/ShapeDescriptorModule.java" "public static final String CREATE_ACCOUNT_REQUEST_FIELD_TENANT_ID_TYPE_NAME = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/ShapeDescriptorModule.java" "new FieldDescriptor(CREATE_ACCOUNT_REQUEST_FIELD_TENANT_ID, FieldType.STRING, CREATE_ACCOUNT_REQUEST_FIELD_TENANT_ID_TYPE_NAME, true)"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/AccountDescriptorModule.java" "List.of(ACCOUNT_FIELD_TENANT_ID, ACCOUNT_FIELD_ACCOUNT_ID)"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/Descriptors.java" "key.append(keyField).append('=').append(value.canonicalString()).append('\\n')"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/Descriptors.java" "values.put(keyValue.field(), keyValue.value())"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "new Descriptors.ApiResponse(404"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryAccount.java" "com.statespec.backend.Json.array(items)"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryProject.java" "repository.updateTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryProject.java" "invalid entity status transition"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryProject.java" "document.put(ProjectDescriptorModule.PROJECT_FIELD_UPDATED_AT"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryProject.java" "var requestedStatus = ProjectDescriptorModule.PROJECT_STATUS_DELETED"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryProject.java" "invalid entity delete transition"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistryProject.java" "new Descriptors.ApiResponse(204"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/ProjectDescriptorModule.java" "updateTx"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/ProjectDescriptorModule.java" "PROJECT_STATUS_DELETED"
cp "$SCRIPT_DIR/ApiPersistenceFixture.java" "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiPersistenceFixture.java"
run_expect_status 0 make -C "$TMPDIR/out-api-entities-java" build-api
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-api-entities-java/build/classes" com.statespec.generated.ApiPersistenceFixture

run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang java "$APP_SPEC" --out "$TMPDIR/out-app-java"
assert_file_manifest_equals "$TMPDIR/out-app-java" "$APP_MANIFEST"
assert_file_contains "$TMPDIR/out-app-java/common/com/statespec/generated/Descriptors.java" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-java/common/com/statespec/generated/Descriptors.java" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-java/common/com/statespec/generated/Descriptors.java" "\"ProvisionWorker\""
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiApplication.java" "class ApiApplication"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/codecs/ApiCodecsStartProvisionRequest.java" "decodeStartProvisionRequest"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/codecs/ApiCodecsStartProvisionResponse.java" "encodeStartProvisionResponse"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiHandlerRegistry.java" "class DefaultHandler"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "ApiProcess.Config.allServers"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "addShutdownHook"
assert_file_not_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "ApiApplication.createDefault"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "public final class ApiProcess"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "public void requestStop()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "public synchronized void start()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "public int join()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "public synchronized boolean isRunning()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "boolean entityGcEnabled"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "public synchronized void addEntityGcWorker"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "startEntityGcWorkers()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiTransport.java" "interface ApiTransport"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiTransport.java" "return server.handle(routeName, context)"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiTransport.java" "final class LocalBlocking"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiTransport.java" "void requestStop()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiTransport.java" "stopRequested.await()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "transport.requestStop()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "new ApiTransport.LocalBlocking()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "process.start()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "process.join()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiServer.java" "class ApiServer"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiDispatcher.java" "dispatchApiRoute"
assert_file_not_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiDispatcher.java" "dispatchApiOperationRoute"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRegistry.java" "findWorkerDescriptor"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRuntime.java" "class WorkerRuntime"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRuntime.java" "record Config(boolean entityGcEnabled"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRuntime.java" "startEntityGcWorkers"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRuntime.java" "public void requestStop()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcess.java" "class WorkerProcess"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcess.java" "public synchronized void start()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcess.java" "public int join()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcess.java" "public synchronized boolean isRunning()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcess.java" "public void requestStop()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcess.java" "startWorkerLoops"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "WorkerProcess"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "WorkflowStepHandlers.DefaultHandler"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "addShutdownHook"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "process.start()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "process.join()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowRunner.java" "keepAliveStep"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "DefaultHandler implements Handler"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.wait_for_remote_service\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "handleProvisionServiceValidateRequest"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowRunner.java" "handleProvisionServiceValidateRequest"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowRunner.java" "nextStep = Optional.of(\"create_remote_service\")"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowRunner.java" "nextStep = Optional.of(\"wait_for_remote_service\")"
cp "$SCRIPT_DIR/ApiLinkingFixture.java" "$TMPDIR/out-app-java/api/com/statespec/generated/ApiLinkingFixture.java"
cp "$SCRIPT_DIR/RegistrationRestartFixture.java" "$TMPDIR/out-app-java/common/com/statespec/generated/RegistrationRestartFixture.java"
cp "$SCRIPT_DIR/WorkerLinkingFixture.java" "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerLinkingFixture.java"
cp "$SCRIPT_DIR/WorkerProcessFixture.java" "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcessFixture.java"
run_expect_status 0 make -C "$TMPDIR/out-app-java" check
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-app-java/build/classes" com.statespec.generated.ApiLinkingFixture
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-app-java/build/classes" com.statespec.generated.RegistrationRestartFixture
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-app-java/build/classes" com.statespec.generated.WorkerLinkingFixture
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-app-java/build/classes" com.statespec.generated.WorkerProcessFixture
