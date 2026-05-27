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
APP_MANIFEST="$TMPDIR/expected-cpp-manifest.txt"

cat > "$APP_MANIFEST" <<'EOF'
Makefile
api/api_application.hpp
api/api_codec_support.hpp
api/api_codecs.hpp
api/api_descriptors.hpp
api/api_dispatcher.hpp
api/api_handler_registry.hpp
api/api_handler_registry_support.hpp
api/api_handlers.hpp
api/api_process.hpp
api/api_routes.hpp
api/api_server.hpp
api/api_transport.hpp
api/codecs/provision_callback_request.hpp
api/codecs/provision_callback_response.hpp
api/codecs/start_provision_request.hpp
api/codecs/start_provision_response.hpp
api/descriptors/catalog.hpp
api/descriptors/report_provision_ready.hpp
api/descriptors/start_provision.hpp
api/external_system_operator_metadata_api.hpp
api/handlers/operations.hpp
api/main.cpp
api/shapes.hpp
api/shapes/provision_callback_request.hpp
api/shapes/provision_callback_response.hpp
api/shapes/start_provision_request.hpp
api/shapes/start_provision_response.hpp
common/backend.hpp
common/descriptors.hpp
common/descriptors/core.hpp
common/descriptors/events.hpp
common/descriptors/external_systems.hpp
common/descriptors/runtime.hpp
common/descriptors/runtime/leases.hpp
common/descriptors/runtime/queues.hpp
common/descriptors/runtime/workflows.hpp
common/descriptors/shapes.hpp
common/descriptors/shapes/provision_callback_request.hpp
common/descriptors/shapes/provision_callback_response.hpp
common/descriptors/shapes/start_provision_request.hpp
common/descriptors/shapes/start_provision_response.hpp
common/entities/service_instance/model.hpp
common/entities/service_instance/persistence.hpp
common/entities/service_instance/schema.hpp
common/entity_repository.hpp
common/external_system.hpp
common/feature_flag.hpp
common/json.hpp
common/lease.hpp
common/log.hpp
common/memory/backend.hpp
common/memory/transaction.hpp
common/metric.hpp
common/queue.hpp
common/runtime/codec.hpp
common/runtime/codec_core.hpp
common/runtime/codec_leases.hpp
common/runtime/codec_queues.hpp
common/runtime/codec_workflows.hpp
common/runtime/entity_gc_descriptors.hpp
common/runtime/entity_gc_repository.hpp
common/runtime/entity_gc_workers.hpp
common/runtime/lease_store.hpp
common/runtime/queue_store.hpp
common/runtime/workflow_store.hpp
common/runtime_registration.hpp
common/schema_compatibility.hpp
common/workflow.hpp
common/workflows/provision_service.hpp
worker/descriptors/catalog.hpp
worker/descriptors/provision_worker.hpp
worker/main.cpp
worker/registry/provision_worker.hpp
worker/worker_application.hpp
worker/worker_contexts.hpp
worker/worker_descriptors.hpp
worker/worker_leases.hpp
worker/worker_process.hpp
worker/worker_queues.hpp
worker/worker_registry.hpp
worker/worker_runtime.hpp
worker/worker_workflows.hpp
worker/workflow_runner.hpp
worker/workflow_step_handlers.hpp
worker/workflows/provision_service.hpp
EOF

run_expect_status 0 "$CLI" generate bindings --lang cpp "$E2E_SPEC" --out "$TMPDIR/out-e2e-cpp"
assert_file_contains "$TMPDIR/out-e2e-cpp/api/descriptors/catalog.hpp" "api_route_descriptors"
assert_file_contains "$TMPDIR/out-e2e-cpp/api/descriptors/catalog.hpp" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-cpp/api/descriptors/catalog.hpp" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors/external_systems.hpp" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors/external_systems.hpp" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "IExternalSystemOperatorMetadataApiHandler"
run_expect_status 0 "${CXX:-clang++}" -std=c++20 -Wall -Wextra -Wpedantic -I"$TMPDIR/out-e2e-cpp" -I"$TMPDIR/out-e2e-cpp/common" "$SCRIPT_DIR/api_process_fixture.cpp" -o "$TMPDIR/api-process-fixture-cpp"
run_expect_status 0 "$TMPDIR/api-process-fixture-cpp"

run_expect_status 0 "$CLI" generate bindings --lang cpp "$API_ENTITY_SPEC" --out "$TMPDIR/out-api-entities-cpp"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/worker/worker_descriptors.hpp"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/worker/worker_registry.hpp"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/Makefile" "check-worker"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/Makefile" "build-worker"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "::statespec_generated::entities::account::DefaultAccountRepository repository"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/project.hpp" "::statespec_generated::entities::project::DefaultProjectRepository repository"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/task.hpp" "::statespec_generated::entities::task::DefaultTaskRepository repository"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "backend_.begin()"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "repository.createTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "kAccountFieldCreatedAt"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "kAccountFieldUpdatedAt"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "kAccountFieldStatus"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "document[::statespec_generated::entities::account::kAccountFieldCreatedAt] = statespec::backend::Json{generated_api_timestamp()}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "document[::statespec_generated::entities::account::kAccountFieldStatus] = statespec::backend::Json{::statespec_generated::entities::account::kAccountStatusActive}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/project.hpp" "missing required parent Account"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/task.hpp" "missing required parent Project"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "extract_api_path_parameters"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "repository.getTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "repository.listByTenantAccountTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/project.hpp" "repository.listByAccountStatusTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/task.hpp" "repository.listByProjectStatusTx"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/common/descriptors/entities/account.hpp"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/common/descriptors/entities/project.hpp"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/common/descriptors/entities/task.hpp"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/project/persistence.hpp" "listByTenantProjectTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/task/persistence.hpp" "listByTenantTaskTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/task/persistence.hpp" "listByAccountPriorityTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/model.hpp" "inline constexpr const char* kAccountEntityName = \"Account\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/model.hpp" "inline constexpr const char* kAccountFieldTenantId = \"tenant_id\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/model.hpp" "inline constexpr const char* kAccountFieldCreatedAtTypeName = \"timestamp\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/model.hpp" "inline constexpr const char* kAccountFieldStatusTypeName = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/model.hpp" "inline constexpr const char* kAccountIndexByTenantAccount = \"by_tenant_account\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/model.hpp" "FieldDescriptor{kAccountFieldCreatedAt, statespec::backend::FieldType::Timestamp, kAccountFieldCreatedAtTypeName, true}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/model.hpp" "FieldDescriptor{kAccountFieldStatus, statespec::backend::FieldType::String, kAccountFieldStatusTypeName, true}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/descriptors/shapes/create_account_request.hpp" "inline constexpr const char* kCreateAccountRequestShapeName = \"CreateAccountRequest\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/descriptors/shapes/create_account_request.hpp" "inline constexpr const char* kCreateAccountRequestFieldTenantId = \"tenant_id\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/descriptors/shapes/create_account_request.hpp" "inline constexpr const char* kCreateAccountRequestFieldTenantIdTypeName = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/descriptors/shapes/create_account_request.hpp" "FieldDescriptor{kCreateAccountRequestFieldTenantId, statespec::backend::FieldType::String, kCreateAccountRequestFieldTenantIdTypeName, true}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/schema.hpp" "{kAccountFieldTenantId, kAccountFieldAccountId}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entity_repository.hpp" "key += value->canonical_string()"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entity_repository.hpp" "object[key_value.field] = key_value.value"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "return ApiResponse{404"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/account.hpp" "statespec::backend::Json::array(std::move(items))"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/project.hpp" "repository.updateTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/project.hpp" "invalid entity status transition"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/project.hpp" "document[::statespec_generated::entities::project::kProjectFieldUpdatedAt]"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/project.hpp" "const auto requested_status = std::string{::statespec_generated::entities::project::kProjectStatusDeleted}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/project.hpp" "invalid entity delete transition"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/handlers/project.hpp" "return ApiResponse{204"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/project/persistence.hpp" "updateTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/project/model.hpp" "kProjectStatusDeleted"
run_expect_status 0 make -C "$TMPDIR/out-api-entities-cpp" check-api
run_expect_status 0 "${CXX:-clang++}" -std=c++20 -Wall -Wextra -Wpedantic -I"$TMPDIR/out-api-entities-cpp" -I"$TMPDIR/out-api-entities-cpp/common" "$SCRIPT_DIR/api_persistence_fixture.cpp" -o "$TMPDIR/out-api-entities-cpp/build/api-persistence-fixture"
run_expect_status 0 "$TMPDIR/out-api-entities-cpp/build/api-persistence-fixture"

run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang cpp "$APP_SPEC" --out "$TMPDIR/out-app-cpp"
assert_file_manifest_equals "$TMPDIR/out-app-cpp" "$APP_MANIFEST"
assert_file_contains "$TMPDIR/out-app-cpp/api/descriptors/start_provision.hpp" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-cpp/worker/descriptors/provision_worker.hpp" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-cpp/worker/descriptors/provision_worker.hpp" "\"ProvisionWorker\""
assert_file_contains "$TMPDIR/out-app-cpp/api/api_application.hpp" "class ApiApplication"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_process.hpp" "class ApiProcess"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_process.hpp" "void request_stop()"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_process.hpp" "int start()"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_process.hpp" "int join()"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_process.hpp" "bool running() const"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_process.hpp" "bool entity_gc_enabled = true"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_process.hpp" "void add_entity_gc_worker"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_process.hpp" "start_entity_gc_workers()"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_transport.hpp" "class IApiTransport"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_transport.hpp" "server.handle(route_name, context)"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_transport.hpp" "class LocalBlockingApiTransport"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_transport.hpp" "virtual void request_stop() = 0"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_transport.hpp" "void request_stop() override"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_process.hpp" "transport_.request_stop()"
assert_file_contains "$TMPDIR/out-app-cpp/api/main.cpp" "LocalBlockingApiTransport transport"
assert_file_contains "$TMPDIR/out-app-cpp/api/codecs/start_provision_request.hpp" "decode_start_provision_request"
assert_file_contains "$TMPDIR/out-app-cpp/api/codecs/start_provision_response.hpp" "encode_start_provision_response"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_handler_registry.hpp" "class DefaultApiHandler"
assert_file_contains "$TMPDIR/out-app-cpp/api/main.cpp" "ApiProcess"
assert_file_contains "$TMPDIR/out-app-cpp/api/main.cpp" "std::signal(SIGTERM"
assert_file_not_contains "$TMPDIR/out-app-cpp/api/main.cpp" "ApiApplication app"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_server.hpp" "class ApiServer"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_dispatcher.hpp" "dispatch_api_route"
assert_file_not_contains "$TMPDIR/out-app-cpp/api/api_dispatcher.hpp" "dispatch_api_operation_route"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_registry.hpp" "find_worker_descriptor"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_runtime.hpp" "class WorkerRuntime"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_runtime.hpp" "struct WorkerRuntimeConfig"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_runtime.hpp" "void start_entity_gc_workers"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_runtime.hpp" "void request_stop()"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_process.hpp" "class WorkerProcess"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_process.hpp" "int start()"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_process.hpp" "int join()"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_process.hpp" "bool running() const"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_process.hpp" "void request_stop()"
assert_file_contains "$TMPDIR/out-app-cpp/worker/worker_process.hpp" "start_worker_loops"
assert_file_contains "$TMPDIR/out-app-cpp/worker/main.cpp" "WorkerProcess"
assert_file_contains "$TMPDIR/out-app-cpp/worker/main.cpp" "DefaultWorkflowStepHandler"
assert_file_contains "$TMPDIR/out-app-cpp/worker/main.cpp" "std::signal(SIGTERM"
assert_file_contains "$TMPDIR/out-app-cpp/worker/main.cpp" "process.start()"
assert_file_contains "$TMPDIR/out-app-cpp/worker/main.cpp" "process.join()"
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_runner.hpp" "keep_alive_step"
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_step_handlers.hpp" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_step_handlers.hpp" "DefaultWorkflowStepHandler"
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_step_handlers.hpp" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_step_handlers.hpp" "\"ProvisionService.wait_for_remote_service\""
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_step_handlers.hpp" "handle_provision_service_validate_request"
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflow_runner.hpp" "provision_service_dispatch_step"
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflows/provision_service.hpp" "handler.handle_provision_service_validate_request"
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflows/provision_service.hpp" "return \"create_remote_service\""
assert_file_contains "$TMPDIR/out-app-cpp/worker/workflows/provision_service.hpp" "return \"wait_for_remote_service\""
run_expect_status 0 make -C "$TMPDIR/out-app-cpp" check
run_expect_status 0 "${CXX:-clang++}" -std=c++20 -Wall -Wextra -Wpedantic -I"$TMPDIR/out-app-cpp" -I"$TMPDIR/out-app-cpp/common" "$SCRIPT_DIR/api_linking_fixture.cpp" -o "$TMPDIR/out-app-cpp/build/api-linking-fixture"
run_expect_status 0 "$TMPDIR/out-app-cpp/build/api-linking-fixture"
run_expect_status 0 "${CXX:-clang++}" -std=c++20 -Wall -Wextra -Wpedantic -I"$TMPDIR/out-app-cpp" -I"$TMPDIR/out-app-cpp/common" "$SCRIPT_DIR/registration_restart_fixture.cpp" -o "$TMPDIR/out-app-cpp/build/registration-restart-fixture"
run_expect_status 0 "$TMPDIR/out-app-cpp/build/registration-restart-fixture"
run_expect_status 0 "${CXX:-clang++}" -std=c++20 -Wall -Wextra -Wpedantic -I"$TMPDIR/out-app-cpp" -I"$TMPDIR/out-app-cpp/common" "$SCRIPT_DIR/worker_linking_fixture.cpp" -o "$TMPDIR/out-app-cpp/build/worker-linking-fixture"
run_expect_status 0 "$TMPDIR/out-app-cpp/build/worker-linking-fixture"
run_expect_status 0 "${CXX:-clang++}" -std=c++20 -Wall -Wextra -Wpedantic -I"$TMPDIR/out-app-cpp" -I"$TMPDIR/out-app-cpp/common" "$SCRIPT_DIR/worker_process_fixture.cpp" -o "$TMPDIR/out-app-cpp/build/worker-process-fixture"
run_expect_status 0 "$TMPDIR/out-app-cpp/build/worker-process-fixture"
