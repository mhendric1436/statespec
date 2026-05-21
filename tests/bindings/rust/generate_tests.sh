#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TESTS_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
. "$TESTS_DIR/cli/common.sh"

SPEC="$TESTS_DIR/fixtures/bindings-full.sspec"

# Positive generation: Rust.
run_expect_status 0 "$CLI" generate bindings --lang rust "$SPEC" --out "$TMPDIR/out-rust"
assert_output_contains "generated $TMPDIR/out-rust/common/backend.rs"
assert_file_exists "$TMPDIR/out-rust/common/json.rs"
assert_file_exists "$TMPDIR/out-rust/common/backend.rs"
assert_file_exists "$TMPDIR/out-rust/common/external_system.rs"
assert_file_exists "$TMPDIR/out-rust/common/feature_flag.rs"
assert_file_exists "$TMPDIR/out-rust/common/lease.rs"
assert_file_exists "$TMPDIR/out-rust/common/log.rs"
assert_file_exists "$TMPDIR/out-rust/common/metric.rs"
assert_file_exists "$TMPDIR/out-rust/common/queue.rs"
assert_file_exists "$TMPDIR/out-rust/common/workflow.rs"
assert_file_exists "$TMPDIR/out-rust/common/memory/backend.rs"
assert_file_exists "$TMPDIR/out-rust/common/memory/transaction.rs"
assert_file_exists "$TMPDIR/out-rust/common/memory/codec.rs"
assert_file_exists "$TMPDIR/out-rust/common/memory/feature_flags.rs"
assert_file_exists "$TMPDIR/out-rust/common/memory/queues.rs"
assert_file_exists "$TMPDIR/out-rust/common/memory/leases.rs"
assert_file_exists "$TMPDIR/out-rust/common/memory/workflows.rs"
assert_file_exists "$TMPDIR/out-rust/common/memory/logs.rs"
assert_file_exists "$TMPDIR/out-rust/common/memory/metrics.rs"
assert_file_exists "$TMPDIR/out-rust/common/descriptors.rs"
assert_file_exists "$TMPDIR/out-rust/Cargo.toml"
assert_file_exists "$TMPDIR/out-rust/Makefile"
assert_file_exists "$TMPDIR/out-rust/lib.rs"
assert_file_exists "$TMPDIR/out-rust/api/api_descriptors.rs"
assert_file_exists "$TMPDIR/out-rust/api/api_handlers.rs"
assert_file_exists "$TMPDIR/out-rust/api/api_dispatcher.rs"
assert_file_exists "$TMPDIR/out-rust/api/api_server.rs"
assert_file_exists "$TMPDIR/out-rust/api/api_routes.rs"
assert_file_exists "$TMPDIR/out-rust/api/external_system_operator_metadata_api.rs"
assert_file_exists "$TMPDIR/out-rust/worker/worker_descriptors.rs"
assert_file_exists "$TMPDIR/out-rust/worker/worker_contexts.rs"
assert_file_exists "$TMPDIR/out-rust/worker/worker_registry.rs"
assert_file_exists "$TMPDIR/out-rust/worker/worker_application.rs"
assert_file_exists "$TMPDIR/out-rust/worker/workflow_step_handlers.rs"
assert_file_exists "$TMPDIR/out-rust/worker/workflow_runner.rs"
assert_file_exists "$TMPDIR/out-rust/worker/worker_handlers.rs"
assert_file_exists "$TMPDIR/out-rust/worker/worker_queues.rs"
assert_file_exists "$TMPDIR/out-rust/worker/worker_leases.rs"
assert_file_exists "$TMPDIR/out-rust/worker/worker_workflows.rs"
assert_file_contains "$TMPDIR/out-rust/Cargo.toml" "name = \"statespec-generated\""
assert_file_contains "$TMPDIR/out-rust/Cargo.toml" "path = \"lib.rs\""
assert_file_contains "$TMPDIR/out-rust/Makefile" "CARGO ?= cargo"
assert_file_contains "$TMPDIR/out-rust/Makefile" "build-api"
assert_file_contains "$TMPDIR/out-rust/Makefile" "build-worker"
assert_file_contains "$TMPDIR/out-rust/Makefile" "package-api"
assert_file_contains "$TMPDIR/out-rust/Makefile" "package-worker"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod descriptors;"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod api_descriptors;"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod api_handlers;"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod api_dispatcher;"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod api_server;"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod api_routes;"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod worker_descriptors;"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod worker_contexts;"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod worker_registry;"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod worker_application;"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod workflow_step_handlers;"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod workflow_runner;"
assert_file_contains "$TMPDIR/out-rust/lib.rs" "pub mod worker_workflows;"
assert_file_contains "$TMPDIR/out-rust/api/api_descriptors.rs" "pub use crate::descriptors::{ApiDescriptor, ApiServerDescriptor}"
assert_file_contains "$TMPDIR/out-rust/api/api_descriptors.rs" "pub fn api_descriptors"
assert_file_contains "$TMPDIR/out-rust/api/api_handlers.rs" "pub use crate::descriptors::{ApiHandler, ApiRequestContext, ApiResponse}"
assert_file_contains "$TMPDIR/out-rust/api/api_dispatcher.rs" "pub fn find_api_route"
assert_file_contains "$TMPDIR/out-rust/api/api_dispatcher.rs" "pub fn dispatch_api_route"
assert_file_contains "$TMPDIR/out-rust/api/api_server.rs" "pub struct ApiServer"
assert_file_contains "$TMPDIR/out-rust/api/api_server.rs" "pub fn find_api_server"
assert_file_contains "$TMPDIR/out-rust/api/api_routes.rs" "pub fn api_route_descriptors"
assert_file_contains "$TMPDIR/out-rust/api/external_system_operator_metadata_api.rs" "ExternalSystemOperatorMetadataApiHandler"
assert_file_contains "$TMPDIR/out-rust/worker/worker_descriptors.rs" "pub fn worker_descriptors"
assert_file_contains "$TMPDIR/out-rust/worker/worker_contexts.rs" "pub fn worker_contexts"
assert_file_contains "$TMPDIR/out-rust/worker/worker_registry.rs" "pub fn find_worker_descriptor"
assert_file_contains "$TMPDIR/out-rust/worker/worker_application.rs" "pub struct WorkerApplication"
assert_file_contains "$TMPDIR/out-rust/worker/workflow_step_handlers.rs" "pub trait WorkflowStepHandler"
assert_file_contains "$TMPDIR/out-rust/worker/workflow_step_handlers.rs" "pub fn workflow_step_handler_keys"
assert_file_contains "$TMPDIR/out-rust/worker/workflow_runner.rs" "pub struct WorkflowRunner"
assert_file_contains "$TMPDIR/out-rust/worker/workflow_runner.rs" "keep_alive_step"
assert_file_contains "$TMPDIR/out-rust/worker/worker_handlers.rs" "pub use crate::descriptors::Worker"
assert_file_contains "$TMPDIR/out-rust/worker/worker_queues.rs" "pub fn queue_definitions"
assert_file_contains "$TMPDIR/out-rust/worker/worker_leases.rs" "pub fn lease_definitions"
assert_file_contains "$TMPDIR/out-rust/worker/worker_workflows.rs" "pub fn workflow_definitions"
assert_file_contains "$TMPDIR/out-rust/common/backend.rs" "pub enum FieldType"
assert_file_contains "$TMPDIR/out-rust/common/backend.rs" "pub field_type: FieldType"
assert_file_contains "$TMPDIR/out-rust/common/backend.rs" "pub type_name: String"
assert_file_contains "$TMPDIR/out-rust/common/workflow.rs" "pub struct KeepAliveWorkflowStepRequest"
assert_file_contains "$TMPDIR/out-rust/common/workflow.rs" "fn keep_alive_step_tx"
assert_file_contains "$TMPDIR/out-rust/common/memory/backend.rs" "pub struct InMemoryBackend"
assert_file_contains "$TMPDIR/out-rust/common/memory/transaction.rs" "pub struct InMemoryTransaction"
assert_file_contains "$TMPDIR/out-rust/common/memory/codec.rs" "feature_flag_definition_to_json"
assert_file_contains "$TMPDIR/out-rust/common/memory/feature_flags.rs" "pub struct InMemoryFeatureFlagStore"
assert_file_contains "$TMPDIR/out-rust/common/memory/queues.rs" "pub struct InMemoryQueueStore"
assert_file_contains "$TMPDIR/out-rust/common/memory/leases.rs" "pub struct InMemoryLeaseStore"
assert_file_contains "$TMPDIR/out-rust/common/memory/workflows.rs" "pub struct InMemoryWorkflowStore"
assert_file_contains "$TMPDIR/out-rust/common/memory/logs.rs" "inspect_events"
assert_file_contains "$TMPDIR/out-rust/common/memory/metrics.rs" "inspect_samples"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct FeatureFlagDefinition"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn feature_flag_definitions() -> Vec<FeatureFlagDefinition>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ShapeDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn shape_descriptors() -> Vec<ShapeDescriptor>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub tenant_id: String"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ValueDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn value_descriptors() -> Vec<ValueDescriptor>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct EnumDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn enum_descriptors() -> Vec<EnumDescriptor>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct EventDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn event_descriptors() -> Vec<EventDescriptor>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct EventEnvelope"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn make_order_accepted_event"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ExternalSystemDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ExternalSystemMetadataDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ExternalSystemMetadataMappingDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ExternalSystemMetadataMappingPlan"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ExternalSystemOperatorMetadataUpsertRequest"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub trait ExternalSystemOperatorMetadataRepository"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ExternalSystemMetadataMappingInputs"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ExternalSystemMetadataMissingMappingSource"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn missing_external_system_metadata_mapping_sources"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn source_value"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub trait ExternalSystemMetadataMappingApplicator"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn external_system_metadata_mapping_plan"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub tenant_field: Option<String>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub key_fields: Vec<String>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "tenant_field: Some(\"tenant_id\".to_string())"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "source: \"metadata.base_url\".to_string()"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "target: \"client.base_url\".to_string()"
assert_file_contains "$TMPDIR/out-rust/common/external_system.rs" "pub trait ExternalSystemMetadataResolver"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn external_system_metadata_lookup"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn external_system_metadata_lookup_by_name"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn resolve_external_system_metadata_tx"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn resolve_external_system_metadata_by_name_tx"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "key_complete()"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn external_system_descriptors() -> Vec<ExternalSystemDescriptor>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ApiDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn api_descriptors() -> Vec<ApiDescriptor>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn api_server_descriptors() -> Vec<ApiServerDescriptor>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ApiRequestContext"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct ApiResponse"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub trait ApiHandler"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub trait ExternalSystemOperatorMetadataApiHandler"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn api_route_descriptors() -> Vec<ApiRouteDescriptor>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "name: \"OrderApi\".to_string()"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "serves: vec![\"StartOrderProcessing\".to_string()]"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "name: \"OrderApi.StartOrderProcessing\".to_string()"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "server_name: \"OrderApi\".to_string()"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct WorkerDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct WorkerContext"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub trait Worker"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn worker_descriptors() -> Vec<WorkerDescriptor>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn worker_contexts() -> Vec<WorkerContext>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "name: \"OrderWorker\".to_string()"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "polls: Some(\"EmailDispatch.SendConfirmation\".to_string())"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct PolicyDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn policy_descriptors() -> Vec<PolicyDescriptor>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct GarbageCollectionPolicy"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct EntityStateDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct EntityOwnershipDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct EntityRelationDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct EntityChildDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct EntityInvariantDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn entity_descriptors() -> Vec<EntityDescriptor>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn collection_descriptors() -> Vec<CollectionDescriptor>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "IndexDescriptor"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn queue_definitions() -> Vec<QueueDefinition>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn lease_definitions() -> Vec<LeaseDefinition>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn workflow_definitions() -> Vec<WorkflowDefinition>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "EmailDispatch"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "OrderReconciler"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "workflow_version: 2"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "validate_order"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "NewScheduler"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "OrderAmount"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "OrderStatus"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "OrderAccepted"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "Stripe"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "StartOrderProcessing"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "path: Some(\"/v1/tenants/{tenantId}/orders/{order_id}/start\".to_string())"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "OrderAccess"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct LogDefinition"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn log_definitions() -> Vec<LogDefinition>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "event_name: \"workflow.launch.decision\".to_string()"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub struct MetricDefinition"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn metric_definitions() -> Vec<MetricDefinition>"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn ensure_system_collections"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn register_feature_flag_definitions_tx"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn register_queue_definitions_tx"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn register_lease_definitions_tx"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn register_runtime_catalog_tx"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn register_observability_catalog_tx"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "pub fn register_workflow_definitions_tx"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "backend_name: \"workflow_launch_attempts_total\".to_string()"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "relation_kind: Some(\"composition\".to_string())"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "name: \"valid_status\".to_string()"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "name: \"by_tenant_status\".to_string()"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "unique: true"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "garbage_collection: Some(GarbageCollectionPolicy { after: \"P30D\".to_string(), mode: \"tombstone\".to_string() })"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "field_type: FieldType::String"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "field_type: FieldType::Named"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "field_type: FieldType::Timestamp"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "field_type: FieldType::Optional"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "type_name: \"OrderStatus\".to_string()"
assert_file_contains "$TMPDIR/out-rust/common/descriptors.rs" "type_name: \"int?\".to_string()"
mkdir -p "$TMPDIR/out-rust/tests"
cp "$SCRIPT_DIR/metadata_resolver_fixture.rs" "$TMPDIR/out-rust/tests/metadata_resolver_fixture.rs"
cat > "$TMPDIR/out-rust/tests/memory_backend_fixture.rs" <<'EOF'
use std::collections::BTreeMap;
use std::time::{Duration, SystemTime};

use statespec_generated::backend::{Backend, Query};
use statespec_generated::feature_flag::{
    FeatureFlagDefinition, FeatureFlagEvaluationContext, FeatureFlagEvaluationRequest,
    FeatureFlagScopeKind, FeatureFlagStore, FeatureFlagType, FeatureFlagValue,
};
use statespec_generated::json::Json;
use statespec_generated::lease::{
    LeaseAcquireRequest, LeaseDefinition, LeaseDefinitionId, LeaseStore,
};
use statespec_generated::log::{LogDefinition, LogEvent, LogLevel, LogSink};
use statespec_generated::memory_backend::InMemoryBackend;
use statespec_generated::memory_feature_flags::InMemoryFeatureFlagStore;
use statespec_generated::memory_leases::InMemoryLeaseStore;
use statespec_generated::memory_logs::InMemoryLogSink;
use statespec_generated::memory_metrics::InMemoryMetricSink;
use statespec_generated::memory_queues::InMemoryQueueStore;
use statespec_generated::memory_workflows::InMemoryWorkflowStore;
use statespec_generated::metric::{MetricDefinition, MetricKind, MetricSample, MetricSink};
use statespec_generated::queue::{
    ClaimMessageRequest, EnqueueMessageRequest, QueueDefinition, QueueStore,
    RegisterQueueDefinitionRequest,
};
use statespec_generated::workflow::{
    ClaimWorkflowStepRequest, KeepAliveWorkflowStepRequest, RegisterWorkflowDefinitionRequest,
    StartWorkflowRequest, WorkflowDefinition, WorkflowStepDefinition, WorkflowStore,
};

#[test]
fn memory_backend_stores_compose_in_transaction() {
    let backend = InMemoryBackend::new();
    let flags = InMemoryFeatureFlagStore::new();
    let queues = InMemoryQueueStore::new();
    let leases = InMemoryLeaseStore::new();
    let workflows = InMemoryWorkflowStore::new();
    let logs = InMemoryLogSink::new();
    let metrics = InMemoryMetricSink::new();

    let mut tx = backend.begin().unwrap();
    <InMemoryFeatureFlagStore as FeatureFlagStore<InMemoryBackend>>::register_definition_tx(
        &flags,
        &mut tx,
        &FeatureFlagDefinition {
            name: "new_scheduler".to_string(),
            flag_type: FeatureFlagType::Bool,
            default_value: FeatureFlagValue::Bool(true),
            scope: FeatureFlagScopeKind::System,
            owner: None,
            description: None,
            expires: None,
        },
    )
    .unwrap();
    <InMemoryQueueStore as QueueStore<InMemoryBackend>>::register_definition_tx(
        &queues,
        &mut tx,
        &RegisterQueueDefinitionRequest {
            definition: QueueDefinition {
                queue: "workflow".to_string(),
                channel: "launch".to_string(),
                visibility_timeout: Duration::from_secs(60),
                max_attempts: 3,
                dead_letter_queue: None,
                metadata: Json::Object(BTreeMap::new()),
            },
        },
    )
    .unwrap();
    let lease_id = LeaseDefinitionId {
        name: "workflow_step".to_string(),
        version: 1,
    };
    <InMemoryLeaseStore as LeaseStore<InMemoryBackend>>::register_definition_tx(
        &leases,
        &mut tx,
        &LeaseDefinition {
            id: lease_id.clone(),
            resource_pattern: "workflow/*".to_string(),
            ttl: Duration::from_secs(60),
            renew_every: Duration::from_secs(30),
            max_ttl: None,
            fencing_token: true,
        },
    )
    .unwrap();
    <InMemoryWorkflowStore as WorkflowStore<InMemoryBackend>>::register_definition_tx(
        &workflows,
        &mut tx,
        &RegisterWorkflowDefinitionRequest {
            definition: WorkflowDefinition {
                workflow_name: "ProvisionService".to_string(),
                workflow_version: 1,
                start_step: "validate_request".to_string(),
                expected_execution_time: Duration::from_secs(60),
                singleton: false,
                steps: vec![WorkflowStepDefinition {
                    name: "validate_request".to_string(),
                    expected_execution_time: Duration::from_secs(1),
                    max_retries: 3,
                }],
                metadata: Json::Object(BTreeMap::new()),
            },
        },
    )
    .unwrap();
    <InMemoryLogSink as LogSink<InMemoryBackend>>::register_definition_tx(
        &logs,
        &mut tx,
        &LogDefinition {
            name: "launch_decision".to_string(),
            level: LogLevel::Info,
            event_name: "workflow.launch.decision".to_string(),
            fields: vec![],
        },
    )
    .unwrap();
    <InMemoryMetricSink as MetricSink<InMemoryBackend>>::register_definition_tx(
        &metrics,
        &mut tx,
        &MetricDefinition {
            name: "launch_attempts".to_string(),
            kind: MetricKind::Counter,
            backend_name: "workflow_launch_attempts_total".to_string(),
            unit: "1".to_string(),
            labels: vec![],
        },
    )
    .unwrap();
    backend.commit(tx).unwrap();

    let flag = flags
        .evaluate(
            &backend,
            &FeatureFlagEvaluationRequest {
                name: "new_scheduler".to_string(),
                context: FeatureFlagEvaluationContext::default(),
            },
        )
        .unwrap();
    assert_eq!(flag.as_bool(), Some(true));

    let now = SystemTime::UNIX_EPOCH + Duration::from_secs(100);
    let message = queues
        .enqueue(
            &backend,
            &EnqueueMessageRequest {
                message_id: "msg-1".to_string(),
                queue: "workflow".to_string(),
                channel: "launch".to_string(),
                idempotency_key: None,
                payload: Json::Object(BTreeMap::new()),
            },
        )
        .unwrap();
    assert_eq!(message.status, "Pending");
    let claimed = queues
        .claim(
            &backend,
            &ClaimMessageRequest {
                queue: "workflow".to_string(),
                channel: "launch".to_string(),
                claimant: "worker-1".to_string(),
                now,
                visibility_timeout: Duration::from_secs(60),
                max_messages: 1,
            },
        )
        .unwrap();
    assert_eq!(claimed.len(), 1);

    let lease = leases
        .acquire(
            &backend,
            &LeaseAcquireRequest {
                definition_id: lease_id,
                resource: "workflow/msg-1".to_string(),
                holder: "worker-1".to_string(),
                now,
            },
        )
        .unwrap();
    assert!(lease.acquired);

    workflows
        .start(
            &backend,
            &StartWorkflowRequest {
                workflow_execution_id: "wf-1".to_string(),
                workflow_name: "ProvisionService".to_string(),
                workflow_version: 1,
                start_step: "validate_request".to_string(),
                state: Json::Object(BTreeMap::new()),
            },
        )
        .unwrap();
    let steps = workflows
        .claim_steps(
            &backend,
            &ClaimWorkflowStepRequest {
                workflow_execution_id: "wf-1".to_string(),
                workflow_name: "ProvisionService".to_string(),
                workflow_version: 1,
                worker: "worker-1".to_string(),
                now,
                lease_duration: Duration::from_secs(60),
                max_steps: 1,
            },
        )
        .unwrap();
    assert_eq!(steps.len(), 1);
    workflows
        .keep_alive_step(
            &backend,
            &KeepAliveWorkflowStepRequest {
                workflow_execution_id: "wf-1".to_string(),
                worker: "worker-1".to_string(),
                current_step: "validate_request".to_string(),
                now,
                lease_duration: Duration::from_secs(60),
            },
        )
        .unwrap();

    logs.emit_log(
        &backend,
        &LogEvent {
            name: "launch_decision".to_string(),
            level: LogLevel::Info,
            event_name: "workflow.launch.decision".to_string(),
            fields: BTreeMap::new(),
        },
    )
    .unwrap();
    metrics
        .record_metric(
            &backend,
            &MetricSample {
                name: "launch_attempts".to_string(),
                kind: MetricKind::Counter,
                backend_name: "workflow_launch_attempts_total".to_string(),
                value: 1.0,
                unit: "1".to_string(),
                labels: BTreeMap::new(),
            },
        )
        .unwrap();
    assert_eq!(logs.inspect_events(&backend).unwrap().len(), 1);
    assert_eq!(metrics.inspect_samples(&backend).unwrap().len(), 1);

    let mut tx = backend.begin().unwrap();
    backend
        .put(
            &mut tx,
            "orders",
            "order-1",
            Json::Object(BTreeMap::from([(
                "status".to_string(),
                Json::String("new".to_string()),
            )])),
        )
        .unwrap();
    backend.commit(tx).unwrap();
    let mut tx = backend.begin().unwrap();
    assert_eq!(
        backend
            .query(
                &mut tx,
                "orders",
                &Query::KeyPrefix {
                    prefix: "order-".to_string()
                },
            )
            .unwrap()
            .len(),
        1
    );
    backend.commit(tx).unwrap();
}
EOF
(cd "$TMPDIR/out-rust" && CARGO_TARGET_DIR="$TMPDIR/rust-target" cargo test)
