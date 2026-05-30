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
WORKFLOW_ENTITY_SPEC="$REPO_DIR/examples/workflow-entities-only.sspec"
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
api/entity_gc_catalog.hpp
api/external_system_operator_metadata_api.hpp
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
common/descriptors/policies.hpp
common/descriptors/runtime.hpp
common/descriptors/runtime/leases.hpp
common/descriptors/runtime/queues.hpp
common/descriptors/runtime/workflows.hpp
common/descriptors/types.hpp
common/descriptors/values_enums.hpp
common/entities/service_instance/constants.hpp
common/entities/service_instance/gc.hpp
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
common/runtime/entity_gc_registration.hpp
common/runtime/entity_gc_repository.hpp
common/runtime/entity_gc_types.hpp
common/runtime/entity_gc_workers.hpp
common/runtime/lease_store.hpp
common/runtime/queue_store.hpp
common/runtime/workflow_store.hpp
common/runtime_registration.hpp
common/schema_compatibility.hpp
common/shape_types.hpp
common/workflow.hpp
common/workflows/provision_service.hpp
worker/descriptors/catalog.hpp
worker/descriptors/provision_worker.hpp
worker/entity_gc_catalog.hpp
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
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors/types.hpp" "IExternalSystemOperatorMetadataApiHandler"
run_expect_status 0 "${CXX:-clang++}" -std=c++20 -Wall -Wextra -Wpedantic -I"$TMPDIR/out-e2e-cpp" -I"$TMPDIR/out-e2e-cpp/common" "$SCRIPT_DIR/api_process_fixture.cpp" -o "$TMPDIR/api-process-fixture-cpp"
run_expect_status 0 "$TMPDIR/api-process-fixture-cpp"

run_expect_status 0 "$CLI" generate bindings --lang cpp "$API_ENTITY_SPEC" --out "$TMPDIR/out-api-entities-cpp"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/worker/worker_descriptors.hpp"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/worker/worker_registry.hpp"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/Makefile" "check-worker"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/Makefile" "build-worker"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/account/registry.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/project/registry.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/task/handlers.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/task/registry.hpp"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "::statespec_generated::entities::account::DefaultAccountRepository repository"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp" "::statespec_generated::entities::project::DefaultProjectRepository repository"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/handlers.hpp" "::statespec_generated::entities::task::DefaultTaskRepository repository"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "backend_.begin()"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "repository.createTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "kAccountFieldCreatedAt"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "kAccountFieldUpdatedAt"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "kAccountFieldStatus"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "const auto create_timestamp = generated_api_timestamp()"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "document[::statespec_generated::entities::account::constants::kAccountFieldCreatedAt] = statespec::backend::Json{create_timestamp}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "document[::statespec_generated::entities::account::constants::kAccountFieldUpdatedAt] = statespec::backend::Json{create_timestamp}"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "document[::statespec_generated::entities::account::constants::kAccountFieldCreatedAt] = statespec::backend::Json{generated_api_timestamp()}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "document[::statespec_generated::entities::account::constants::kAccountFieldStatus] = statespec::backend::Json{::statespec_generated::entities::account::constants::kAccountStatusActive}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "decode_string(require_member(record.value().document, ::statespec_generated::entities::account::constants::kAccountFieldDisplayName), ::statespec_generated::entities::account::constants::kAccountFieldDisplayName)"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "body.emplace(::statespec_generated::entities::account::constants::kAccountFieldTenantId"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "body.emplace(\"tenant_id\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp" "missing required parent Account"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/handlers.hpp" "missing required parent Project"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "extract_api_path_parameters"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "repository.getTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "repository.listByTenantAccountTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp" "repository.listByAccountStatusTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/handlers.hpp" "repository.listByProjectStatusTx"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/common/descriptors/entities/account.hpp"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/common/descriptors/entities/project.hpp"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/common/descriptors/entities/task.hpp"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/project/persistence.hpp" "listByTenantProjectTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/task/persistence.hpp" "listByTenantTaskTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/task/persistence.hpp" "listByAccountPriorityTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/constants.hpp" "inline constexpr const char* kAccountEntityName = \"Account\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/constants.hpp" "inline constexpr const char* kAccountFieldTenantId = \"tenant_id\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/constants.hpp" "inline constexpr const char* kAccountFieldCreatedAtTypeName = \"timestamp\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/constants.hpp" "inline constexpr const char* kAccountFieldStatusTypeName = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/constants.hpp" "inline constexpr const char* kAccountIndexByTenantAccount = \"by_tenant_account\""
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/constants.hpp" "CreateAccountRequest"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/constants.hpp" "AccountResponse"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/constants.hpp" "decode_create_account_request"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/model.hpp" "inline constexpr const char* kAccountEntityName = \"Account\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/model.hpp" "FieldDescriptor{kAccountFieldCreatedAt, statespec::backend::FieldType::Timestamp, kAccountFieldCreatedAtTypeName, true}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/model.hpp" "FieldDescriptor{kAccountFieldStatus, statespec::backend::FieldType::String, kAccountFieldStatusTypeName, true}"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/account/constants.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/project/constants.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/task/constants.hpp"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/constants.hpp" "inline constexpr const char* kCreateAccountApiName = \"CreateAccount\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/constants.hpp" "inline constexpr const char* kEntityApiCreateAccountRouteName = \"EntityApi.CreateAccount\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/constants.hpp" "inline constexpr const char* kCreateAccountRequestShapeName = \"CreateAccountRequest\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/constants.hpp" "inline constexpr const char* kAccountListResponseEnvelopeName = ::statespec_generated::entities::account::constants::kAccountEntityPluralName"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/constants.hpp" "inline constexpr const char* kListAccountProjectsApiName = \"ListAccountProjects\""
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/account/shapes.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/account/catalog.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/project/shapes.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/project/codecs.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/project/catalog.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/task/shapes.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/task/codecs.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/task/catalog.hpp"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/shapes.hpp" "struct CreateAccountRequest"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/shapes.hpp" "entities::account::constants::kAccountEntityPluralName"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/shapes.hpp" "kListAccountsResponseFieldAccounts = \"accounts\""
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/shapes.hpp" "\"accounts\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "#include \"../../../common/entities/account/constants.hpp\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/codecs.hpp" "#include \"../../../common/entities/project/constants.hpp\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/codecs.hpp" "#include \"../../../common/entities/task/constants.hpp\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "decode_create_account_request"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "require_member(context.body, ::statespec_generated::entities::account::constants::kAccountFieldDisplayName)"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "body.emplace(::statespec_generated::entities::account::constants::kAccountFieldTenantId"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "body.emplace(::statespec_generated::entities::account::constants::kAccountFieldAccountId"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "require_member(context.body, ::statespec_generated::entities::account::constants::kAccountFieldStatus)"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/constants.hpp" "kAccountEntityPluralName = \"accounts\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "body.emplace(::statespec_generated::entities::account::constants::kAccountEntityPluralName"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/codecs.hpp" "require_member(context.body, ::statespec_generated::entities::project::constants::kProjectFieldAccountId)"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/project/constants.hpp" "kProjectEntityPluralName = \"projects\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/codecs.hpp" "body.emplace(::statespec_generated::entities::project::constants::kProjectFieldProjectId"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/codecs.hpp" "require_member(context.body, ::statespec_generated::entities::task::constants::kTaskFieldProjectId)"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/task/constants.hpp" "kTaskEntityPluralName = \"tasks\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/codecs.hpp" "require_member(context.body, ::statespec_generated::entities::task::constants::kTaskFieldPriority)"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "require_member(context.body, \"display_name\")"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "body.emplace(\"tenant_id\""
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "\"account_id\""
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "\"accounts\""
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/codecs.hpp" "\"account_id\""
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/codecs.hpp" "\"projects\""
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/codecs.hpp" "\"project_id\""
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/codecs.hpp" "\"tasks\""
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "common/descriptors.hpp"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "model.hpp"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/codecs.hpp" "common/descriptors.hpp"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/codecs.hpp" "model.hpp"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/codecs.hpp" "common/descriptors.hpp"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/codecs.hpp" "model.hpp"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/shapes.hpp" "struct AccountResponse"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/catalog.hpp" "shape_descriptors()"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/catalog.hpp" "api_descriptors()"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/catalog.hpp" "api_route_descriptors()"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/catalog.hpp" "handler_entrypoints()"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/catalog.hpp" "DefaultAccountApiHandlerRegistry"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/catalog.hpp" "#include \"handlers.hpp\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/registry.hpp" "register_handler_invokers"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/registry.hpp" "ApiHandlerLookupMap& handlers"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/registry.hpp" "statespec::backend::IBackend& backend"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/registry.hpp" "handlers.emplace(\"CreateAccount\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/registry.hpp" "handlers.emplace(\"ListAccountProjects\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/registry.hpp" "handlers.emplace(\"DeleteTask\""
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/catalog.hpp" "common/descriptors.hpp"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/catalog.hpp" "model.hpp"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/catalog.hpp" "handle_list_account_projects"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/catalog.hpp" "handle_list_account_projects"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/account/descriptors/create_account.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/project/descriptors/create_project.hpp"
assert_file_exists "$TMPDIR/out-api-entities-cpp/api/entities/task/descriptors/create_task.hpp"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/api/descriptors/create_account.hpp"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/api/descriptors/create_project.hpp"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/api/descriptors/create_task.hpp"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/descriptors/create_account.hpp" "#include \"../../../../common/entities/account/constants.hpp\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/descriptors/create_account.hpp" "std::string{\"/v1/tenants/{\"} + ::statespec_generated::entities::account::constants::kAccountFieldTenantId"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/descriptors/create_account.hpp" "common/descriptors.hpp"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/descriptors/create_account.hpp" "model.hpp"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/descriptors/create_project.hpp" "#include \"../../../../common/entities/project/constants.hpp\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/descriptors/create_project.hpp" "::statespec_generated::entities::project::constants::kProjectFieldProjectId"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/descriptors/create_task.hpp" "#include \"../../../../common/entities/task/constants.hpp\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/task/descriptors/create_task.hpp" "::statespec_generated::entities::task::constants::kTaskFieldTaskId"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-cpp/api/entities" "*.hpp" "common/descriptors.hpp"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-cpp/api/entities" "*.hpp" "model.hpp"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-cpp/api/entities" "catalog.hpp" "common/descriptors.hpp"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-cpp/api/entities" "catalog.hpp" "model.hpp"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/descriptors/catalog.hpp" "create_account_api_descriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/descriptors/catalog.hpp" "create_project_api_descriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/descriptors/catalog.hpp" "create_task_api_descriptors"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/api/shapes/create_account_request.hpp"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/shapes.hpp" "api_shape_descriptors"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/shapes.hpp" "api::entities::account::shape_descriptors"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/descriptors/catalog.hpp" "api::entities::account::api_descriptors"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/descriptors/catalog.hpp" "api::entities::project::api_route_descriptors"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/api_handler_registry.hpp" "register_handler_invokers([[maybe_unused]] ApiHandlerLookupMap& handlers)"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/api_handler_registry.hpp" "api::entities::account::register_handler_invokers(handlers)"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/api_handler_registry.hpp" "api::entities::project::register_handler_invokers(handlers)"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/api_handler_registry.hpp" "api::entities::task::register_handler_invokers(handlers)"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/api_dispatcher.hpp" "register_handler_invokers(map)"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/api_dispatcher.hpp" "handlers.emplace(\"CreateAccount\""
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/api_handler_registry.hpp" "handle_create_account"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/api_handlers.hpp" "IApiOperationHandler"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/shapes.hpp" "kCreateAccountRequestShapeName"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/shapes.hpp" "kAccountResponseShapeName"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/codecs.hpp" "decode_create_account_request"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/shapes.hpp" "kCreateAccountRequestFieldDisplayName = \"display_name\""
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/shapes.hpp" "FieldDescriptor{entities::account::constants::kAccountFieldDisplayName, statespec::backend::FieldType::String, entities::account::constants::kAccountFieldDisplayNameTypeName, true}"
assert_file_not_exists "$TMPDIR/out-api-entities-cpp/common/descriptors/shapes/create_account_request.hpp"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/account/schema.hpp" "{kAccountFieldTenantId, kAccountFieldAccountId}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entity_repository.hpp" "key += value->canonical_string()"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entity_repository.hpp" "object[key_value.field] = key_value.value"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "return ApiResponse{404"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/account/handlers.hpp" "statespec::backend::Json::array(std::move(items))"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp" "repository.updateTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp" "invalid entity status transition"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp" "document[::statespec_generated::entities::project::constants::kProjectFieldUpdatedAt]"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp" "const auto requested_status = std::string{::statespec_generated::entities::project::constants::kProjectStatusDeleted}"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp" "invalid entity delete transition"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp" "static bool project_status_transition_allowed"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp" "project_status_transition_allowed(current_status, requested_status)"
assert_file_not_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp" "const bool transition_allowed"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/entities/project/handlers.hpp" "return ApiResponse{204"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/project/persistence.hpp" "updateTx"
assert_file_contains "$TMPDIR/out-api-entities-cpp/common/entities/project/constants.hpp" "kProjectStatusDeleted"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/main.cpp" "../common/runtime/entity_gc_registration.hpp"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/main.cpp" "entity_gc_catalog.hpp"
assert_file_contains "$TMPDIR/out-api-entities-cpp/api/main.cpp" "api_entity_gc_descriptors()"
run_expect_status 0 make -C "$TMPDIR/out-api-entities-cpp" check-api
run_expect_status 0 "${CXX:-clang++}" -std=c++20 -Wall -Wextra -Wpedantic -I"$TMPDIR/out-api-entities-cpp" -I"$TMPDIR/out-api-entities-cpp/common" "$SCRIPT_DIR/api_persistence_fixture.cpp" -o "$TMPDIR/out-api-entities-cpp/build/api-persistence-fixture"
run_expect_status 0 "$TMPDIR/out-api-entities-cpp/build/api-persistence-fixture"

run_expect_status 0 "$CLI" generate bindings --lang cpp "$WORKFLOW_ENTITY_SPEC" --out "$TMPDIR/out-workflow-entities-cpp"
assert_file_not_exists "$TMPDIR/out-workflow-entities-cpp/api/main.cpp"
assert_file_contains "$TMPDIR/out-workflow-entities-cpp/worker/main.cpp" "../common/runtime/entity_gc_registration.hpp"
assert_file_contains "$TMPDIR/out-workflow-entities-cpp/worker/main.cpp" "entity_gc_catalog.hpp"
assert_file_contains "$TMPDIR/out-workflow-entities-cpp/worker/main.cpp" "worker_entity_gc_descriptors()"
run_expect_status 0 make -C "$TMPDIR/out-workflow-entities-cpp" check-worker

run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang cpp "$APP_SPEC" --out "$TMPDIR/out-app-cpp"
assert_file_manifest_equals "$TMPDIR/out-app-cpp" "$APP_MANIFEST"
assert_file_exists "$TMPDIR/out-app-cpp/common/descriptors.hpp"
assert_file_exists "$TMPDIR/out-app-cpp/api/api_descriptors.hpp"
assert_file_exists "$TMPDIR/out-app-cpp/worker/worker_descriptors.hpp"
assert_tree_files_not_contains "$TMPDIR/out-app-cpp/api/descriptors" "*.hpp" "common/descriptors.hpp"
assert_tree_files_not_contains "$TMPDIR/out-app-cpp/worker/descriptors" "*.hpp" "common/descriptors.hpp"
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
assert_file_contains "$TMPDIR/out-app-cpp/api/main.cpp" "api_entity_gc_descriptors()"
assert_file_contains "$TMPDIR/out-app-cpp/api/entity_gc_catalog.hpp" "service_instance_entity_gc_descriptor()"
assert_file_contains "$TMPDIR/out-app-cpp/common/entities/service_instance/gc.hpp" "inline ::statespec::backend::runtime::EntityGcDescriptor service_instance_entity_gc_descriptor()"
assert_file_contains "$TMPDIR/out-app-cpp/common/runtime/entity_gc_types.hpp" "struct EntityGcDescriptor"
assert_file_not_contains "$TMPDIR/out-app-cpp/common/runtime/entity_gc_types.hpp" "service_instance_entity_gc_descriptor"
assert_file_contains "$TMPDIR/out-app-cpp/common/runtime/entity_gc_registration.hpp" "for (const auto& descriptor : descriptors)"
assert_file_contains "$TMPDIR/out-app-cpp/api/codecs/start_provision_request.hpp" "decode_start_provision_request"
assert_file_contains "$TMPDIR/out-app-cpp/api/codecs/start_provision_response.hpp" "encode_start_provision_response"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_handlers.hpp" "class IBusinessApiOperationHandler"
assert_file_not_contains "$TMPDIR/out-app-cpp/api/api_handlers.hpp" "class IApiOperationHandler"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_handler_registry.hpp" "register_business_handler_invokers"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_handler_registry.hpp" "handler.handle_start_provision"
assert_file_not_contains "$TMPDIR/out-app-cpp/api/api_handler_registry.hpp" "class DefaultApiHandler"
assert_file_not_exists "$TMPDIR/out-app-cpp/api/entities/operations/handlers.hpp"
assert_file_contains "$TMPDIR/out-app-cpp/api/main.cpp" "ApiProcess"
assert_file_contains "$TMPDIR/out-app-cpp/api/main.cpp" "std::signal(SIGTERM"
assert_file_not_contains "$TMPDIR/out-app-cpp/api/main.cpp" "ApiApplication app"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_server.hpp" "class ApiServer"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_dispatcher.hpp" "dispatch_api_route"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_dispatcher.hpp" "api_route_descriptor_map"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_dispatcher.hpp" "api_handler_lookup_map"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_dispatcher.hpp" "ApiHandlerInvoker"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_dispatcher.hpp" "handler_it->second(backend, context)"
assert_file_contains "$TMPDIR/out-app-cpp/api/api_dispatcher.hpp" "business_handler_it->second(*business_handler, context)"
assert_file_not_contains "$TMPDIR/out-app-cpp/api/api_dispatcher.hpp" "route->api_name =="
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
assert_file_contains "$TMPDIR/out-app-cpp/worker/main.cpp" "worker_entity_gc_descriptors()"
assert_file_contains "$TMPDIR/out-app-cpp/worker/entity_gc_catalog.hpp" "service_instance_entity_gc_descriptor()"
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
