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
CPP_MANIFEST="$TMPDIR/expected-cpp-manifest.txt"
GO_MANIFEST="$TMPDIR/expected-go-manifest.txt"
JAVA_MANIFEST="$TMPDIR/expected-java-manifest.txt"
RUST_MANIFEST="$TMPDIR/expected-rust-manifest.txt"

cat > "$CPP_MANIFEST" <<'EOF'
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

cat > "$GO_MANIFEST" <<'EOF'
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
common/backend/metric.go
common/backend/queue.go
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

cat > "$JAVA_MANIFEST" <<'EOF'
Makefile
api/com/statespec/generated/ApiDescriptors.java
api/com/statespec/generated/ApiDispatcher.java
api/com/statespec/generated/ApiHandlers.java
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
common/com/statespec/generated/Descriptors.java
worker/com/statespec/generated/WorkerApplication.java
worker/com/statespec/generated/WorkerContexts.java
worker/com/statespec/generated/WorkerDescriptors.java
worker/com/statespec/generated/WorkerHandlers.java
worker/com/statespec/generated/WorkerLeases.java
worker/com/statespec/generated/WorkerQueues.java
worker/com/statespec/generated/WorkerRegistry.java
worker/com/statespec/generated/WorkerWorkflows.java
worker/com/statespec/generated/WorkflowRunner.java
worker/com/statespec/generated/WorkflowStepHandlers.java
EOF

cat > "$RUST_MANIFEST" <<'EOF'
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
common/metric.rs
common/queue.rs
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

# End-to-end generated API + worker application regression.
run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang cpp "$APP_SPEC" --out "$TMPDIR/out-app-cpp"
assert_file_manifest_equals "$TMPDIR/out-app-cpp" "$CPP_MANIFEST"
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

run_expect_status 0 "$CLI" generate bindings --lang go "$APP_SPEC" --out "$TMPDIR/out-app-go"
assert_file_manifest_equals "$TMPDIR/out-app-go" "$GO_MANIFEST"
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
assert_file_manifest_equals "$TMPDIR/out-app-java" "$JAVA_MANIFEST"
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
assert_file_manifest_equals "$TMPDIR/out-app-rust" "$RUST_MANIFEST"
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
