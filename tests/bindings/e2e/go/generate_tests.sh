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
APP_MANIFEST="$TMPDIR/expected-go-manifest.txt"

cat > "$APP_MANIFEST" <<'EOF'
Makefile
api/backend/api_descriptors.go
api/backend/api_dispatcher.go
api/backend/api_handlers.go
api/backend/api_routes.go
api/backend/api_server.go
api/backend/external_system_operator_metadata_api.go
common/backend/backend.go
common/backend/descriptors.go
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
common/backend/runtime/codec_feature_flags.go
common/backend/runtime/codec_leases.go
common/backend/runtime/codec_observability.go
common/backend/runtime/codec_queues.go
common/backend/runtime/codec_workflows.go
common/backend/runtime/feature_flags.go
common/backend/runtime/leases.go
common/backend/runtime/logs.go
common/backend/runtime/metrics.go
common/backend/runtime/queues.go
common/backend/runtime/workflows.go
common/backend/workflow.go
go.mod
worker/backend/worker_application.go
worker/backend/worker_contexts.go
worker/backend/worker_descriptors.go
worker/backend/worker_handlers.go
worker/backend/worker_leases.go
worker/backend/worker_queues.go
worker/backend/worker_registry.go
worker/backend/worker_workflows.go
worker/backend/workflow_runner.go
worker/backend/workflow_step_handlers.go
EOF

run_expect_status 0 "$CLI" generate bindings --lang go "$E2E_SPEC" --out "$TMPDIR/out-e2e-go"
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "func ApiRouteDescriptors"
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "ExternalSystemOperatorMetadataAPIHandler"

run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang go "$APP_SPEC" --out "$TMPDIR/out-app-go"
assert_file_manifest_equals "$TMPDIR/out-app-go" "$APP_MANIFEST"
assert_file_contains "$TMPDIR/out-app-go/common/backend/descriptors.go" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-go/common/backend/descriptors.go" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-go/common/backend/descriptors.go" "\"ProvisionWorker\""
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_server.go" "type APITierServer struct"
assert_file_contains "$TMPDIR/out-app-go/api/backend/api_dispatcher.go" "func DispatchAPITierRoute"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/worker_registry.go" "func FindWorkerTierDescriptor"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_runner.go" "KeepAliveStep"
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-go/worker/backend/workflow_step_handlers.go" "\"ProvisionService.wait_for_remote_service\""
cp "$SCRIPT_DIR/api_linking_fixture_test.go" "$TMPDIR/out-app-go/api/backend/api_linking_fixture_test.go"
cp "$SCRIPT_DIR/worker_linking_fixture_test.go" "$TMPDIR/out-app-go/worker/backend/worker_linking_fixture_test.go"
run_expect_status 0 make -C "$TMPDIR/out-app-go" check
