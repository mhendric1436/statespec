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
api/codecs/provision_callback_request.rs
api/codecs/provision_callback_response.rs
api/codecs/start_provision_request.rs
api/codecs/start_provision_response.rs
api/descriptors/catalog.rs
api/descriptors/report_provision_ready.rs
api/descriptors/start_provision.rs
api/entity_gc_catalog.rs
api/external_system_operator_metadata_api.rs
api/main.rs
api/shapes.rs
api/shapes/provision_callback_request.rs
api/shapes/provision_callback_response.rs
api/shapes/start_provision_request.rs
api/shapes/start_provision_response.rs
common/backend.rs
common/descriptors.rs
common/descriptors/core.rs
common/descriptors/events.rs
common/descriptors/external_systems.rs
common/descriptors/runtime.rs
common/descriptors/runtime/leases.rs
common/descriptors/runtime/queues.rs
common/descriptors/runtime/workflows.rs
common/descriptors/shapes.rs
common/descriptors/shapes/provision_callback_request.rs
common/descriptors/shapes/provision_callback_response.rs
common/descriptors/shapes/start_provision_request.rs
common/descriptors/shapes/start_provision_response.rs
common/descriptors/types.rs
common/entities/service_instance/gc.rs
common/entities/service_instance/mod.rs
common/entities/service_instance/model.rs
common/entities/service_instance/persistence.rs
common/entities/service_instance/schema.rs
common/entity_repository.rs
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
common/runtime/entity_gc_registration.rs
common/runtime/entity_gc_repository.rs
common/runtime/entity_gc_types.rs
common/runtime/entity_gc_workers.rs
common/runtime/leases.rs
common/runtime/queues.rs
common/runtime/workflows.rs
common/runtime_registration.rs
common/schema_compatibility.rs
common/shape_types.rs
common/workflow.rs
common/workflows/provision_service.rs
lib.rs
worker/descriptors/catalog.rs
worker/descriptors/provision_worker.rs
worker/entity_gc_catalog.rs
worker/main.rs
worker/registry/provision_worker.rs
worker/worker_application.rs
worker/worker_contexts.rs
worker/worker_descriptors.rs
worker/worker_leases.rs
worker/worker_process.rs
worker/worker_queues.rs
worker/worker_registry.rs
worker/worker_runtime.rs
worker/worker_workflows.rs
worker/workflow_runner.rs
worker/workflow_step_handlers.rs
worker/workflows/provision_service.rs
EOF

run_expect_status 0 "$CLI" generate bindings --lang rust "$E2E_SPEC" --out "$TMPDIR/out-e2e-rust"
assert_file_contains "$TMPDIR/out-e2e-rust/api/descriptors/catalog.rs" "pub fn api_route_descriptors"
assert_file_contains "$TMPDIR/out-e2e-rust/api/descriptors/catalog.rs" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-rust/api/descriptors/catalog.rs" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors/external_systems.rs" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors/external_systems.rs" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "ExternalSystemOperatorMetadataApiHandler"
mkdir -p "$TMPDIR/out-e2e-rust/tests"
cp "$SCRIPT_DIR/api_process_fixture.rs" "$TMPDIR/out-e2e-rust/tests/api_process_fixture.rs"
run_expect_status 0 make -C "$TMPDIR/out-e2e-rust" check-api

run_expect_status 0 "$CLI" generate bindings --lang rust "$API_ENTITY_SPEC" --out "$TMPDIR/out-api-entities-rust"
assert_file_not_exists "$TMPDIR/out-api-entities-rust/worker/worker_descriptors.rs"
assert_file_not_exists "$TMPDIR/out-api-entities-rust/worker/worker_registry.rs"
assert_file_not_contains "$TMPDIR/out-api-entities-rust/Makefile" "check-worker"
assert_file_not_contains "$TMPDIR/out-api-entities-rust/Makefile" "build-worker"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "let repository = crate::entity_account::persistence::DefaultAccountRepository;"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/project.rs" "let repository = crate::entity_project::persistence::DefaultProjectRepository;"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/task.rs" "let repository = crate::entity_task::persistence::DefaultTaskRepository;"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "self.backend.begin()"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "create_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "ACCOUNT_FIELD_CREATED_AT"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "ACCOUNT_FIELD_UPDATED_AT"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "ACCOUNT_FIELD_STATUS"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/api_handler_registry.rs" "pub(crate) fn generated_api_timestamp() -> String"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "document.insert(crate::entity_account::model::ACCOUNT_FIELD_CREATED_AT.to_string(), Json::String(generated_api_timestamp()))"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "document.insert(crate::entity_account::model::ACCOUNT_FIELD_STATUS.to_string(), Json::String(crate::entity_account::model::ACCOUNT_STATUS_ACTIVE.to_string()))"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/project.rs" "missing required parent Account"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/task.rs" "missing required parent Project"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "extract_api_path_parameters"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "get_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "list_by_tenant_account_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "list_by_account_status_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/project.rs" "list_by_project_status_tx"
assert_file_not_exists "$TMPDIR/out-api-entities-rust/common/descriptors/entities/account.rs"
assert_file_not_exists "$TMPDIR/out-api-entities-rust/common/descriptors/entities/project.rs"
assert_file_not_exists "$TMPDIR/out-api-entities-rust/common/descriptors/entities/task.rs"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/project/persistence.rs" "list_by_tenant_project_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/task/persistence.rs" "list_by_tenant_task_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/task/persistence.rs" "list_by_account_priority_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/account/model.rs" "pub const ACCOUNT_ENTITY_NAME: &str = \"Account\""
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/account/model.rs" "pub const ACCOUNT_FIELD_TENANT_ID: &str = \"tenant_id\""
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/account/model.rs" "pub const ACCOUNT_FIELD_CREATED_AT_TYPE_NAME: &str = \"timestamp\""
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/account/model.rs" "pub const ACCOUNT_FIELD_STATUS_TYPE_NAME: &str = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/account/model.rs" "pub const ACCOUNT_INDEX_BY_TENANT_ACCOUNT: &str = \"by_tenant_account\""
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/account/model.rs" "FieldDescriptor { name: ACCOUNT_FIELD_CREATED_AT.to_string(), field_type: FieldType::Timestamp, type_name: ACCOUNT_FIELD_CREATED_AT_TYPE_NAME.to_string(), required: true }"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/account/model.rs" "FieldDescriptor { name: ACCOUNT_FIELD_STATUS.to_string(), field_type: FieldType::String, type_name: ACCOUNT_FIELD_STATUS_TYPE_NAME.to_string(), required: true }"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/descriptors/shapes/create_account_request.rs" "pub const CREATE_ACCOUNT_REQUEST_SHAPE_NAME: &str = \"CreateAccountRequest\""
assert_file_contains "$TMPDIR/out-api-entities-rust/common/descriptors/shapes/create_account_request.rs" "pub const CREATE_ACCOUNT_REQUEST_FIELD_DISPLAY_NAME: &str = \"display_name\""
assert_file_contains "$TMPDIR/out-api-entities-rust/common/descriptors/shapes/create_account_request.rs" "pub const CREATE_ACCOUNT_REQUEST_FIELD_DISPLAY_NAME_TYPE_NAME: &str = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-rust/common/descriptors/shapes/create_account_request.rs" "FieldDescriptor { name: CREATE_ACCOUNT_REQUEST_FIELD_DISPLAY_NAME.to_string(), field_type: FieldType::String, type_name: CREATE_ACCOUNT_REQUEST_FIELD_DISPLAY_NAME_TYPE_NAME.to_string(), required: true }"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/account/schema.rs" "key_fields: vec![ACCOUNT_FIELD_TENANT_ID.to_string(), ACCOUNT_FIELD_ACCOUNT_ID.to_string()]"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entity_repository.rs" "key.push_str(&value.value.canonical_string())"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entity_repository.rs" "values.insert(key_value.field.clone(), key_value.value.clone())"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "status_code: 404"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/account.rs" "Json::Array(records.into_iter().map"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/project.rs" "update_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/project.rs" "invalid entity status transition"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/project.rs" "document.insert(crate::entity_project::model::PROJECT_FIELD_UPDATED_AT.to_string()"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/project.rs" "let requested_status = crate::entity_project::model::PROJECT_STATUS_DELETED.to_string()"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/project.rs" "invalid entity delete transition"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/handlers/project.rs" "status_code: 204"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/project/persistence.rs" "update_tx"
assert_file_contains "$TMPDIR/out-api-entities-rust/common/entities/project/model.rs" "PROJECT_STATUS_DELETED"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/main.rs" "use crate::api_entity_gc_catalog::entity_gc_descriptors"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/main.rs" "register_entity_gc_workers"
assert_file_contains "$TMPDIR/out-api-entities-rust/api/main.rs" "api_entity_gc_descriptors()"
mkdir -p "$TMPDIR/out-api-entities-rust/tests"
cp "$SCRIPT_DIR/api_persistence_fixture.rs" "$TMPDIR/out-api-entities-rust/tests/api_persistence_fixture.rs"
run_expect_status 0 make -C "$TMPDIR/out-api-entities-rust" check-api

run_expect_status 0 "$CLI" generate bindings --lang rust "$WORKFLOW_ENTITY_SPEC" --out "$TMPDIR/out-workflow-entities-rust"
assert_file_not_exists "$TMPDIR/out-workflow-entities-rust/api/main.rs"
assert_file_contains "$TMPDIR/out-workflow-entities-rust/worker/main.rs" "use crate::runtime_entity_gc_registration::register_entity_gc_workers"
assert_file_contains "$TMPDIR/out-workflow-entities-rust/worker/main.rs" "use crate::worker_entity_gc_catalog::entity_gc_descriptors"
assert_file_contains "$TMPDIR/out-workflow-entities-rust/worker/main.rs" "worker_entity_gc_descriptors()"

run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang rust "$APP_SPEC" --out "$TMPDIR/out-app-rust"
assert_file_manifest_equals "$TMPDIR/out-app-rust" "$APP_MANIFEST"
assert_file_exists "$TMPDIR/out-app-rust/common/descriptors.rs"
assert_file_exists "$TMPDIR/out-app-rust/api/api_descriptors.rs"
assert_file_exists "$TMPDIR/out-app-rust/worker/worker_descriptors.rs"
assert_tree_files_not_contains "$TMPDIR/out-app-rust/api/descriptors" "*.rs" "crate::descriptors"
assert_tree_files_not_contains "$TMPDIR/out-app-rust/worker/descriptors" "*.rs" "crate::descriptors"
assert_file_contains "$TMPDIR/out-app-rust/api/descriptors/start_provision.rs" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-rust/worker/descriptors/provision_worker.rs" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-rust/worker/descriptors/provision_worker.rs" "\"ProvisionWorker\""
assert_file_contains "$TMPDIR/out-app-rust/api/api_application.rs" "pub struct ApiApplication"
assert_file_contains "$TMPDIR/out-app-rust/api/codecs/start_provision_request.rs" "decode_start_provision_request"
assert_file_contains "$TMPDIR/out-app-rust/api/codecs/start_provision_response.rs" "encode_start_provision_response"
assert_file_contains "$TMPDIR/out-app-rust/api/api_handler_registry.rs" "pub struct DefaultApiHandler"
assert_file_contains "$TMPDIR/out-app-rust/api/api_handlers.rs" "pub trait BusinessApiHandler"
assert_file_contains "$TMPDIR/out-app-rust/api/api_handler_registry.rs" "business_handler: Option<std::sync::Arc<dyn BusinessApiHandler + Send + Sync>>"
assert_file_contains "$TMPDIR/out-app-rust/api/api_handler_registry.rs" "handler.handle_start_provision"
assert_file_not_exists "$TMPDIR/out-app-rust/api/handlers/operations.rs"
assert_file_contains "$TMPDIR/out-app-rust/api/main.rs" "ApiProcessConfig::all_servers"
assert_file_contains "$TMPDIR/out-app-rust/api/main.rs" "register_entity_gc_workers"
assert_file_contains "$TMPDIR/out-app-rust/api/main.rs" "install_signal_handling"
assert_file_not_contains "$TMPDIR/out-app-rust/api/main.rs" "ApiApplication::new_default"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "pub struct ApiProcess"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "pub fn request_stop(&self)"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "pub fn start(self: &Arc<Self>)"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "pub fn join(&self)"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "pub fn is_running(&self)"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "pub entity_gc_enabled: bool"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "pub fn add_entity_gc_worker"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "fn start_entity_gc_workers"
assert_file_contains "$TMPDIR/out-app-rust/api/api_transport.rs" "pub trait ApiTransport"
assert_file_contains "$TMPDIR/out-app-rust/api/api_transport.rs" "server.handle(route_name, context)"
assert_file_contains "$TMPDIR/out-app-rust/api/api_transport.rs" "pub struct LocalBlockingApiTransport"
assert_file_contains "$TMPDIR/out-app-rust/api/api_transport.rs" "fn request_stop(&self)"
assert_file_contains "$TMPDIR/out-app-rust/api/api_process.rs" "self.transport.request_stop()"
assert_file_contains "$TMPDIR/out-app-rust/api/main.rs" "LocalBlockingApiTransport::new()"
assert_file_contains "$TMPDIR/out-app-rust/api/main.rs" "api_entity_gc_descriptors()"
assert_file_contains "$TMPDIR/out-app-rust/api/entity_gc_catalog.rs" "service_instance_entity_gc_descriptor()"
assert_file_contains "$TMPDIR/out-app-rust/common/entities/service_instance/gc.rs" "pub fn service_instance_entity_gc_descriptor() -> crate::runtime_entity_gc_types::EntityGcDescriptor"
assert_file_contains "$TMPDIR/out-app-rust/common/runtime/entity_gc_types.rs" "pub struct EntityGcDescriptor"
assert_file_not_contains "$TMPDIR/out-app-rust/common/runtime/entity_gc_types.rs" "service_instance_entity_gc_descriptor"
assert_file_contains "$TMPDIR/out-app-rust/common/runtime/entity_gc_registration.rs" "for descriptor in descriptors"
assert_file_contains "$TMPDIR/out-app-rust/api/main.rs" "process.start()"
assert_file_contains "$TMPDIR/out-app-rust/api/main.rs" "process.join()"
assert_file_contains "$TMPDIR/out-app-rust/api/api_server.rs" "pub struct ApiServer"
assert_file_contains "$TMPDIR/out-app-rust/api/api_dispatcher.rs" "pub fn dispatch_api_route"
assert_file_not_contains "$TMPDIR/out-app-rust/api/api_dispatcher.rs" "pub fn dispatch_api_operation_route"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_registry.rs" "pub fn find_worker_descriptor"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_runtime.rs" "pub struct WorkerRuntime"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_runtime.rs" "pub struct WorkerRuntimeConfig"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_runtime.rs" "pub fn start_entity_gc_workers"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_runtime.rs" "pub fn request_stop"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_process.rs" "pub struct WorkerProcess"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_process.rs" "pub fn start(self: &Arc<Self>)"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_process.rs" "pub fn join(&self)"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_process.rs" "pub fn is_running(&self)"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_process.rs" "pub fn request_stop(&self)"
assert_file_contains "$TMPDIR/out-app-rust/worker/worker_process.rs" "fn start_worker_loops"
assert_file_contains "$TMPDIR/out-app-rust/worker/main.rs" "WorkerProcess::new"
assert_file_contains "$TMPDIR/out-app-rust/worker/main.rs" "DefaultWorkflowStepHandler"
assert_file_contains "$TMPDIR/out-app-rust/worker/main.rs" "register_entity_gc_workers"
assert_file_contains "$TMPDIR/out-app-rust/worker/main.rs" "worker_entity_gc_descriptors()"
assert_file_contains "$TMPDIR/out-app-rust/worker/entity_gc_catalog.rs" "service_instance_entity_gc_descriptor()"
assert_file_contains "$TMPDIR/out-app-rust/worker/main.rs" "install_signal_handling"
assert_file_contains "$TMPDIR/out-app-rust/worker/main.rs" "process.start()"
assert_file_contains "$TMPDIR/out-app-rust/worker/main.rs" "process.join()"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_runner.rs" "keep_alive_step"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_step_handlers.rs" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_step_handlers.rs" "pub struct DefaultWorkflowStepHandler"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_step_handlers.rs" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_step_handlers.rs" "\"ProvisionService.wait_for_remote_service\""
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_step_handlers.rs" "handle_provision_service_validate_request"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflow_runner.rs" "workflow_provision_service::dispatch_step"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflows/provision_service.rs" "handle_provision_service_validate_request"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflows/provision_service.rs" "Some(\"create_remote_service\".to_string())"
assert_file_contains "$TMPDIR/out-app-rust/worker/workflows/provision_service.rs" "Some(\"wait_for_remote_service\".to_string())"
mkdir -p "$TMPDIR/out-app-rust/tests"
cp "$SCRIPT_DIR/api_linking_fixture.rs" "$TMPDIR/out-app-rust/tests/api_linking_fixture.rs"
cp "$SCRIPT_DIR/registration_restart_fixture.rs" "$TMPDIR/out-app-rust/tests/registration_restart_fixture.rs"
cp "$SCRIPT_DIR/worker_linking_fixture.rs" "$TMPDIR/out-app-rust/tests/worker_linking_fixture.rs"
cp "$SCRIPT_DIR/worker_process_fixture.rs" "$TMPDIR/out-app-rust/tests/worker_process_fixture.rs"
run_expect_status 0 make -C "$TMPDIR/out-app-rust" check
