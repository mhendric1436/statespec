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

# Positive generation: Java.
run_expect_status 0 "$CLI" generate bindings --lang java "$SPEC" --out "$TMPDIR/out-java"
assert_output_contains "generated $TMPDIR/out-java/common/com/statespec/backend/Backend.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/Json.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/Backend.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/ExternalSystem.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/FeatureFlag.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/Lease.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/Log.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/Metric.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/Queue.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/Workflow.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryBackend.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryTransaction.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryCodec.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryFeatureFlagStore.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryQueueStore.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryLeaseStore.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryWorkflowStore.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryLogSink.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryMetricSink.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java"
assert_file_exists "$TMPDIR/out-java/Makefile"
assert_file_exists "$TMPDIR/out-java/api/com/statespec/generated/ApiDescriptors.java"
assert_file_exists "$TMPDIR/out-java/api/com/statespec/generated/ApiHandlers.java"
assert_file_exists "$TMPDIR/out-java/api/com/statespec/generated/ApiDispatcher.java"
assert_file_exists "$TMPDIR/out-java/api/com/statespec/generated/ApiServer.java"
assert_file_exists "$TMPDIR/out-java/api/com/statespec/generated/ApiRoutes.java"
assert_file_exists "$TMPDIR/out-java/api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java"
assert_file_exists "$TMPDIR/out-java/worker/com/statespec/generated/WorkerDescriptors.java"
assert_file_exists "$TMPDIR/out-java/worker/com/statespec/generated/WorkerContexts.java"
assert_file_exists "$TMPDIR/out-java/worker/com/statespec/generated/WorkerRegistry.java"
assert_file_exists "$TMPDIR/out-java/worker/com/statespec/generated/WorkerApplication.java"
assert_file_exists "$TMPDIR/out-java/worker/com/statespec/generated/WorkflowStepHandlers.java"
assert_file_exists "$TMPDIR/out-java/worker/com/statespec/generated/WorkflowRunner.java"
assert_file_exists "$TMPDIR/out-java/worker/com/statespec/generated/WorkerHandlers.java"
assert_file_exists "$TMPDIR/out-java/worker/com/statespec/generated/WorkerQueues.java"
assert_file_exists "$TMPDIR/out-java/worker/com/statespec/generated/WorkerLeases.java"
assert_file_exists "$TMPDIR/out-java/worker/com/statespec/generated/WorkerWorkflows.java"
assert_file_contains "$TMPDIR/out-java/Makefile" "JAVAC ?= javac"
assert_file_contains "$TMPDIR/out-java/Makefile" "BUILD_DIR ?= build/classes"
assert_file_contains "$TMPDIR/out-java/Makefile" "SOURCES := \$(shell find . -name '*.java')"
assert_file_contains "$TMPDIR/out-java/Makefile" "\$(JAVAC) -d \$(BUILD_DIR) \$(SOURCES)"
assert_file_contains "$TMPDIR/out-java/Makefile" "build-api"
assert_file_contains "$TMPDIR/out-java/Makefile" "build-worker"
assert_file_contains "$TMPDIR/out-java/Makefile" "package-api"
assert_file_contains "$TMPDIR/out-java/Makefile" "package-worker"
assert_file_contains "$TMPDIR/out-java/api/com/statespec/generated/ApiDescriptors.java" "class ApiDescriptors"
assert_file_contains "$TMPDIR/out-java/api/com/statespec/generated/ApiDescriptors.java" "apiDescriptors"
assert_file_contains "$TMPDIR/out-java/api/com/statespec/generated/ApiHandlers.java" "class ApiHandlers"
assert_file_contains "$TMPDIR/out-java/api/com/statespec/generated/ApiDispatcher.java" "findApiRoute"
assert_file_contains "$TMPDIR/out-java/api/com/statespec/generated/ApiDispatcher.java" "dispatchApiRoute"
assert_file_contains "$TMPDIR/out-java/api/com/statespec/generated/ApiServer.java" "class ApiServer"
assert_file_contains "$TMPDIR/out-java/api/com/statespec/generated/ApiServer.java" "findApiServer"
assert_file_contains "$TMPDIR/out-java/api/com/statespec/generated/ApiRoutes.java" "apiRouteDescriptors"
assert_file_contains "$TMPDIR/out-java/api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java" "ExternalSystemOperatorMetadataHandler"
assert_file_contains "$TMPDIR/out-java/worker/com/statespec/generated/WorkerDescriptors.java" "class WorkerDescriptors"
assert_file_contains "$TMPDIR/out-java/worker/com/statespec/generated/WorkerContexts.java" "workerContexts"
assert_file_contains "$TMPDIR/out-java/worker/com/statespec/generated/WorkerRegistry.java" "findWorkerDescriptor"
assert_file_contains "$TMPDIR/out-java/worker/com/statespec/generated/WorkerApplication.java" "class WorkerApplication"
assert_file_contains "$TMPDIR/out-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "interface Handler"
assert_file_contains "$TMPDIR/out-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "workflowStepHandlerKeys"
assert_file_contains "$TMPDIR/out-java/worker/com/statespec/generated/WorkflowRunner.java" "class WorkflowRunner"
assert_file_contains "$TMPDIR/out-java/worker/com/statespec/generated/WorkflowRunner.java" "keepAliveStep"
assert_file_contains "$TMPDIR/out-java/worker/com/statespec/generated/WorkerHandlers.java" "interface Handler"
assert_file_contains "$TMPDIR/out-java/worker/com/statespec/generated/WorkerQueues.java" "queueDefinitions"
assert_file_contains "$TMPDIR/out-java/worker/com/statespec/generated/WorkerLeases.java" "leaseDefinitions"
assert_file_contains "$TMPDIR/out-java/worker/com/statespec/generated/WorkerWorkflows.java" "workflowDefinitions"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/Backend.java" "enum FieldType"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/Backend.java" "FieldType type"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/Backend.java" "String typeName"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/Workflow.java" "record KeepAliveWorkflowStepRequest"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/Workflow.java" "keepAliveStepTx"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryBackend.java" "public final class InMemoryBackend"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryTransaction.java" "public final class InMemoryTransaction"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryCodec.java" "final class InMemoryCodec"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryFeatureFlagStore.java" "implements FeatureFlag"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryQueueStore.java" "implements Queue"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryLeaseStore.java" "implements Lease"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryWorkflowStore.java" "implements Workflow"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryLogSink.java" "inspectEvents"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/memory/InMemoryMetricSink.java" "inspectSamples"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "class Descriptors"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record FeatureFlagDefinition"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ShapeDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "String tenant_id"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ValueDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record EnumDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record EventDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record EventEnvelope"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "buildOrderAcceptedEvent"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ExternalSystemDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataMappingDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataMappingPlan"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ExternalSystemOperatorMetadataUpsertRequest"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "interface ExternalSystemOperatorMetadataRepository"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataMappingInputs"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ExternalSystemMetadataMissingMappingSource"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "missingExternalSystemMetadataMappingSources"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "sourceValue"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "interface ExternalSystemMetadataMappingApplicator"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "externalSystemMetadataMappingPlan"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "Optional<String> tenantField"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "List<String> keyFields"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "Optional.of(\"tenant_id\")"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "\"metadata.base_url\""
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "\"client.base_url\""
assert_file_contains "$TMPDIR/out-java/common/com/statespec/backend/ExternalSystem.java" "interface ExternalSystem"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "externalSystemMetadataLookup"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "resolveExternalSystemMetadataTx"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "keyComplete()"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ApiDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ApiRequestContext"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record ApiResponse"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "interface ApiHandler"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "interface ExternalSystemOperatorMetadataApiHandler"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "apiDescriptors"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "\"OrderApi\""
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "List.of(\"StartOrderProcessing\")"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "\"OrderApi.StartOrderProcessing\""
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record WorkerDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record WorkerContext"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "interface Worker"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "\"OrderWorker\""
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "Optional.of(\"EmailDispatch.SendConfirmation\")"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record PolicyDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record GarbageCollectionPolicy"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record EntityStateDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record EntityOwnershipDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record EntityRelationDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record EntityChildDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record EntityInvariantDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "entityDescriptors"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "featureFlagDefinitions"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "shapeDescriptors"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "valueDescriptors"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "enumDescriptors"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "eventDescriptors"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "externalSystemDescriptors"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "apiDescriptors"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "apiServerDescriptors"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "workerDescriptors"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "workerContexts"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "policyDescriptors"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "queueDefinitions"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "leaseDefinitions"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "workflowDefinitions"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "EmailDispatch"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "OrderReconciler"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "2L"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "validate_order"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "NewScheduler"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "OrderAmount"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "OrderStatus"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "OrderAccepted"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "Stripe"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "StartOrderProcessing"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "Optional.of(\"/v1/tenants/{tenantId}/orders/{order_id}/start\")"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "OrderAccess"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record LogDefinition"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "logDefinitions"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "workflow.launch.decision"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "record MetricDefinition"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "metricDefinitions"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "ensureSystemCollections"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "registerFeatureFlagDefinitionsTx"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "registerQueueDefinitionsTx"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "registerLeaseDefinitionsTx"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "registerRuntimeCatalogTx"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "new IndexDescriptor"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "registerObservabilityCatalogTx"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "registerWorkflowDefinitionsTx"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "workflow_launch_attempts_total"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "Optional.of(\"composition\")"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "new EntityInvariantDescriptor(\"valid_status\""
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "\"by_tenant_status\""
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "\"by_tenant_order\""
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "new GarbageCollectionPolicy(\"P30D\", \"tombstone\")"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "FieldType.STRING"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "FieldType.NAMED"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "FieldType.TIMESTAMP"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "FieldType.OPTIONAL"
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "\"OrderStatus\""
assert_file_contains "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java" "\"int?\""

cp "$SCRIPT_DIR/MetadataResolverFixture.java" "$TMPDIR/out-java/common/com/statespec/generated/MetadataResolverFixture.java"
cat > "$TMPDIR/out-java/common/com/statespec/backend/memory/MemoryBackendFixture.java" <<'EOF'
package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.FeatureFlag;
import com.statespec.backend.Json;
import com.statespec.backend.Lease;
import com.statespec.backend.Log;
import com.statespec.backend.Metric;
import com.statespec.backend.Queue;
import com.statespec.backend.Workflow;
import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Optional;

public final class MemoryBackendFixture
{
    public static void main(String[] args) throws Exception
    {
        var backend = new InMemoryBackend();
        var flags = new InMemoryFeatureFlagStore();
        var queues = new InMemoryQueueStore();
        var leases = new InMemoryLeaseStore();
        var workflows = new InMemoryWorkflowStore();
        var logs = new InMemoryLogSink();
        var metrics = new InMemoryMetricSink();

        var tx = backend.begin();
        flags.registerDefinitionTx(
            tx,
            new FeatureFlag.Definition(
                "new_scheduler", FeatureFlag.Type.BOOL, new FeatureFlag.Value.BoolValue(true),
                FeatureFlag.ScopeKind.SYSTEM, Optional.empty(), Optional.empty(), Optional.empty()
            )
        );
        queues.registerDefinitionTx(
            tx,
            new Queue.RegisterQueueDefinitionRequest(
                new Queue.QueueDefinition(
                    "workflow", "launch", Duration.ofMinutes(1), 3, Optional.empty(), "{}"
                )
            )
        );
        var leaseDefinitionId = new Lease.LeaseDefinitionId("workflow_step", 1L);
        leases.registerDefinitionTx(
            tx,
            new Lease.LeaseDefinition(
                leaseDefinitionId, "workflow/*", Duration.ofMinutes(1),
                Duration.ofSeconds(30), Optional.empty(), true
            )
        );
        workflows.registerDefinitionTx(
            tx,
            new Workflow.RegisterWorkflowDefinitionRequest(
                new Workflow.WorkflowDefinition(
                    "ProvisionService", 1L, "validate_request", Duration.ofMinutes(1), false,
                    List.of(
                        new Workflow.WorkflowStepDefinition(
                            "validate_request", Duration.ofSeconds(1), 3
                        )
                    ),
                    "{}"
                )
            )
        );
        logs.registerDefinitionTx(
            tx,
            new Log.Definition(
                "launch_decision", Log.Level.INFO, "workflow.launch.decision", List.of()
            )
        );
        metrics.registerDefinitionTx(
            tx,
            new Metric.Definition(
                "launch_attempts", Metric.Kind.COUNTER, "workflow_launch_attempts_total", "1",
                List.of()
            )
        );
        backend.commit(tx);

        var flag = flags.evaluate(
            backend,
            new FeatureFlag.EvaluationRequest(
                "new_scheduler",
                new FeatureFlag.EvaluationContext(
                    Optional.empty(), Optional.empty(), Optional.empty(), Optional.empty()
                )
            )
        );
        if (flag.asBool().isEmpty() || !flag.asBool().get())
        {
            throw new IllegalStateException("feature flag evaluation failed");
        }

        var now = Instant.ofEpochSecond(100);
        var message = queues.enqueue(
            backend,
            new Queue.EnqueueMessageRequest(
                "msg-1", "workflow", "launch", Optional.empty(), "{}"
            )
        );
        if (!message.status().equals("Pending"))
        {
            throw new IllegalStateException("queue enqueue failed");
        }
        var claimed = queues.claim(
            backend,
            new Queue.ClaimMessageRequest(
                "workflow", "launch", "worker-1", now, Duration.ofMinutes(1), 1
            )
        );
        if (claimed.size() != 1)
        {
            throw new IllegalStateException("queue claim failed");
        }

        var lease = leases.acquire(
            backend,
            new Lease.LeaseAcquireRequest(
                leaseDefinitionId, "workflow/msg-1", "worker-1", now
            )
        );
        if (!lease.acquired() || lease.lease().isEmpty())
        {
            throw new IllegalStateException("lease acquire failed");
        }

        workflows.start(
            backend,
            new Workflow.StartWorkflowRequest(
                "wf-1", "ProvisionService", 1L, "validate_request", "{}"
            )
        );
        var steps = workflows.claimSteps(
            backend,
            new Workflow.ClaimWorkflowStepRequest(
                "wf-1", "ProvisionService", 1L, "worker-1", now, Duration.ofMinutes(1), 1
            )
        );
        if (steps.size() != 1)
        {
            throw new IllegalStateException("workflow claim failed");
        }
        workflows.keepAliveStep(
            backend,
            new Workflow.KeepAliveWorkflowStepRequest(
                "wf-1", "worker-1", "validate_request", now, Duration.ofMinutes(1)
            )
        );

        logs.emit(
            backend,
            new Log.Event(
                "launch_decision", Log.Level.INFO, "workflow.launch.decision", Map.of()
            )
        );
        metrics.record(
            backend,
            new Metric.Sample(
                "launch_attempts", Metric.Kind.COUNTER, "workflow_launch_attempts_total", 1.0,
                "1", Map.of()
            )
        );
        if (logs.inspectEvents(backend).size() != 1 || metrics.inspectSamples(backend).size() != 1)
        {
            throw new IllegalStateException("observability inspect failed");
        }

        var appTx = backend.begin();
        backend.put(appTx, "orders", "order-1", Json.object(Map.of("status", Json.string("new"))));
        backend.commit(appTx);
        var readTx = backend.begin();
        if (backend.query(readTx, "orders", new Backend.Query.KeyPrefix("order-")).size() != 1)
        {
            throw new IllegalStateException("backend query failed");
        }
        backend.commit(readTx);
    }
}
EOF
(cd "$TMPDIR/out-java" && make check)
${JAVA:-java} -cp "$TMPDIR/out-java/build/classes" com.statespec.generated.MetadataResolverFixture
${JAVA:-java} -cp "$TMPDIR/out-java/build/classes" com.statespec.backend.memory.MemoryBackendFixture
