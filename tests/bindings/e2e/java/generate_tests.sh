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
APP_MANIFEST="$TMPDIR/expected-java-manifest.txt"

cat > "$APP_MANIFEST" <<'EOF'
Makefile
api/com/statespec/generated/ApiApplication.java
api/com/statespec/generated/ApiDescriptors.java
api/com/statespec/generated/ApiDispatcher.java
api/com/statespec/generated/ApiHandlerRegistry.java
api/com/statespec/generated/ApiHandlers.java
api/com/statespec/generated/ApiMain.java
api/com/statespec/generated/ApiRoutes.java
api/com/statespec/generated/ApiServer.java
api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java
common/com/statespec/backend/Backend.java
common/com/statespec/backend/ExternalSystem.java
common/com/statespec/backend/FeatureFlag.java
common/com/statespec/backend/Json.java
common/com/statespec/backend/Lease.java
common/com/statespec/backend/Log.java
common/com/statespec/backend/Metric.java
common/com/statespec/backend/Queue.java
common/com/statespec/backend/Workflow.java
common/com/statespec/backend/memory/InMemoryBackend.java
common/com/statespec/backend/memory/InMemoryTransaction.java
common/com/statespec/backend/runtime/Codec.java
common/com/statespec/backend/runtime/FeatureFlagCodec.java
common/com/statespec/backend/runtime/FeatureFlagStore.java
common/com/statespec/backend/runtime/LeaseCodec.java
common/com/statespec/backend/runtime/LeaseStore.java
common/com/statespec/backend/runtime/LogCodec.java
common/com/statespec/backend/runtime/LogSink.java
common/com/statespec/backend/runtime/MetricCodec.java
common/com/statespec/backend/runtime/MetricSink.java
common/com/statespec/backend/runtime/ObservabilityCodec.java
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

run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang java "$APP_SPEC" --out "$TMPDIR/out-app-java"
assert_file_manifest_equals "$TMPDIR/out-app-java" "$APP_MANIFEST"
assert_file_contains "$TMPDIR/out-app-java/common/com/statespec/generated/Descriptors.java" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-java/common/com/statespec/generated/Descriptors.java" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-java/common/com/statespec/generated/Descriptors.java" "\"ProvisionWorker\""
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiApplication.java" "class ApiApplication"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiHandlerRegistry.java" "class DefaultHandler"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "ApiApplication"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiServer.java" "class ApiServer"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiDispatcher.java" "dispatchApiRoute"
assert_file_not_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiDispatcher.java" "dispatchApiOperationRoute"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRegistry.java" "findWorkerDescriptor"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRuntime.java" "class WorkerRuntime"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "WorkerRuntime"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowRunner.java" "keepAliveStep"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.wait_for_remote_service\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "handleProvisionServiceValidateRequest"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowRunner.java" "handleProvisionServiceValidateRequest"
cp "$SCRIPT_DIR/ApiLinkingFixture.java" "$TMPDIR/out-app-java/api/com/statespec/generated/ApiLinkingFixture.java"
cp "$SCRIPT_DIR/WorkerLinkingFixture.java" "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerLinkingFixture.java"
run_expect_status 0 make -C "$TMPDIR/out-app-java" check
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-app-java/build/classes" com.statespec.generated.ApiLinkingFixture
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-app-java/build/classes" com.statespec.generated.WorkerLinkingFixture
