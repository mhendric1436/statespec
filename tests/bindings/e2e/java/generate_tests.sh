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
api/com/statespec/generated/ApiHandlers.java
api/com/statespec/generated/ApiMain.java
api/com/statespec/generated/ApiProcess.java
api/com/statespec/generated/ApiRoutes.java
api/com/statespec/generated/ApiServer.java
api/com/statespec/generated/ApiTransport.java
api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java
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
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "new Descriptors.DefaultAccountRepository()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "new Descriptors.DefaultProjectRepository()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "new Descriptors.DefaultTaskRepository()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "backend.begin()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "repository.createTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "\"created_at\""
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "\"updated_at\""
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "\"status\""
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "missing required parent Account"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "missing required parent Project"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "extractApiPathParameters"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "repository.getTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "repository.listByIndexTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "new Descriptors.ApiResponse(404"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "com.statespec.backend.Json.array(items)"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "repository.updateTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "invalid entity status transition"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "document.put(\"updated_at\""
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "var requestedStatus = Descriptors.PROJECT_STATUS_DELETED"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "invalid entity delete transition"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "new Descriptors.ApiResponse(204"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/Descriptors.java" "updateTx"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/Descriptors.java" "PROJECT_STATUS_DELETED"
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
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiCodecs.java" "decodeStartProvisionRequest"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiCodecs.java" "encodeStartProvisionResponse"
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
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "WorkerProcess"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "addShutdownHook"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "process.start()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "process.join()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowRunner.java" "keepAliveStep"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.validate_request\""
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
