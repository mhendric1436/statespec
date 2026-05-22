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
APP_MANIFEST="$TMPDIR/expected-rust-manifest.txt"

cat > "$APP_MANIFEST" <<'EOF'
Cargo.toml
Makefile
api/api_descriptors.rs
api/api_dispatcher.rs
api/api_handlers.rs
api/api_routes.rs
api/api_server.rs
api/external_system_operator_metadata_api.rs
common/backend.rs
common/descriptors.rs
common/external_system.rs
common/feature_flag.rs
common/json.rs
common/lease.rs
common/log.rs
common/memory/backend.rs
common/memory/transaction.rs
common/metric.rs
common/queue.rs
common/runtime/codec.rs
common/runtime/codec_core.rs
common/runtime/codec_feature_flags.rs
common/runtime/codec_leases.rs
common/runtime/codec_logs.rs
common/runtime/codec_metrics.rs
common/runtime/codec_observability.rs
common/runtime/codec_queues.rs
common/runtime/codec_workflows.rs
common/runtime/feature_flags.rs
common/runtime/leases.rs
common/runtime/logs.rs
common/runtime/metrics.rs
common/runtime/queues.rs
common/runtime/workflows.rs
common/workflow.rs
lib.rs
worker/worker_application.rs
worker/worker_contexts.rs
worker/worker_descriptors.rs
worker/worker_handlers.rs
worker/worker_leases.rs
worker/worker_queues.rs
worker/worker_registry.rs
worker/worker_workflows.rs
worker/workflow_runner.rs
worker/workflow_step_handlers.rs
EOF

run_expect_status 0 "$CLI" generate bindings --lang rust "$E2E_SPEC" --out "$TMPDIR/out-e2e-rust"
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "pub fn api_route_descriptors"
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "ExternalSystemOperatorMetadataApiHandler"

run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang rust "$APP_SPEC" --out "$TMPDIR/out-app-rust"
assert_file_manifest_equals "$TMPDIR/out-app-rust" "$APP_MANIFEST"
assert_file_contains "$TMPDIR/out-app-rust/common/descriptors.rs" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-rust/common/descriptors.rs" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-rust/common/descriptors.rs" "\"ProvisionWorker\""
assert_file_contains "$TMPDIR/out-app-rust/api/api_server.rs" "pub struct ApiServer"
assert_file_contains "$TMPDIR/out-app-rust/api/api_dispatcher.rs" "pub fn dispatch_api_route"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_registry.rs" "pub fn find_worker_descriptor"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_runner.rs" "keep_alive_step"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_step_handlers.rs" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_step_handlers.rs" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_step_handlers.rs" "\"ProvisionService.wait_for_remote_service\""
mkdir -p "$TMPDIR/out-app-rust/tests"
cp "$SCRIPT_DIR/api_linking_fixture.rs" "$TMPDIR/out-app-rust/tests/api_linking_fixture.rs"
cp "$SCRIPT_DIR/worker_linking_fixture.rs" "$TMPDIR/out-app-rust/tests/worker_linking_fixture.rs"
run_expect_status 0 make -C "$TMPDIR/out-app-rust" check
