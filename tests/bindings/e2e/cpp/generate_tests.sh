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
APP_MANIFEST="$TMPDIR/expected-cpp-manifest.txt"

cat > "$APP_MANIFEST" <<'EOF'
Makefile
api/api_descriptors.hpp
api/api_dispatcher.hpp
api/api_handlers.hpp
api/api_routes.hpp
api/api_server.hpp
api/external_system_operator_metadata_api.hpp
common/backend.hpp
common/descriptors.hpp
common/external_system.hpp
common/feature_flag.hpp
common/json.hpp
common/lease.hpp
common/log.hpp
common/memory/backend.hpp
common/memory/codec.hpp
common/memory/feature_flag_store.hpp
common/memory/lease_store.hpp
common/memory/log_sink.hpp
common/memory/metric_sink.hpp
common/memory/queue_store.hpp
common/memory/transaction.hpp
common/memory/workflow_store.hpp
common/metric.hpp
common/queue.hpp
common/workflow.hpp
worker/worker_application.hpp
worker/worker_contexts.hpp
worker/worker_descriptors.hpp
worker/worker_handlers.hpp
worker/worker_leases.hpp
worker/worker_queues.hpp
worker/worker_registry.hpp
worker/worker_workflows.hpp
worker/workflow_runner.hpp
worker/workflow_step_handlers.hpp
EOF

run_expect_status 0 "$CLI" generate bindings --lang cpp "$E2E_SPEC" --out "$TMPDIR/out-e2e-cpp"
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "api_route_descriptors"
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "IExternalSystemOperatorMetadataApiHandler"

run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang cpp "$APP_SPEC" --out "$TMPDIR/out-app-cpp"
assert_file_manifest_equals "$TMPDIR/out-app-cpp" "$APP_MANIFEST"
assert_file_contains "$TMPDIR/out-app-cpp/common/descriptors.hpp" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-cpp/common/descriptors.hpp" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-cpp/common/descriptors.hpp" "\"ProvisionWorker\""
assert_file_contains "$TMPDIR/out-app-cpp/api/api_server.hpp" "class ApiServer"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_dispatcher.hpp" "dispatch_api_route"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_registry.hpp" "find_worker_descriptor"
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_runner.hpp" "keep_alive_step"
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_step_handlers.hpp" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_step_handlers.hpp" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_step_handlers.hpp" "\"ProvisionService.wait_for_remote_service\""
run_expect_status 0 make -C "$TMPDIR/out-app-cpp" check
run_expect_status 0 "${CXX:-clang++}" -std=c++20 -Wall -Wextra -Wpedantic -I"$TMPDIR/out-app-cpp" -I"$TMPDIR/out-app-cpp/common" "$SCRIPT_DIR/api_linking_fixture.cpp" -o "$TMPDIR/out-app-cpp/build/api-linking-fixture"
run_expect_status 0 "$TMPDIR/out-app-cpp/build/api-linking-fixture"
run_expect_status 0 "${CXX:-clang++}" -std=c++20 -Wall -Wextra -Wpedantic -I"$TMPDIR/out-app-cpp" -I"$TMPDIR/out-app-cpp/common" "$SCRIPT_DIR/worker_linking_fixture.cpp" -o "$TMPDIR/out-app-cpp/build/worker-linking-fixture"
run_expect_status 0 "$TMPDIR/out-app-cpp/build/worker-linking-fixture"
