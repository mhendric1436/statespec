#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../../.." && pwd)"
TESTS_DIR="$REPO_DIR/tests"
. "$TESTS_DIR/cli/common.sh"

APP_SPEC="$REPO_DIR/testdata/generators/canonical-api-worker-app.sspec"

# End-to-end generated API + worker application regression.
run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang cpp "$APP_SPEC" --out "$TMPDIR/out-app-cpp"
assert_file_contains "$TMPDIR/out-app-cpp/common/system_descriptors.hpp" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-cpp/common/system_descriptors.hpp" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-cpp/common/system_descriptors.hpp" "\"ProvisionWorker\""
assert_file_contains "$TMPDIR/out-app-cpp/api/api_server.hpp" "class ApiServer"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_dispatcher.hpp" "dispatch_api_route"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_registry.hpp" "find_worker_descriptor"
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_runner.hpp" "keep_alive_step"
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_step_handlers.hpp" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_step_handlers.hpp" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_step_handlers.hpp" "\"ProvisionService.wait_for_remote_service\""
run_expect_status 0 make -C "$TMPDIR/out-app-cpp" check

run_expect_status 0 "$CLI" generate bindings --lang go "$APP_SPEC" --out "$TMPDIR/out-app-go"
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
run_expect_status 0 make -C "$TMPDIR/out-app-go" check

run_expect_status 0 "$CLI" generate bindings --lang java "$APP_SPEC" --out "$TMPDIR/out-app-java"
assert_file_contains "$TMPDIR/out-app-java/common/com/statespec/generated/Descriptors.java" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-java/common/com/statespec/generated/Descriptors.java" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-java/common/com/statespec/generated/Descriptors.java" "\"ProvisionWorker\""
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiServer.java" "class ApiServer"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiDispatcher.java" "dispatchApiRoute"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRegistry.java" "findWorkerDescriptor"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowRunner.java" "keepAliveStep"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.wait_for_remote_service\""
run_expect_status 0 make -C "$TMPDIR/out-app-java" check

run_expect_status 0 "$CLI" generate bindings --lang rust "$APP_SPEC" --out "$TMPDIR/out-app-rust"
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
run_expect_status 0 make -C "$TMPDIR/out-app-rust" check
