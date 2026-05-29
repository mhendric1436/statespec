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
APP_MANIFEST="$TMPDIR/expected-java-manifest.txt"

cat > "$APP_MANIFEST" <<'EOF'
Makefile
api/com/statespec/generated/ApiApplication.java
api/com/statespec/generated/ApiCodecs.java
api/com/statespec/generated/ApiDescriptors.java
api/com/statespec/generated/ApiDispatcher.java
api/com/statespec/generated/ApiHandlerRegistry.java
api/com/statespec/generated/ApiHandlers.java
api/com/statespec/generated/ApiMain.java
api/com/statespec/generated/ApiProcess.java
api/com/statespec/generated/ApiRoutes.java
api/com/statespec/generated/ApiServer.java
api/com/statespec/generated/ApiTransport.java
api/com/statespec/generated/EntityGcCatalog.java
api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java
api/com/statespec/generated/codecs/ApiCodecsProvisionCallbackRequest.java
api/com/statespec/generated/codecs/ApiCodecsProvisionCallbackResponse.java
api/com/statespec/generated/codecs/ApiCodecsStartProvisionRequest.java
api/com/statespec/generated/codecs/ApiCodecsStartProvisionResponse.java
api/com/statespec/generated/descriptors/Catalog.java
api/com/statespec/generated/descriptors/ReportProvisionReadyDescriptorModule.java
api/com/statespec/generated/descriptors/StartProvisionDescriptorModule.java
api/com/statespec/generated/shapes/ProvisionCallbackRequest.java
api/com/statespec/generated/shapes/ProvisionCallbackResponse.java
api/com/statespec/generated/shapes/ShapeCatalog.java
api/com/statespec/generated/shapes/StartProvisionRequest.java
api/com/statespec/generated/shapes/StartProvisionResponse.java
common/com/statespec/backend/Backend.java
common/com/statespec/backend/ExternalSystem.java
common/com/statespec/backend/FeatureFlag.java
common/com/statespec/backend/Json.java
common/com/statespec/backend/Lease.java
common/com/statespec/backend/Log.java
common/com/statespec/backend/Metric.java
common/com/statespec/backend/Queue.java
common/com/statespec/backend/SchemaCompatibility.java
common/com/statespec/backend/Workflow.java
common/com/statespec/backend/memory/InMemoryBackend.java
common/com/statespec/backend/memory/InMemoryTransaction.java
common/com/statespec/backend/runtime/Codec.java
common/com/statespec/backend/runtime/EntityGcRegistration.java
common/com/statespec/backend/runtime/EntityGcRepository.java
common/com/statespec/backend/runtime/EntityGcTypes.java
common/com/statespec/backend/runtime/EntityGcWorkers.java
common/com/statespec/backend/runtime/LeaseCodec.java
common/com/statespec/backend/runtime/LeaseStore.java
common/com/statespec/backend/runtime/QueueCodec.java
common/com/statespec/backend/runtime/QueueStore.java
common/com/statespec/backend/runtime/WorkflowCodec.java
common/com/statespec/backend/runtime/WorkflowStore.java
common/com/statespec/generated/ApiRequestContext.java
common/com/statespec/generated/ApiResponse.java
common/com/statespec/generated/Descriptors.java
common/com/statespec/generated/descriptors/CoreDescriptorModule.java
common/com/statespec/generated/descriptors/DescriptorCatalog.java
common/com/statespec/generated/descriptors/EventDescriptorModule.java
common/com/statespec/generated/descriptors/ExternalSystemDescriptorModule.java
common/com/statespec/generated/descriptors/RuntimeDescriptorModule.java
common/com/statespec/generated/descriptors/RuntimeLeaseRegistrationModule.java
common/com/statespec/generated/descriptors/RuntimeQueueRegistrationModule.java
common/com/statespec/generated/descriptors/RuntimeWorkflowRegistrationModule.java
common/com/statespec/generated/descriptors/types/ApiDescriptor.java
common/com/statespec/generated/descriptors/types/ApiRouteDescriptor.java
common/com/statespec/generated/descriptors/types/ApiServerDescriptor.java
common/com/statespec/generated/descriptors/types/EntityChildDescriptor.java
common/com/statespec/generated/descriptors/types/EntityDescriptor.java
common/com/statespec/generated/descriptors/types/EntityInvariantDescriptor.java
common/com/statespec/generated/descriptors/types/EntityOwnershipDescriptor.java
common/com/statespec/generated/descriptors/types/EntityRelationDescriptor.java
common/com/statespec/generated/descriptors/types/EntityStateDescriptor.java
common/com/statespec/generated/descriptors/types/EnumDescriptor.java
common/com/statespec/generated/descriptors/types/EnumMemberDescriptor.java
common/com/statespec/generated/descriptors/types/EventDescriptor.java
common/com/statespec/generated/descriptors/types/EventEnvelope.java
common/com/statespec/generated/descriptors/types/ExternalSystemDescriptor.java
common/com/statespec/generated/descriptors/types/ExternalSystemMetadataDescriptor.java
common/com/statespec/generated/descriptors/types/ExternalSystemMetadataMappingDescriptor.java
common/com/statespec/generated/descriptors/types/ExternalSystemPropertyDescriptor.java
common/com/statespec/generated/descriptors/types/FeatureFlagDefinition.java
common/com/statespec/generated/descriptors/types/GarbageCollectionPolicy.java
common/com/statespec/generated/descriptors/types/LeaseDefinition.java
common/com/statespec/generated/descriptors/types/LogDefinition.java
common/com/statespec/generated/descriptors/types/MetricDefinition.java
common/com/statespec/generated/descriptors/types/PolicyDescriptor.java
common/com/statespec/generated/descriptors/types/PolicyRuleDescriptor.java
common/com/statespec/generated/descriptors/types/QuotaDescriptor.java
common/com/statespec/generated/descriptors/types/ShapeDescriptor.java
common/com/statespec/generated/descriptors/types/ValueDescriptor.java
common/com/statespec/generated/descriptors/types/WorkerContext.java
common/com/statespec/generated/descriptors/types/WorkerDescriptor.java
common/com/statespec/generated/entities/DefaultEntityRepository.java
common/com/statespec/generated/entities/EntityCreateRequest.java
common/com/statespec/generated/entities/EntityDeleteRequest.java
common/com/statespec/generated/entities/EntityGetRequest.java
common/com/statespec/generated/entities/EntityKeyValue.java
common/com/statespec/generated/entities/EntityListByIndexRequest.java
common/com/statespec/generated/entities/EntityLookup.java
common/com/statespec/generated/entities/EntityRepository.java
common/com/statespec/generated/entities/EntityUpsertRequest.java
common/com/statespec/generated/entities/service_instance/Constants.java
common/com/statespec/generated/entities/service_instance/Gc.java
common/com/statespec/generated/entities/service_instance/Model.java
common/com/statespec/generated/entities/service_instance/Persistence.java
common/com/statespec/generated/entities/service_instance/Schema.java
common/com/statespec/generated/external/metadata/DefaultExternalSystemMetadataMappingApplicator.java
common/com/statespec/generated/external/metadata/DefaultExternalSystemOperatorMetadataRepository.java
common/com/statespec/generated/external/metadata/ExternalSystemCallRequest.java
common/com/statespec/generated/external/metadata/ExternalSystemCallResponse.java
common/com/statespec/generated/external/metadata/ExternalSystemClient.java
common/com/statespec/generated/external/metadata/ExternalSystemMetadata.java
common/com/statespec/generated/external/metadata/ExternalSystemMetadataMappingApplicator.java
common/com/statespec/generated/external/metadata/ExternalSystemMetadataMappingAssignment.java
common/com/statespec/generated/external/metadata/ExternalSystemMetadataMappingInputs.java
common/com/statespec/generated/external/metadata/ExternalSystemMetadataMappingOutput.java
common/com/statespec/generated/external/metadata/ExternalSystemMetadataMappingPlan.java
common/com/statespec/generated/external/metadata/ExternalSystemMetadataMissingMappingSource.java
common/com/statespec/generated/external/metadata/ExternalSystemOperatorMetadataApiHandler.java
common/com/statespec/generated/external/metadata/ExternalSystemOperatorMetadataDeleteRequest.java
common/com/statespec/generated/external/metadata/ExternalSystemOperatorMetadataDisableRequest.java
common/com/statespec/generated/external/metadata/ExternalSystemOperatorMetadataGetRequest.java
common/com/statespec/generated/external/metadata/ExternalSystemOperatorMetadataRepository.java
common/com/statespec/generated/external/metadata/ExternalSystemOperatorMetadataUpsertRequest.java
common/com/statespec/generated/runtime/RuntimeRegistration.java
common/com/statespec/generated/workflows/ProvisionServiceDescriptorModule.java
worker/com/statespec/generated/WorkerApplication.java
worker/com/statespec/generated/WorkerContexts.java
worker/com/statespec/generated/WorkerDescriptors.java
worker/com/statespec/generated/WorkerEntityGcCatalog.java
worker/com/statespec/generated/WorkerLeases.java
worker/com/statespec/generated/WorkerMain.java
worker/com/statespec/generated/WorkerProcess.java
worker/com/statespec/generated/WorkerQueues.java
worker/com/statespec/generated/WorkerRegistry.java
worker/com/statespec/generated/WorkerRuntime.java
worker/com/statespec/generated/WorkerWorkflows.java
worker/com/statespec/generated/WorkflowRunner.java
worker/com/statespec/generated/WorkflowStepHandlers.java
worker/com/statespec/generated/registry/ProvisionWorkerRegistry.java
worker/com/statespec/generated/worker/descriptors/Catalog.java
worker/com/statespec/generated/worker/descriptors/ProvisionWorkerDescriptorModule.java
worker/com/statespec/generated/workflows/ProvisionServiceWorkerModule.java
EOF

run_expect_status 0 "$CLI" generate bindings --lang java "$E2E_SPEC" --out "$TMPDIR/out-e2e-java"
assert_file_contains "$TMPDIR/out-e2e-java/api/com/statespec/generated/descriptors/Catalog.java" "apiRouteDescriptors"
assert_file_contains "$TMPDIR/out-e2e-java/api/com/statespec/generated/descriptors/Catalog.java" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-java/api/com/statespec/generated/descriptors/Catalog.java" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/descriptors/ExternalSystemDescriptorModule.java" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/descriptors/ExternalSystemDescriptorModule.java" "\"input.payment_id\""
assert_file_not_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "ExternalSystemOperatorMetadataApiHandler"
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/external/metadata/ExternalSystemOperatorMetadataApiHandler.java" "ExternalSystemOperatorMetadataApiHandler"
cp "$SCRIPT_DIR/ApiProcessFixture.java" "$TMPDIR/out-e2e-java/api/com/statespec/generated/ApiProcessFixture.java"
run_expect_status 0 make -C "$TMPDIR/out-e2e-java" build-api
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-e2e-java/build/classes" com.statespec.generated.ApiProcessFixture

run_expect_status 0 "$CLI" generate bindings --lang java "$API_ENTITY_SPEC" --out "$TMPDIR/out-api-entities-java"
assert_file_not_exists "$TMPDIR/out-api-entities-java/worker/com/statespec/generated/WorkerDescriptors.java"
assert_file_not_exists "$TMPDIR/out-api-entities-java/worker/com/statespec/generated/WorkerRegistry.java"
assert_file_not_contains "$TMPDIR/out-api-entities-java/Makefile" "build-worker"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Registry.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Registry.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Handlers.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Registry.java"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "new com.statespec.generated.entities.account.Persistence.DefaultAccountRepository()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java" "new com.statespec.generated.entities.project.Persistence.DefaultProjectRepository()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Handlers.java" "new com.statespec.generated.entities.task.Persistence.DefaultTaskRepository()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "backend.begin()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "repository.createTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "com.statespec.generated.entities.account.Constants.ACCOUNT_FIELD_CREATED_AT"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "com.statespec.generated.entities.account.Constants.ACCOUNT_FIELD_UPDATED_AT"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "com.statespec.generated.entities.account.Constants.ACCOUNT_FIELD_STATUS"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "var createTimestamp = java.time.Instant.now().toString();"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "document.put(com.statespec.generated.entities.account.Constants.ACCOUNT_FIELD_CREATED_AT, com.statespec.backend.Json.string(createTimestamp))"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "document.put(com.statespec.generated.entities.account.Constants.ACCOUNT_FIELD_UPDATED_AT, com.statespec.backend.Json.string(createTimestamp))"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "document.put(com.statespec.generated.entities.account.Constants.ACCOUNT_FIELD_CREATED_AT, com.statespec.backend.Json.string(java.time.Instant.now().toString()))"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "document.put(com.statespec.generated.entities.account.Constants.ACCOUNT_FIELD_STATUS, com.statespec.backend.Json.string(com.statespec.generated.entities.account.Constants.ACCOUNT_STATUS_ACTIVE))"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java" "missing required parent Account"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Handlers.java" "missing required parent Project"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "extractApiPathParameters"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "repository.getTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "repository.listByTenantAccountTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java" "repository.listByAccountStatusTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Handlers.java" "repository.listByProjectStatusTx"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/project/Persistence.java" "listByTenantProjectTx"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/task/Persistence.java" "listByTenantTaskTx"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/task/Persistence.java" "listByAccountPriorityTx"
assert_file_not_exists "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/AccountDescriptorModule.java"
assert_file_not_exists "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/ProjectDescriptorModule.java"
assert_file_not_exists "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/entities/TaskDescriptorModule.java"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Constants.java" "ACCOUNT_ENTITY_NAME = \"Account\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Constants.java" "ACCOUNT_FIELD_TENANT_ID = \"tenant_id\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Constants.java" "ACCOUNT_FIELD_CREATED_AT_TYPE_NAME = \"timestamp\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Constants.java" "ACCOUNT_FIELD_STATUS_TYPE_NAME = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Constants.java" "ACCOUNT_INDEX_BY_TENANT_ACCOUNT = \"by_tenant_account\""
assert_file_not_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Constants.java" "CreateAccountRequest"
assert_file_not_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Constants.java" "AccountResponse"
assert_file_not_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Constants.java" "decodeCreateAccountRequest"
assert_file_not_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Model.java" "ACCOUNT_ENTITY_NAME = \"Account\""
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Model.java" "new FieldDescriptor(ACCOUNT_FIELD_CREATED_AT, FieldType.TIMESTAMP, ACCOUNT_FIELD_CREATED_AT_TYPE_NAME, true)"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Model.java" "new FieldDescriptor(ACCOUNT_FIELD_STATUS, FieldType.STRING, ACCOUNT_FIELD_STATUS_TYPE_NAME, true)"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "document.put(com.statespec.generated.entities.account.Constants.ACCOUNT_FIELD_DISPLAY_NAME"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "body.put(com.statespec.generated.entities.account.Constants.ACCOUNT_FIELD_TENANT_ID"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "body.put(\"tenant_id\""
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Shapes.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Catalog.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Shapes.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Codecs.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Catalog.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Shapes.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Codecs.java"
assert_file_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Catalog.java"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Shapes.java" "record CreateAccountRequest"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Shapes.java" "new FieldDescriptor(Constants.ACCOUNT_ENTITY_PLURAL_NAME"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Shapes.java" "LIST_ACCOUNTS_RESPONSE_FIELD_ACCOUNTS = \"accounts\""
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Shapes.java" "\"accounts\""
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "import com.statespec.generated.entities.account.Constants;"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Codecs.java" "import com.statespec.generated.entities.project.Constants;"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Codecs.java" "import com.statespec.generated.entities.task.Constants;"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "decodeCreateAccountRequest"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "requireMember(request.body(), Constants.ACCOUNT_FIELD_DISPLAY_NAME)"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "body.put(Constants.ACCOUNT_FIELD_TENANT_ID"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "body.put(Constants.ACCOUNT_FIELD_ACCOUNT_ID"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "requireMember(request.body(), Constants.ACCOUNT_FIELD_STATUS)"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Constants.java" "ACCOUNT_ENTITY_PLURAL_NAME = \"accounts\""
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "body.put(Constants.ACCOUNT_ENTITY_PLURAL_NAME"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Codecs.java" "requireMember(request.body(), Constants.PROJECT_FIELD_ACCOUNT_ID)"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/project/Constants.java" "PROJECT_ENTITY_PLURAL_NAME = \"projects\""
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Codecs.java" "body.put(Constants.PROJECT_FIELD_PROJECT_ID"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Codecs.java" "requireMember(request.body(), Constants.TASK_FIELD_PROJECT_ID)"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/task/Constants.java" "TASK_ENTITY_PLURAL_NAME = \"tasks\""
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Codecs.java" "requireMember(request.body(), Constants.TASK_FIELD_PRIORITY)"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "requireMember(request.body(), \"display_name\")"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "body.put(\"tenant_id\""
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "\"account_id\""
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "\"accounts\""
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Codecs.java" "\"account_id\""
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Codecs.java" "\"projects\""
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Codecs.java" "\"project_id\""
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Codecs.java" "\"tasks\""
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "com.statespec.generated.Descriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "Model"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Codecs.java" "com.statespec.generated.Descriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Codecs.java" "Model"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Codecs.java" "com.statespec.generated.Descriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/task/Codecs.java" "Model"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Shapes.java" "record AccountResponse"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Catalog.java" "shapeDescriptors()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Catalog.java" "apiDescriptors()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Catalog.java" "apiRouteDescriptors()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Catalog.java" "handlerEntrypoints()"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Catalog.java" "com.statespec.generated.Descriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Catalog.java" "Model"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Catalog.java" "handleListAccountProjects"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Catalog.java" "handleListAccountProjects"
assert_file_not_exists "$TMPDIR/out-api-entities-java/api/com/statespec/generated/shapes/CreateAccountRequest.java"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/shapes/ShapeCatalog.java" "com.statespec.generated.entities.account.Catalog.shapeDescriptors()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/Catalog.java" "com.statespec.generated.entities.account.Catalog.apiDescriptors()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/Catalog.java" "com.statespec.generated.entities.project.Catalog.apiRouteDescriptors()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/CreateAccountDescriptorModule.java" "import com.statespec.generated.entities.account.Constants;"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/CreateAccountDescriptorModule.java" "Optional.of(\"/v1/tenants/{\" + Constants.ACCOUNT_FIELD_TENANT_ID"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/CreateAccountDescriptorModule.java" "com.statespec.generated.Descriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/CreateAccountDescriptorModule.java" "Model"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/CreateProjectDescriptorModule.java" "import com.statespec.generated.entities.project.Constants;"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/CreateProjectDescriptorModule.java" "Constants.PROJECT_FIELD_PROJECT_ID"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/CreateTaskDescriptorModule.java" "import com.statespec.generated.entities.task.Constants;"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/CreateTaskDescriptorModule.java" "Constants.TASK_FIELD_TASK_ID"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors" "*.java" "com.statespec.generated.Descriptors"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors" "*.java" "Model"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities" "Catalog.java" "com.statespec.generated.Descriptors"
assert_tree_files_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities" "Catalog.java" "Model"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/Catalog.java" "CreateAccountDescriptorModule.apiDescriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/Catalog.java" "CreateProjectDescriptorModule.apiDescriptors"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/descriptors/Catalog.java" "CreateTaskDescriptorModule.apiDescriptors"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiHandlerRegistry.java" "com.statespec.generated.entities.account.Catalog.handle"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Shapes.java" "CREATE_ACCOUNT_REQUEST_SHAPE_NAME = \"CreateAccountRequest\""
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Shapes.java" "ACCOUNT_RESPONSE_SHAPE_NAME = \"AccountResponse\""
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Codecs.java" "decodeCreateAccountRequest"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Shapes.java" "CREATE_ACCOUNT_REQUEST_FIELD_DISPLAY_NAME = \"display_name\""
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Shapes.java" "CREATE_ACCOUNT_REQUEST_FIELD_DISPLAY_NAME_TYPE_NAME = \"string\""
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Shapes.java" "new FieldDescriptor(Constants.ACCOUNT_FIELD_DISPLAY_NAME, FieldType.STRING, Constants.ACCOUNT_FIELD_DISPLAY_NAME_TYPE_NAME, true)"
assert_file_not_exists "$TMPDIR/out-api-entities-java/common/com/statespec/generated/descriptors/shapes/CreateAccountRequestDescriptorModule.java"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/account/Schema.java" "List.of(Constants.ACCOUNT_FIELD_TENANT_ID, Constants.ACCOUNT_FIELD_ACCOUNT_ID)"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/EntityRepository.java" "key.append(keyField).append('=').append(value.canonicalString()).append('\\n')"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/EntityRepository.java" "values.put(keyValue.field(), keyValue.value())"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "new ApiResponse(404"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/account/Handlers.java" "com.statespec.backend.Json.array(items)"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java" "repository.updateTx"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java" "invalid entity status transition"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java" "document.put(com.statespec.generated.entities.project.Constants.PROJECT_FIELD_UPDATED_AT"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java" "var requestedStatus = com.statespec.generated.entities.project.Constants.PROJECT_STATUS_DELETED"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java" "invalid entity delete transition"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java" "private static boolean projectStatusTransitionAllowed(String currentStatus, String requestedStatus)"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java" "projectStatusTransitionAllowed(currentStatus, requestedStatus)"
assert_file_not_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java" "var transitionAllowed"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/entities/project/Handlers.java" "new ApiResponse(204"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/project/Persistence.java" "updateTx"
assert_file_contains "$TMPDIR/out-api-entities-java/common/com/statespec/generated/entities/project/Constants.java" "PROJECT_STATUS_DELETED"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiMain.java" "import com.statespec.backend.runtime.EntityGcRegistration"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiMain.java" "EntityGcRegistration.registerEntityGcWorkers"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiMain.java" "EntityGcCatalog.descriptors()"
assert_file_contains "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiMain.java" "process.addEntityGcWorker"
cp "$SCRIPT_DIR/ApiPersistenceFixture.java" "$TMPDIR/out-api-entities-java/api/com/statespec/generated/ApiPersistenceFixture.java"
run_expect_status 0 make -C "$TMPDIR/out-api-entities-java" build-api
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-api-entities-java/build/classes" com.statespec.generated.ApiPersistenceFixture

run_expect_status 0 "$CLI" generate bindings --lang java "$WORKFLOW_ENTITY_SPEC" --out "$TMPDIR/out-workflow-entities-java"
assert_file_not_exists "$TMPDIR/out-workflow-entities-java/api/com/statespec/generated/ApiMain.java"
assert_file_contains "$TMPDIR/out-workflow-entities-java/worker/com/statespec/generated/WorkerMain.java" "import com.statespec.backend.runtime.EntityGcRegistration"
assert_file_contains "$TMPDIR/out-workflow-entities-java/worker/com/statespec/generated/WorkerMain.java" "EntityGcRegistration.registerEntityGcWorkers"
assert_file_contains "$TMPDIR/out-workflow-entities-java/worker/com/statespec/generated/WorkerMain.java" "WorkerEntityGcCatalog.descriptors()"
assert_file_contains "$TMPDIR/out-workflow-entities-java/worker/com/statespec/generated/WorkerMain.java" "runtime.addEntityGcWorker"

run_expect_status 0 "$CLI" validate "$APP_SPEC"
run_expect_status 0 "$CLI" generate bindings --lang java "$APP_SPEC" --out "$TMPDIR/out-app-java"
assert_file_manifest_equals "$TMPDIR/out-app-java" "$APP_MANIFEST"
assert_file_exists "$TMPDIR/out-app-java/common/com/statespec/generated/Descriptors.java"
assert_file_exists "$TMPDIR/out-app-java/api/com/statespec/generated/ApiDescriptors.java"
assert_file_exists "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerDescriptors.java"
assert_tree_files_not_contains "$TMPDIR/out-app-java/api/com/statespec/generated/descriptors" "*.java" "com.statespec.generated.Descriptors"
assert_tree_files_not_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/worker/descriptors" "*.java" "com.statespec.generated.Descriptors"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/descriptors/StartProvisionDescriptorModule.java" "\"ProvisionApi.StartProvision\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/worker/descriptors/ProvisionWorkerDescriptorModule.java" "\"ProvisionCommands.CreateRemoteService\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/worker/descriptors/ProvisionWorkerDescriptorModule.java" "\"ProvisionWorker\""
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiApplication.java" "class ApiApplication"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/codecs/ApiCodecsStartProvisionRequest.java" "decodeStartProvisionRequest"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/codecs/ApiCodecsStartProvisionResponse.java" "encodeStartProvisionResponse"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiHandlerRegistry.java" "class DefaultHandler"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiHandlers.java" "interface BusinessHandler"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiHandlerRegistry.java" "ApiHandlers.BusinessHandler businessHandler"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiHandlerRegistry.java" "businessHandler.handleStartProvision"
assert_file_not_exists "$TMPDIR/out-app-java/api/com/statespec/generated/entities/operations/Handlers.java"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "ApiProcess.Config.allServers"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "EntityGcRegistration.registerEntityGcWorkers"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "EntityGcCatalog.descriptors()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/EntityGcCatalog.java" "entities.service_instance.Gc.descriptor()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "addShutdownHook"
assert_file_not_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "ApiApplication.createDefault"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "public final class ApiProcess"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "public void requestStop()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "public synchronized void start()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "public int join()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "public synchronized boolean isRunning()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "boolean entityGcEnabled"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "public synchronized void addEntityGcWorker"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "startEntityGcWorkers()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiTransport.java" "interface ApiTransport"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiTransport.java" "return server.handle(routeName, context)"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiTransport.java" "final class LocalBlocking"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiTransport.java" "void requestStop()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiTransport.java" "stopRequested.await()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiProcess.java" "transport.requestStop()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "new ApiTransport.LocalBlocking()"
assert_file_contains "$TMPDIR/out-app-java/common/com/statespec/generated/entities/service_instance/Gc.java" "public static EntityGcTypes.Descriptor descriptor()"
assert_file_contains "$TMPDIR/out-app-java/common/com/statespec/backend/runtime/EntityGcTypes.java" "record Descriptor"
assert_file_not_contains "$TMPDIR/out-app-java/common/com/statespec/backend/runtime/EntityGcTypes.java" "Gc.descriptor()"
assert_file_contains "$TMPDIR/out-app-java/common/com/statespec/backend/runtime/EntityGcRegistration.java" "for (var descriptor : descriptors)"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "process.start()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiMain.java" "process.join()"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiServer.java" "class ApiServer"
assert_file_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiDispatcher.java" "dispatchApiRoute"
assert_file_not_contains "$TMPDIR/out-app-java/api/com/statespec/generated/ApiDispatcher.java" "dispatchApiOperationRoute"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRegistry.java" "findWorkerDescriptor"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRuntime.java" "class WorkerRuntime"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRuntime.java" "record Config(boolean entityGcEnabled"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRuntime.java" "startEntityGcWorkers"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerRuntime.java" "public void requestStop()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcess.java" "class WorkerProcess"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcess.java" "public synchronized void start()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcess.java" "public int join()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcess.java" "public synchronized boolean isRunning()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcess.java" "public void requestStop()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcess.java" "startWorkerLoops"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "WorkerProcess"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "WorkflowStepHandlers.DefaultHandler"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "EntityGcRegistration.registerEntityGcWorkers"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "WorkerEntityGcCatalog.descriptors()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerEntityGcCatalog.java" "entities.service_instance.Gc.descriptor()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "addShutdownHook"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "process.start()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerMain.java" "process.join()"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowRunner.java" "keepAliveStep"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.validate_request\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "DefaultHandler implements Handler"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.create_remote_service\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "\"ProvisionService.wait_for_remote_service\""
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowStepHandlers.java" "handleProvisionServiceValidateRequest"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkflowRunner.java" "ProvisionServiceWorkerModule.dispatchStep"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/workflows/ProvisionServiceWorkerModule.java" "handleProvisionServiceValidateRequest"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/workflows/ProvisionServiceWorkerModule.java" "Optional.of(\"create_remote_service\")"
assert_file_contains "$TMPDIR/out-app-java/worker/com/statespec/generated/workflows/ProvisionServiceWorkerModule.java" "Optional.of(\"wait_for_remote_service\")"
cp "$SCRIPT_DIR/ApiLinkingFixture.java" "$TMPDIR/out-app-java/api/com/statespec/generated/ApiLinkingFixture.java"
cp "$SCRIPT_DIR/RegistrationRestartFixture.java" "$TMPDIR/out-app-java/common/com/statespec/generated/RegistrationRestartFixture.java"
cp "$SCRIPT_DIR/WorkerLinkingFixture.java" "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerLinkingFixture.java"
cp "$SCRIPT_DIR/WorkerProcessFixture.java" "$TMPDIR/out-app-java/worker/com/statespec/generated/WorkerProcessFixture.java"
run_expect_status 0 make -C "$TMPDIR/out-app-java" check
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-app-java/build/classes" com.statespec.generated.ApiLinkingFixture
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-app-java/build/classes" com.statespec.generated.RegistrationRestartFixture
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-app-java/build/classes" com.statespec.generated.WorkerLinkingFixture
run_expect_status 0 "${JAVA:-java}" -cp "$TMPDIR/out-app-java/build/classes" com.statespec.generated.WorkerProcessFixture
