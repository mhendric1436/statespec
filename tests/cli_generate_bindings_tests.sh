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

# Positive generation: Java.
run_expect_status 0 "$CLI" generate bindings --lang java "$SPEC" --out "$TMPDIR/out-java"
assert_output_contains "generated $TMPDIR/out-java/com/statespec/backend/Backend.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Json.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Backend.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/ExternalSystem.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/FeatureFlag.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Lease.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Log.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Metric.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Queue.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Workflow.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/generated/Descriptors.java"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "class Descriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record FeatureFlagDefinition"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ShapeDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "String tenant_id"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record NamespaceDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ValueDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EnumDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EventDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EventEnvelope"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "buildOrderAcceptedEvent"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ExternalSystemDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataMappingDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataMappingPlan"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ExternalSystemOperatorMetadataUpsertRequest"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "interface ExternalSystemOperatorMetadataRepository"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataMappingInputs"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataMissingMappingSource"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "missingExternalSystemMetadataMappingSources"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "sourceValue"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "interface ExternalSystemMetadataMappingApplicator"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "externalSystemMetadataMappingPlan"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Optional<String> tenantField"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "List<String> keyFields"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Optional.of(\"tenant_id\")"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"metadata.base_url\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"client.base_url\""
assert_file_contains "$TMPDIR/out-java/com/statespec/backend/ExternalSystem.java" "interface ExternalSystem"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "externalSystemMetadataLookup"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "resolveExternalSystemMetadataTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "keyComplete()"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiRequestContext"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiResponse"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "interface ApiHandler"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "interface ExternalSystemOperatorMetadataApiHandler"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "apiRouteDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"OrderApi\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "List.of(\"StartOrderProcessing\")"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"OrderApi.StartOrderProcessing\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record WorkerDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record WorkerContext"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "interface Worker"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"OrderWorker\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Optional.of(\"EmailDispatch.SendConfirmation\")"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record PolicyDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record GarbageCollectionPolicy"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EntityStateDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EntityOwnershipDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EntityRelationDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EntityChildDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EntityInvariantDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "entityDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "featureFlagDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "shapeDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "namespaceDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "valueDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "enumDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "eventDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "externalSystemDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "apiDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "apiServerDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workerDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workerContexts"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "policyDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "queueDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "leaseDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workflowDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "EmailDispatch"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "OrderReconciler"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "2L"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "validate_order"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "NewScheduler"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "OrderAmount"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "OrderStatus"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "OrderAccepted"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Billing.Stripe"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "StartOrderProcessing"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Optional.of(\"/v1/tenants/{tenantId}/orders/{order_id}/start\")"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "OrderAccess"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record LogDefinition"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "logDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workflow.launch.decision"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record MetricDefinition"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "metricDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "ensureSystemCollections"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "registerFeatureFlagDefinitionsTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "createQueueDefinitionsTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "registerLeaseDefinitionsTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "registerRuntimeCatalogTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "new IndexDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "registerObservabilityCatalogTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "registerWorkflowDefinitionsTx"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workflow_launch_attempts_total"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Optional.of(\"composition\")"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "new EntityInvariantDescriptor(\"valid_status\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"by_tenant_status\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "\"by_tenant_order\""
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "new GarbageCollectionPolicy(\"P30D\", \"tombstone\")"

cat > "$TMPDIR/out-java/com/statespec/generated/MetadataResolverFixture.java" <<'JAVA'
package com.statespec.generated;

import com.statespec.backend.Backend;
import com.statespec.backend.ExternalSystem;
import com.statespec.backend.Json;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public final class MetadataResolverFixture
{
    private static final class FixtureTx implements Backend.Transaction
    {
        @Override public boolean isOpen() { return true; }
        @Override public void abort() throws Backend.BackendException {}
    }

    private static final class FixtureResolver implements ExternalSystem
    {
        int calls = 0;

        @Override public Optional<MetadataResolution> resolveMetadata(
            Backend backend,
            MetadataLookup lookup
        ) throws Backend.BackendException
        {
            return Optional.empty();
        }

        @Override public Optional<MetadataResolution> resolveMetadataTx(
            Backend.Transaction tx,
            MetadataLookup lookup
        ) throws Backend.BackendException
        {
            calls++;
            Json document = Json.object(Map.of(
                "tenant_id", Json.string("tenant-a"),
                "base_url", Json.string("https://api.stripe.test")
            ));
            return Optional.of(new MetadataResolution(
                new Backend.VersionedRecord(
                    lookup.metadataEntity(),
                    "tenant-a/stripe/default",
                    1L,
                    document
                ),
                ExternalSystem.missingRequiredMetadataFields(document, lookup.requiredFields())
            ));
        }
    }

    private static final class FixtureMappingApplicator
        implements Descriptors.ExternalSystemMetadataMappingApplicator
    {
        @Override public Descriptors.ExternalSystemMetadataMappingOutput
        applyExternalSystemMetadataMappings(
            Descriptors.ExternalSystemMetadataMappingPlan plan,
            Descriptors.ExternalSystemMetadataMappingInputs inputs
        )
        {
            java.util.HashMap<String, Json> clientConfig = new java.util.HashMap<>();
            java.util.HashMap<String, Json> requestPayload = new java.util.HashMap<>();
            for (Descriptors.ExternalSystemMetadataMappingAssignment assignment :
                plan.allMappings())
            {
                Optional<Json> value = inputs.assignmentValue(assignment);
                if (value.isEmpty())
                {
                    continue;
                }
                if (assignment.targetRoot().equals("client"))
                {
                    clientConfig.put(assignment.field(), value.orElseThrow());
                }
                else if (assignment.targetRoot().equals("request"))
                {
                    requestPayload.put(assignment.field(), value.orElseThrow());
                }
            }
            return new Descriptors.ExternalSystemMetadataMappingOutput(
                clientConfig,
                requestPayload,
                Descriptors.missingExternalSystemMetadataMappingSources(plan, inputs)
            );
        }
    }

    private static final class FixtureOperatorMetadataRepository
        implements Descriptors.ExternalSystemOperatorMetadataRepository
    {
        @Override public Optional<Backend.VersionedRecord> upsertMetadataTx(
            Backend.Transaction tx,
            Descriptors.ExternalSystemOperatorMetadataUpsertRequest request
        )
        {
            return Optional.of(new Backend.VersionedRecord(
                request.lookup().metadataEntity(),
                "tenant-a/stripe/default",
                request.expectedVersion().orElse(0L) + 1L,
                request.document()
            ));
        }

        @Override public Optional<Backend.VersionedRecord> getMetadataTx(
            Backend.Transaction tx,
            Descriptors.ExternalSystemOperatorMetadataGetRequest request
        )
        {
            return Optional.of(new Backend.VersionedRecord(
                request.lookup().metadataEntity(),
                "tenant-a/stripe/default",
                1L,
                Json.object(Map.of("status", Json.string("Active")))
            ));
        }

        @Override public Optional<Backend.VersionedRecord> disableMetadataTx(
            Backend.Transaction tx,
            Descriptors.ExternalSystemOperatorMetadataDisableRequest request
        )
        {
            return Optional.of(new Backend.VersionedRecord(
                request.lookup().metadataEntity(),
                "tenant-a/stripe/default",
                request.expectedVersion().orElse(0L) + 1L,
                Json.object(Map.of("status", Json.string(request.disabledStatus())))
            ));
        }

        @Override public Optional<Backend.VersionedRecord> deleteMetadataTx(
            Backend.Transaction tx,
            Descriptors.ExternalSystemOperatorMetadataDeleteRequest request
        )
        {
            return Optional.of(new Backend.VersionedRecord(
                request.lookup().metadataEntity(),
                "tenant-a/stripe/default",
                request.expectedVersion().orElse(0L) + 1L,
                Json.object(Map.of("status", Json.string(request.deletedStatus())))
            ));
        }
    }

    private static final class FixtureOperatorMetadataApiHandler
        implements Descriptors.ExternalSystemOperatorMetadataApiHandler
    {
        @Override public Descriptors.ApiResponse handleUpsertMetadataTx(
            Backend.Transaction tx,
            Descriptors.ExternalSystemOperatorMetadataRepository repository,
            Descriptors.ExternalSystemOperatorMetadataUpsertRequest request
        ) throws Exception
        {
            Optional<Backend.VersionedRecord> record =
                repository.upsertMetadataTx(tx, request);
            return new Descriptors.ApiResponse(
                record.isPresent() ? 200 : 404,
                Json.object(Map.of("operation", Json.string("upsert")))
            );
        }

        @Override public Descriptors.ApiResponse handleGetMetadataTx(
            Backend.Transaction tx,
            Descriptors.ExternalSystemOperatorMetadataRepository repository,
            Descriptors.ExternalSystemOperatorMetadataGetRequest request
        ) throws Exception
        {
            Optional<Backend.VersionedRecord> record =
                repository.getMetadataTx(tx, request);
            return new Descriptors.ApiResponse(
                record.isPresent() ? 200 : 404,
                Json.object(Map.of("operation", Json.string("get")))
            );
        }

        @Override public Descriptors.ApiResponse handleDisableMetadataTx(
            Backend.Transaction tx,
            Descriptors.ExternalSystemOperatorMetadataRepository repository,
            Descriptors.ExternalSystemOperatorMetadataDisableRequest request
        ) throws Exception
        {
            Optional<Backend.VersionedRecord> record =
                repository.disableMetadataTx(tx, request);
            return new Descriptors.ApiResponse(
                record.isPresent() ? 200 : 404,
                Json.object(Map.of("operation", Json.string("disable")))
            );
        }

        @Override public Descriptors.ApiResponse handleDeleteMetadataTx(
            Backend.Transaction tx,
            Descriptors.ExternalSystemOperatorMetadataRepository repository,
            Descriptors.ExternalSystemOperatorMetadataDeleteRequest request
        ) throws Exception
        {
            Optional<Backend.VersionedRecord> record =
                repository.deleteMetadataTx(tx, request);
            return new Descriptors.ApiResponse(
                record.isPresent() ? 200 : 404,
                Json.object(Map.of("operation", Json.string("delete")))
            );
        }
    }

    public static void main(String[] args) throws Exception
    {
        FixtureResolver resolver = new FixtureResolver();
        FixtureTx tx = new FixtureTx();
        Descriptors.ExternalSystemMetadataMappingPlan plan =
            Descriptors.externalSystemMetadataMappingPlan(
                Descriptors.externalSystemDescriptors().get(0)
            );
        if (plan.allMappings().size() != 4 ||
            plan.clientMappings().size() != 3 || plan.requestMappings().size() != 1 ||
            !plan.clientMappings().get(0).sourceRoot().equals("metadata") ||
            !plan.clientMappings().get(0).sourceField().equals("base_url") ||
            !plan.clientMappings().get(0).targetRoot().equals("client") ||
            !plan.clientMappings().get(0).field().equals("base_url") ||
            !plan.requestMappings().get(0).sourceRoot().equals("input") ||
            !plan.requestMappings().get(0).sourceField().equals("order_id") ||
            !plan.requestMappings().get(0).targetRoot().equals("request") ||
            !plan.requestMappings().get(0).field().equals("order_id"))
        {
            throw new AssertionError("unexpected metadata mapping plan");
        }
        Descriptors.ExternalSystemMetadataMappingApplicator applicator =
            new FixtureMappingApplicator();
        Descriptors.ExternalSystemMetadataMappingOutput mapped =
            applicator.applyExternalSystemMetadataMappings(
                plan,
                new Descriptors.ExternalSystemMetadataMappingInputs(
                    Map.of("order_id", Json.string("order-1")),
                    Map.of(),
                    Map.of(),
                    Map.of(
                        "base_url", Json.string("https://api.stripe.test"),
                        "auth_ref", Json.string("secret:stripe"),
                        "timeout_ms", Json.integer(5000)
                    )
                )
            );
        if (mapped.clientConfig().size() != 3 || mapped.requestPayload().size() != 1 ||
            !mapped.missingSources().isEmpty())
        {
            throw new AssertionError("unexpected mapped metadata output");
        }
        Descriptors.ExternalSystemMetadataMappingOutput missingMapped =
            applicator.applyExternalSystemMetadataMappings(
                plan,
                new Descriptors.ExternalSystemMetadataMappingInputs(
                    Map.of("order_id", Json.string("order-1")),
                    Map.of(),
                    Map.of(),
                    Map.of(
                        "base_url", Json.string("https://api.stripe.test"),
                        "auth_ref", Json.string("secret:stripe")
                    )
                )
            );
        if (missingMapped.missingSources().size() != 1 ||
            !missingMapped.missingSources().get(0).source().equals("metadata.timeout_ms") ||
            !missingMapped.missingSources().get(0).targetRoot().equals("client") ||
            !missingMapped.missingSources().get(0).field().equals("timeout_ms"))
        {
            throw new AssertionError("unexpected missing mapping diagnostics");
        }
        List<ExternalSystem.MetadataKeyValue> keys = List.of(
            new ExternalSystem.MetadataKeyValue("tenant_id", Json.string("tenant-a")),
            new ExternalSystem.MetadataKeyValue("external_system_id", Json.string("stripe")),
            new ExternalSystem.MetadataKeyValue("profile", Json.string("default"))
        );
        ExternalSystem.MetadataLookup lookup =
            Descriptors.externalSystemMetadataLookup("Billing.Stripe", keys).orElseThrow();
        Descriptors.ExternalSystemOperatorMetadataRepository repository =
            new FixtureOperatorMetadataRepository();
        Backend.VersionedRecord upserted = repository.upsertMetadataTx(
            tx,
            new Descriptors.ExternalSystemOperatorMetadataUpsertRequest(
                lookup,
                Json.object(Map.of("tenant_id", Json.string("tenant-a"))),
                Optional.of(1L)
            )
        ).orElseThrow();
        Backend.VersionedRecord loaded = repository.getMetadataTx(
            tx,
            new Descriptors.ExternalSystemOperatorMetadataGetRequest(lookup)
        ).orElseThrow();
        Backend.VersionedRecord disabled = repository.disableMetadataTx(
            tx,
            new Descriptors.ExternalSystemOperatorMetadataDisableRequest(
                lookup,
                Optional.of(upserted.version()),
                "Disabled"
            )
        ).orElseThrow();
        Backend.VersionedRecord deleted = repository.deleteMetadataTx(
            tx,
            new Descriptors.ExternalSystemOperatorMetadataDeleteRequest(
                lookup,
                Optional.of(disabled.version()),
                "Deleted"
            )
        ).orElseThrow();
        if (upserted.version() != 2L || loaded.version() != 1L ||
            disabled.version() != 3L || deleted.version() != 4L)
        {
            throw new AssertionError("unexpected metadata repository result");
        }
        Descriptors.ExternalSystemOperatorMetadataApiHandler metadataApiHandler =
            new FixtureOperatorMetadataApiHandler();
        Descriptors.ApiResponse metadataApiResponse =
            metadataApiHandler.handleUpsertMetadataTx(
                tx,
                repository,
                new Descriptors.ExternalSystemOperatorMetadataUpsertRequest(
                    lookup,
                    Json.object(Map.of("tenant_id", Json.string("tenant-a"))),
                    Optional.of(deleted.version())
                )
            );
        if (metadataApiResponse.statusCode() != 200)
        {
            throw new AssertionError("unexpected operator metadata API response");
        }
        Optional<ExternalSystem.MetadataResolution> resolved =
            Descriptors.resolveExternalSystemMetadataTx(resolver, tx, "Billing.Stripe", keys);
        if (resolved.isEmpty() || resolved.orElseThrow().complete() || resolver.calls != 1)
        {
            throw new AssertionError("expected incomplete metadata resolution through resolver");
        }

        Optional<ExternalSystem.MetadataResolution> skipped =
            Descriptors.resolveExternalSystemMetadataTx(
                resolver, tx, "Billing.Stripe", keys.subList(0, 2)
            );
        if (skipped.isPresent() || resolver.calls != 1)
        {
            throw new AssertionError("expected incomplete key to skip resolver");
        }
    }
}
JAVA
find "$TMPDIR/out-java" -name '*.java' -print > "$TMPDIR/out-java/sources.list"
${JAVAC:-javac} @"$TMPDIR/out-java/sources.list"
${JAVA:-java} -cp "$TMPDIR/out-java" com.statespec.generated.MetadataResolverFixture

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
