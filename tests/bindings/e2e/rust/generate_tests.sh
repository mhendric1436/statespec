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
APP_MANIFEST="$TMPDIR/expected-rust-manifest.txt"

cat > "$APP_MANIFEST" <<'EOF'
Cargo.toml
Makefile
api/api_application.rs
api/api_codecs.rs
api/api_descriptors.rs
api/api_dispatcher.rs
api/api_handler_registry.rs
api/api_handlers.rs
api/api_process.rs
api/api_routes.rs
api/api_server.rs
api/api_transport.rs
api/external_system_operator_metadata_api.rs
api/main.rs
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
common/runtime/codec_leases.rs
common/runtime/codec_queues.rs
common/runtime/codec_workflows.rs
common/runtime/leases.rs
common/runtime/queues.rs
common/runtime/workflows.rs
common/schema_compatibility.rs
common/workflow.rs
lib.rs
worker/main.rs
worker/worker_application.rs
worker/worker_contexts.rs
worker/worker_descriptors.rs
worker/worker_leases.rs
worker/worker_queues.rs
worker/worker_registry.rs
worker/worker_runtime.rs
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
mkdir -p "$TMPDIR/out-e2e-rust/tests"
cp "$SCRIPT_DIR/api_process_fixture.rs" "$TMPDIR/out-e2e-rust/tests/api_process_fixture.rs"
run_expect_status 0 make -C "$TMPDIR/out-e2e-rust" check-api

run_expect_status 0 "$CLI" generate bindings --lang rust "$API_ENTITY_SPEC" --out "$TMPDIR/out-api-entities-rust"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "let repository = DefaultAccountRepository;"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "let repository = DefaultProjectRepository;"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "let repository = DefaultTaskRepository;"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "self.backend.begin()"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "create_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "\"created_at\""
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "\"updated_at\""
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "\"status\""
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "missing required parent Account"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "missing required parent Project"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "extract_api_path_parameters"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "get_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "list_by_index_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "status_code: 404"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "Json::Array(records.into_iter().map"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "update_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "invalid entity status transition"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "document.insert(\"updated_at\""
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "let requested_status = PROJECT_STATUS_DELETED.to_string()"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "invalid entity delete transition"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "status_code: 204"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/descriptors.rs" "update_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/descriptors.rs" "PROJECT_STATUS_DELETED"
mkdir -p "$TMPDIR/out-api-entities-rust/tests"
cp "$SCRIPT_DIR/api_persistence_fixture.rs" "$TMPDIR/out-api-entities-rust/tests/api_persistence_fixture.rs"
run_expect_status 0 make -C "$TMPDIR/out-api-entities-rust" check-api

run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang rust "$APP_SPEC" --out "$TMPDIR/out-app-rust"
assert_file_manifest_equals "$TMPDIR/out-app-rust" "$APP_MANIFEST"
assert_file_contains "$TMPDIR/out-app-rust/common/descriptors.rs" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-rust/common/descriptors.rs" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-rust/common/descriptors.rs" "\"ProvisionWorker\""
assert_file_contains "$TMPDIR/out-app-rust/api/api_application.rs" "pub struct ApiApplication"
assert_file_contains "$TMPDIR/out-app-rust/api/api_codecs.rs" "decode_start_provision_request"
assert_file_contains "$TMPDIR/out-app-rust/api/api_codecs.rs" "encode_start_provision_response"
assert_file_contains "$TMPDIR/out-app-rust/api/api_handler_registry.rs" "pub struct DefaultApiHandler"
assert_file_contains "$TMPDIR/out-app-rust/api/main.rs" "ApiProcessConfig::all_servers"
assert_file_contains "$TMPDIR/out-app-rust/api/main.rs" "install_signal_handling"
assert_file_not_contains "$TMPDIR/out-app-rust/api/main.rs" "ApiApplication::new_default"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "pub struct ApiProcess"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "pub fn request_stop(&self)"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "pub fn start(self: &Arc<Self>)"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "pub fn join(&self)"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "pub fn is_running(&self)"
assert_file_contains "$TMPDIR/out-app-rust/api/api_transport.rs" "pub trait ApiTransport"
assert_file_contains "$TMPDIR/out-app-rust/api/api_transport.rs" "server.handle(route_name, context)"
assert_file_contains "$TMPDIR/out-app-rust/api/api_transport.rs" "pub struct LocalBlockingApiTransport"
assert_file_contains "$TMPDIR/out-app-rust/api/main.rs" "LocalBlockingApiTransport::new()"
assert_file_contains "$TMPDIR/out-app-rust/api/main.rs" "process.start()"
assert_file_contains "$TMPDIR/out-app-rust/api/main.rs" "process.join()"
assert_file_contains "$TMPDIR/out-app-rust/api/api_server.rs" "pub struct ApiServer"
assert_file_contains "$TMPDIR/out-app-rust/api/api_dispatcher.rs" "pub fn dispatch_api_route"
assert_file_not_contains "$TMPDIR/out-app-rust/api/api_dispatcher.rs" "pub fn dispatch_api_operation_route"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_registry.rs" "pub fn find_worker_descriptor"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_runtime.rs" "pub struct WorkerRuntime"
assert_file_contains "$TMPDIR/out-app-rust/worker/main.rs" "WorkerRuntime"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_runner.rs" "keep_alive_step"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_step_handlers.rs" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_step_handlers.rs" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_step_handlers.rs" "\"ProvisionService.wait_for_remote_service\""
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_step_handlers.rs" "handle_provision_service_validate_request"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_runner.rs" "handle_provision_service_validate_request"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_runner.rs" "Some(\"create_remote_service\".to_string())"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_runner.rs" "Some(\"wait_for_remote_service\".to_string())"
mkdir -p "$TMPDIR/out-app-rust/tests"
cp "$SCRIPT_DIR/api_linking_fixture.rs" "$TMPDIR/out-app-rust/tests/api_linking_fixture.rs"
cp "$SCRIPT_DIR/registration_restart_fixture.rs" "$TMPDIR/out-app-rust/tests/registration_restart_fixture.rs"
cp "$SCRIPT_DIR/worker_linking_fixture.rs" "$TMPDIR/out-app-rust/tests/worker_linking_fixture.rs"
run_expect_status 0 make -C "$TMPDIR/out-app-rust" check
