#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR" generated/cpp generated/go generated/java generated/rust generated/openapi
    rmdir generated 2>/dev/null || true
}
trap cleanup EXIT

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
FIXTURES_DIR="$SCRIPT_DIR/fixtures"
. "$SCRIPT_DIR/cli/common.sh"

SPEC="$FIXTURES_DIR/bindings-full.sspec"
E2E_SPEC="testdata/generators/external-system-metadata-e2e.sspec"

# End-to-end external-system metadata generated binding regression.
run_expect_status 0 "$CLI" generate bindings --lang cpp "$E2E_SPEC" --out "$TMPDIR/out-e2e-cpp"
assert_file_contains "$TMPDIR/out-e2e-cpp/system_descriptors.hpp" "api_route_descriptors"
assert_file_contains "$TMPDIR/out-e2e-cpp/system_descriptors.hpp" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-cpp/system_descriptors.hpp" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-cpp/system_descriptors.hpp" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-cpp/system_descriptors.hpp" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-cpp/system_descriptors.hpp" "IExternalSystemOperatorMetadataApiHandler"

run_expect_status 0 "$CLI" generate bindings --lang go "$E2E_SPEC" --out "$TMPDIR/out-e2e-go"
assert_file_contains "$TMPDIR/out-e2e-go/backend/descriptors.go" "func ApiRouteDescriptors"
assert_file_contains "$TMPDIR/out-e2e-go/backend/descriptors.go" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-go/backend/descriptors.go" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-go/backend/descriptors.go" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-go/backend/descriptors.go" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-go/backend/descriptors.go" "ExternalSystemOperatorMetadataAPIHandler"

run_expect_status 0 "$CLI" generate bindings --lang java "$E2E_SPEC" --out "$TMPDIR/out-e2e-java"
assert_file_contains "$TMPDIR/out-e2e-java/com/statespec/generated/Descriptors.java" "apiRouteDescriptors"
assert_file_contains "$TMPDIR/out-e2e-java/com/statespec/generated/Descriptors.java" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-java/com/statespec/generated/Descriptors.java" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-java/com/statespec/generated/Descriptors.java" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-java/com/statespec/generated/Descriptors.java" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-java/com/statespec/generated/Descriptors.java" "ExternalSystemOperatorMetadataApiHandler"

run_expect_status 0 "$CLI" generate bindings --lang rust "$E2E_SPEC" --out "$TMPDIR/out-e2e-rust"
assert_file_contains "$TMPDIR/out-e2e-rust/descriptors.rs" "pub fn api_route_descriptors"
assert_file_contains "$TMPDIR/out-e2e-rust/descriptors.rs" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-rust/descriptors.rs" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-rust/descriptors.rs" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-rust/descriptors.rs" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-rust/descriptors.rs" "ExternalSystemOperatorMetadataApiHandler"

# Positive generation: Rust.
run_expect_status 0 "$CLI" generate bindings --lang rust "$SPEC" --out "$TMPDIR/out-rust"
assert_output_contains "generated $TMPDIR/out-rust/backend.rs"
assert_file_exists "$TMPDIR/out-rust/json.rs"
assert_file_exists "$TMPDIR/out-rust/backend.rs"
assert_file_exists "$TMPDIR/out-rust/external_system.rs"
assert_file_exists "$TMPDIR/out-rust/feature_flag.rs"
assert_file_exists "$TMPDIR/out-rust/lease.rs"
assert_file_exists "$TMPDIR/out-rust/log.rs"
assert_file_exists "$TMPDIR/out-rust/metric.rs"
assert_file_exists "$TMPDIR/out-rust/queue.rs"
assert_file_exists "$TMPDIR/out-rust/workflow.rs"
assert_file_exists "$TMPDIR/out-rust/descriptors.rs"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct FeatureFlagDefinition"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn feature_flag_definitions() -> Vec<FeatureFlagDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ShapeDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn shape_descriptors() -> Vec<ShapeDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub tenant_id: String"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct NamespaceDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn namespace_descriptors() -> Vec<NamespaceDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ValueDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn value_descriptors() -> Vec<ValueDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EnumDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn enum_descriptors() -> Vec<EnumDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EventDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn event_descriptors() -> Vec<EventDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EventEnvelope"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn make_order_accepted_event"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ExternalSystemDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ExternalSystemMetadataDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ExternalSystemMetadataMappingDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ExternalSystemMetadataMappingPlan"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ExternalSystemOperatorMetadataUpsertRequest"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub trait ExternalSystemOperatorMetadataRepository"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ExternalSystemMetadataMappingInputs"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ExternalSystemMetadataMissingMappingSource"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn missing_external_system_metadata_mapping_sources"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn source_value"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub trait ExternalSystemMetadataMappingApplicator"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn external_system_metadata_mapping_plan"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub tenant_field: Option<String>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub key_fields: Vec<String>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "tenant_field: Some(\"tenant_id\".to_string())"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "source: \"metadata.base_url\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "target: \"client.base_url\".to_string()"
assert_file_contains "$TMPDIR/out-rust/external_system.rs" "pub trait ExternalSystemMetadataResolver"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn external_system_metadata_lookup"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn external_system_metadata_lookup_by_name"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn resolve_external_system_metadata_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn resolve_external_system_metadata_by_name_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "key_complete()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn external_system_descriptors() -> Vec<ExternalSystemDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn api_descriptors() -> Vec<ApiDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn api_server_descriptors() -> Vec<ApiServerDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiRequestContext"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiResponse"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub trait ApiHandler"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub trait ExternalSystemOperatorMetadataApiHandler"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn api_route_descriptors() -> Vec<ApiRouteDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "name: \"OrderApi\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "serves: vec![\"StartOrderProcessing\".to_string()]"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "name: \"OrderApi.StartOrderProcessing\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "server_name: \"OrderApi\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct WorkerDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct WorkerContext"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub trait Worker"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn worker_descriptors() -> Vec<WorkerDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn worker_contexts() -> Vec<WorkerContext>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "name: \"OrderWorker\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "polls: Some(\"EmailDispatch.SendConfirmation\".to_string())"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct PolicyDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn policy_descriptors() -> Vec<PolicyDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct GarbageCollectionPolicy"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EntityStateDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EntityOwnershipDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EntityRelationDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EntityChildDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EntityInvariantDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn entity_descriptors() -> Vec<EntityDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn collection_descriptors() -> Vec<CollectionDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "IndexDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn queue_definitions() -> Vec<QueueDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn lease_definitions() -> Vec<LeaseDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn workflow_definitions() -> Vec<WorkflowDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "EmailDispatch"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "OrderReconciler"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "workflow_version: 2"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "validate_order"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "NewScheduler"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "OrderAmount"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "OrderStatus"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "OrderAccepted"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "Billing.Stripe"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "StartOrderProcessing"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "path: Some(\"/v1/tenants/{tenantId}/orders/{order_id}/start\".to_string())"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "OrderAccess"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct LogDefinition"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn log_definitions() -> Vec<LogDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "event_name: \"workflow.launch.decision\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct MetricDefinition"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn metric_definitions() -> Vec<MetricDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn ensure_system_collections"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn register_feature_flag_definitions_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn create_queue_definitions_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn register_lease_definitions_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn register_runtime_catalog_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn register_observability_catalog_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn register_workflow_definitions_tx"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "backend_name: \"workflow_launch_attempts_total\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "relation_kind: Some(\"composition\".to_string())"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "name: \"valid_status\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "name: \"by_tenant_status\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "unique: true"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "garbage_collection: Some(GarbageCollectionPolicy { after: \"P30D\".to_string(), mode: \"tombstone\".to_string() })"

cat > "$TMPDIR/out-rust/Cargo.toml" <<'TOML'
[package]
name = "statespec-generated-fixture"
version = "0.1.0"
edition = "2021"

[lib]
path = "lib.rs"
TOML

cat > "$TMPDIR/out-rust/lib.rs" <<'RUST'
pub mod backend;
pub mod descriptors;
pub mod external_system;
pub mod feature_flag;
pub mod json;
pub mod lease;
pub mod log;
pub mod metric;
pub mod queue;
pub mod workflow;

#[cfg(test)]
mod tests {
    use std::cell::Cell;
    use std::collections::BTreeMap;

    use crate::backend::{
        Backend, BackendCapabilities, BackendResult, CollectionDescriptor, Query, Transaction,
        VersionedRecord,
    };
    use crate::external_system::{
        ExternalSystemMetadataKeyValue, ExternalSystemMetadataLookup,
        ExternalSystemMetadataResolution, ExternalSystemMetadataResolver,
    };
    use crate::json::Json;

    #[derive(Default)]
    struct FixtureTx;

    impl Transaction for FixtureTx {
        fn is_open(&self) -> bool {
            true
        }

        fn abort(&mut self) -> BackendResult<()> {
            Ok(())
        }
    }

    struct FixtureBackend;

    impl Backend for FixtureBackend {
        type Tx = FixtureTx;

        fn capabilities(&self) -> BackendCapabilities {
            BackendCapabilities::default()
        }

        fn ensure_collection(&self, _descriptor: &CollectionDescriptor) -> BackendResult<()> {
            Ok(())
        }

        fn ensure_collections(&self, _descriptors: &[CollectionDescriptor]) -> BackendResult<()> {
            Ok(())
        }

        fn begin(&self) -> BackendResult<Self::Tx> {
            Ok(FixtureTx)
        }

        fn get(
            &self,
            _tx: &mut Self::Tx,
            _collection: &str,
            _key: &str,
        ) -> BackendResult<Option<VersionedRecord>> {
            Ok(None)
        }

        fn query(
            &self,
            _tx: &mut Self::Tx,
            _collection: &str,
            _query: &Query,
        ) -> BackendResult<Vec<VersionedRecord>> {
            Ok(vec![])
        }

        fn put(
            &self,
            _tx: &mut Self::Tx,
            _collection: &str,
            _key: &str,
            _document: Json,
        ) -> BackendResult<()> {
            Ok(())
        }

        fn erase(&self, _tx: &mut Self::Tx, _collection: &str, _key: &str) -> BackendResult<()> {
            Ok(())
        }

        fn commit(&self, _tx: Self::Tx) -> BackendResult<()> {
            Ok(())
        }
    }

    struct FixtureResolver {
        calls: Cell<usize>,
    }

    impl ExternalSystemMetadataResolver<FixtureBackend> for FixtureResolver {
        fn resolve_metadata(
            &self,
            _backend: &FixtureBackend,
            _lookup: &ExternalSystemMetadataLookup,
        ) -> BackendResult<Option<ExternalSystemMetadataResolution>> {
            Ok(None)
        }

        fn resolve_metadata_tx(
            &self,
            _tx: &mut FixtureTx,
            lookup: &ExternalSystemMetadataLookup,
        ) -> BackendResult<Option<ExternalSystemMetadataResolution>> {
            self.calls.set(self.calls.get() + 1);
            let document = Json::Object(
                [
                    ("tenant_id".to_string(), Json::String("tenant-a".to_string())),
                    (
                        "base_url".to_string(),
                        Json::String("https://api.stripe.test".to_string()),
                    ),
                ]
                .into_iter()
                .collect(),
            );
            Ok(Some(ExternalSystemMetadataResolution {
                record: VersionedRecord {
                    collection: lookup.metadata_entity.clone(),
                    key: "tenant-a/stripe/default".to_string(),
                    version: 1,
                    document: document.clone(),
                },
                missing_required_fields: crate::external_system::missing_required_metadata_fields(
                    &document,
                    &lookup.required_fields,
                ),
            }))
        }
    }

    struct FixtureMappingApplicator;

    impl crate::descriptors::ExternalSystemMetadataMappingApplicator
        for FixtureMappingApplicator
    {
        fn apply_external_system_metadata_mappings(
            &self,
            plan: &crate::descriptors::ExternalSystemMetadataMappingPlan,
            inputs: &crate::descriptors::ExternalSystemMetadataMappingInputs,
        ) -> BackendResult<crate::descriptors::ExternalSystemMetadataMappingOutput> {
            let mut output = crate::descriptors::ExternalSystemMetadataMappingOutput::default();
            output.missing_sources =
                crate::descriptors::missing_external_system_metadata_mapping_sources(
                    plan, inputs,
                );
            for assignment in &plan.all_mappings {
                let source = inputs.assignment_value(assignment);
                if let Some(value) = source {
                    if assignment.target_root == "client" {
                        output
                            .client_config
                            .insert(assignment.field.clone(), value.clone());
                    } else if assignment.target_root == "request" {
                        output
                            .request_payload
                            .insert(assignment.field.clone(), value.clone());
                    }
                }
            }
            Ok(output)
        }
    }

    struct FixtureOperatorMetadataRepository;

    impl crate::descriptors::ExternalSystemOperatorMetadataRepository<FixtureBackend>
        for FixtureOperatorMetadataRepository
    {
        fn upsert_metadata_tx(
            &self,
            _tx: &mut FixtureTx,
            request: &crate::descriptors::ExternalSystemOperatorMetadataUpsertRequest,
        ) -> BackendResult<Option<VersionedRecord>> {
            Ok(Some(VersionedRecord {
                collection: request.lookup.metadata_entity.clone(),
                key: "tenant-a/stripe/default".to_string(),
                version: request.expected_version.unwrap_or(0) + 1,
                document: request.document.clone(),
            }))
        }

        fn get_metadata_tx(
            &self,
            _tx: &mut FixtureTx,
            request: &crate::descriptors::ExternalSystemOperatorMetadataGetRequest,
        ) -> BackendResult<Option<VersionedRecord>> {
            Ok(Some(VersionedRecord {
                collection: request.lookup.metadata_entity.clone(),
                key: "tenant-a/stripe/default".to_string(),
                version: 1,
                document: Json::Object(BTreeMap::from([(
                    "status".to_string(),
                    Json::String("Active".to_string()),
                )])),
            }))
        }

        fn disable_metadata_tx(
            &self,
            _tx: &mut FixtureTx,
            request: &crate::descriptors::ExternalSystemOperatorMetadataDisableRequest,
        ) -> BackendResult<Option<VersionedRecord>> {
            Ok(Some(VersionedRecord {
                collection: request.lookup.metadata_entity.clone(),
                key: "tenant-a/stripe/default".to_string(),
                version: request.expected_version.unwrap_or(0) + 1,
                document: Json::Object(BTreeMap::from([(
                    "status".to_string(),
                    Json::String(request.disabled_status.clone()),
                )])),
            }))
        }

        fn delete_metadata_tx(
            &self,
            _tx: &mut FixtureTx,
            request: &crate::descriptors::ExternalSystemOperatorMetadataDeleteRequest,
        ) -> BackendResult<Option<VersionedRecord>> {
            Ok(Some(VersionedRecord {
                collection: request.lookup.metadata_entity.clone(),
                key: "tenant-a/stripe/default".to_string(),
                version: request.expected_version.unwrap_or(0) + 1,
                document: Json::Object(BTreeMap::from([(
                    "status".to_string(),
                    Json::String(request.deleted_status.clone()),
                )])),
            }))
        }
    }

    struct FixtureOperatorMetadataApiHandler;

    impl crate::descriptors::ExternalSystemOperatorMetadataApiHandler<FixtureBackend>
        for FixtureOperatorMetadataApiHandler
    {
        fn handle_upsert_metadata_tx<
            R: crate::descriptors::ExternalSystemOperatorMetadataRepository<FixtureBackend>,
        >(
            &self,
            tx: &mut FixtureTx,
            repository: &R,
            request: &crate::descriptors::ExternalSystemOperatorMetadataUpsertRequest,
        ) -> BackendResult<crate::descriptors::ApiResponse> {
            let record = repository.upsert_metadata_tx(tx, request)?;
            Ok(crate::descriptors::ApiResponse {
                status_code: if record.is_some() { 200 } else { 404 },
                body: Json::Object(BTreeMap::from([(
                    "operation".to_string(),
                    Json::String("upsert".to_string()),
                )])),
            })
        }

        fn handle_get_metadata_tx<
            R: crate::descriptors::ExternalSystemOperatorMetadataRepository<FixtureBackend>,
        >(
            &self,
            tx: &mut FixtureTx,
            repository: &R,
            request: &crate::descriptors::ExternalSystemOperatorMetadataGetRequest,
        ) -> BackendResult<crate::descriptors::ApiResponse> {
            let record = repository.get_metadata_tx(tx, request)?;
            Ok(crate::descriptors::ApiResponse {
                status_code: if record.is_some() { 200 } else { 404 },
                body: Json::Object(BTreeMap::from([(
                    "operation".to_string(),
                    Json::String("get".to_string()),
                )])),
            })
        }

        fn handle_disable_metadata_tx<
            R: crate::descriptors::ExternalSystemOperatorMetadataRepository<FixtureBackend>,
        >(
            &self,
            tx: &mut FixtureTx,
            repository: &R,
            request: &crate::descriptors::ExternalSystemOperatorMetadataDisableRequest,
        ) -> BackendResult<crate::descriptors::ApiResponse> {
            let record = repository.disable_metadata_tx(tx, request)?;
            Ok(crate::descriptors::ApiResponse {
                status_code: if record.is_some() { 200 } else { 404 },
                body: Json::Object(BTreeMap::from([(
                    "operation".to_string(),
                    Json::String("disable".to_string()),
                )])),
            })
        }

        fn handle_delete_metadata_tx<
            R: crate::descriptors::ExternalSystemOperatorMetadataRepository<FixtureBackend>,
        >(
            &self,
            tx: &mut FixtureTx,
            repository: &R,
            request: &crate::descriptors::ExternalSystemOperatorMetadataDeleteRequest,
        ) -> BackendResult<crate::descriptors::ApiResponse> {
            let record = repository.delete_metadata_tx(tx, request)?;
            Ok(crate::descriptors::ApiResponse {
                status_code: if record.is_some() { 200 } else { 404 },
                body: Json::Object(BTreeMap::from([(
                    "operation".to_string(),
                    Json::String("delete".to_string()),
                )])),
            })
        }
    }

    #[test]
    fn generated_metadata_resolver_fixture() {
        let resolver = FixtureResolver {
            calls: Cell::new(0),
        };
        let mut tx = FixtureTx;
        let descriptors = crate::descriptors::external_system_descriptors();
        let plan = crate::descriptors::external_system_metadata_mapping_plan(&descriptors[0]);
        assert_eq!(plan.all_mappings.len(), 4);
        assert_eq!(plan.client_mappings.len(), 3);
        assert_eq!(plan.request_mappings.len(), 1);
        assert_eq!(plan.client_mappings[0].source_root, "metadata");
        assert_eq!(plan.client_mappings[0].source_field, "base_url");
        assert_eq!(plan.client_mappings[0].target_root, "client");
        assert_eq!(plan.client_mappings[0].field, "base_url");
        assert_eq!(plan.request_mappings[0].source_root, "input");
        assert_eq!(plan.request_mappings[0].source_field, "order_id");
        assert_eq!(plan.request_mappings[0].target_root, "request");
        assert_eq!(plan.request_mappings[0].field, "order_id");
        let applicator = FixtureMappingApplicator;
        let mapped = crate::descriptors::ExternalSystemMetadataMappingApplicator::apply_external_system_metadata_mappings(
            &applicator,
            &plan,
            &crate::descriptors::ExternalSystemMetadataMappingInputs {
                input: BTreeMap::from([(
                    "order_id".to_string(),
                    Json::String("order-1".to_string()),
                )]),
                metadata: BTreeMap::from([
                    (
                        "base_url".to_string(),
                        Json::String("https://api.stripe.test".to_string()),
                    ),
                    (
                        "auth_ref".to_string(),
                        Json::String("secret:stripe".to_string()),
                    ),
                    ("timeout_ms".to_string(), Json::Integer(5000)),
                ]),
                ..Default::default()
            },
        )
        .expect("mapping applicator should not fail");
        assert_eq!(mapped.client_config.len(), 3);
        assert_eq!(mapped.request_payload.len(), 1);
        assert_eq!(mapped.missing_sources.len(), 0);
        let missing_mapped = crate::descriptors::ExternalSystemMetadataMappingApplicator::apply_external_system_metadata_mappings(
            &applicator,
            &plan,
            &crate::descriptors::ExternalSystemMetadataMappingInputs {
                input: BTreeMap::from([(
                    "order_id".to_string(),
                    Json::String("order-1".to_string()),
                )]),
                metadata: BTreeMap::from([
                    (
                        "base_url".to_string(),
                        Json::String("https://api.stripe.test".to_string()),
                    ),
                    (
                        "auth_ref".to_string(),
                        Json::String("secret:stripe".to_string()),
                    ),
                ]),
                ..Default::default()
            },
        )
        .expect("mapping applicator should not fail");
        assert_eq!(missing_mapped.missing_sources.len(), 1);
        assert_eq!(missing_mapped.missing_sources[0].source, "metadata.timeout_ms");
        assert_eq!(missing_mapped.missing_sources[0].target_root, "client");
        assert_eq!(missing_mapped.missing_sources[0].field, "timeout_ms");
        let keys = vec![
            ExternalSystemMetadataKeyValue {
                field: "tenant_id".to_string(),
                value: Json::String("tenant-a".to_string()),
            },
            ExternalSystemMetadataKeyValue {
                field: "external_system_id".to_string(),
                value: Json::String("stripe".to_string()),
            },
            ExternalSystemMetadataKeyValue {
                field: "profile".to_string(),
                value: Json::String("default".to_string()),
            },
        ];
        let lookup = crate::descriptors::external_system_metadata_lookup_by_name(
            "Billing.Stripe",
            keys.clone(),
        )
        .expect("metadata lookup should build");
        let repository = FixtureOperatorMetadataRepository;
        let upserted = crate::descriptors::ExternalSystemOperatorMetadataRepository::<FixtureBackend>::upsert_metadata_tx(
            &repository,
            &mut tx,
            &crate::descriptors::ExternalSystemOperatorMetadataUpsertRequest {
                lookup: lookup.clone(),
                document: Json::Object(BTreeMap::from([(
                    "tenant_id".to_string(),
                    Json::String("tenant-a".to_string()),
                )])),
                expected_version: Some(1),
            },
        )
        .expect("metadata upsert should not fail")
        .expect("metadata upsert should return record");
        let loaded = crate::descriptors::ExternalSystemOperatorMetadataRepository::<FixtureBackend>::get_metadata_tx(
            &repository,
            &mut tx,
            &crate::descriptors::ExternalSystemOperatorMetadataGetRequest {
                lookup: lookup.clone(),
            },
        )
        .expect("metadata get should not fail")
        .expect("metadata get should return record");
        let disabled = crate::descriptors::ExternalSystemOperatorMetadataRepository::<FixtureBackend>::disable_metadata_tx(
            &repository,
            &mut tx,
            &crate::descriptors::ExternalSystemOperatorMetadataDisableRequest {
                lookup: lookup.clone(),
                expected_version: Some(upserted.version),
                disabled_status: "Disabled".to_string(),
            },
        )
        .expect("metadata disable should not fail")
        .expect("metadata disable should return record");
        let deleted = crate::descriptors::ExternalSystemOperatorMetadataRepository::<FixtureBackend>::delete_metadata_tx(
            &repository,
            &mut tx,
            &crate::descriptors::ExternalSystemOperatorMetadataDeleteRequest {
                lookup: lookup.clone(),
                expected_version: Some(disabled.version),
                deleted_status: "Deleted".to_string(),
            },
        )
        .expect("metadata delete should not fail")
        .expect("metadata delete should return record");
        assert_eq!(upserted.version, 2);
        assert_eq!(loaded.version, 1);
        assert_eq!(disabled.version, 3);
        assert_eq!(deleted.version, 4);
        let metadata_api_handler = FixtureOperatorMetadataApiHandler;
        let metadata_api_response = crate::descriptors::ExternalSystemOperatorMetadataApiHandler::<
            FixtureBackend,
        >::handle_upsert_metadata_tx(
            &metadata_api_handler,
            &mut tx,
            &repository,
            &crate::descriptors::ExternalSystemOperatorMetadataUpsertRequest {
                lookup: lookup.clone(),
                document: Json::Object(BTreeMap::from([(
                    "tenant_id".to_string(),
                    Json::String("tenant-a".to_string()),
                )])),
                expected_version: Some(deleted.version),
            },
        )
        .expect("operator metadata API handler should not fail");
        assert_eq!(metadata_api_response.status_code, 200);

        let resolved = crate::descriptors::resolve_external_system_metadata_by_name_tx::<
            FixtureBackend,
            FixtureResolver,
        >(&resolver, &mut tx, "Billing.Stripe", keys.clone())
        .expect("resolver call should not fail")
        .expect("metadata should resolve");
        assert!(!resolved.complete());
        assert_eq!(resolver.calls.get(), 1);

        let skipped = crate::descriptors::resolve_external_system_metadata_by_name_tx::<
            FixtureBackend,
            FixtureResolver,
        >(&resolver, &mut tx, "Billing.Stripe", keys[..2].to_vec())
        .expect("resolver call should not fail");
        assert!(skipped.is_none());
        assert_eq!(resolver.calls.get(), 1);
    }
}
RUST
(cd "$TMPDIR/out-rust" && CARGO_TARGET_DIR="$TMPDIR/rust-target" cargo test)
