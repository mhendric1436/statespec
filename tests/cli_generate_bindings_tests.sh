#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR" generated/cpp generated/go generated/java generated/rust
    rmdir generated 2>/dev/null || true
}
trap cleanup EXIT

SPEC="$TMPDIR/minimal.sspec"
cat > "$SPEC" <<'SSPEC'
system Demo {
  tenant scoped_by tenant_id
  system_tenant configured

  feature_flag NewScheduler {
    type bool
    default false
    scope tenant
    owner "platform"
    description "Route order processing through the new scheduler"
    expires "2026-12-31"
  }

  feature_flag MaxPendingOrders {
    type int
    default 100
    scope tenant
  }

  log WorkflowLaunchDecision {
    level info
    event_name "workflow.launch.decision"
    fields {
      tenant_id string
      order_id string
      decision string
    }
  }

  metric WorkflowLaunchAttempts {
    kind counter
    name "workflow_launch_attempts_total"
    unit count
    labels {
      tenant_id string
      workflow_name string
      decision string
    }
  }

  value OrderAmount: decimal where OrderAmount >= 0;

  enum OrderStatus {
    Pending = "pending"
    Active = "active"
    Failed = "failed"
  }

  event OrderAccepted {
    fields {
      order_id string
      amount OrderAmount
      status OrderStatus
    }
  }

  namespace Billing {
    external_system Stripe {
      owner: "payments"
      endpoint: "https://api.stripe.test"
      metadata {
        entity ExternalSystemEndpoint
        profile_field profile
        required_fields [base_url, auth_ref, timeout_ms]
      }
    }
  }

  entity ExternalSystemEndpoint {
    key tenant_id, external_system_id, profile
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      external_system_id string
      profile string
      base_url string
      auth_ref string
      timeout_ms int
      retry_policy string
    }
    state_machine {
      state Active
      state Deleted {
        terminal: true
        garbage_collection {
          after: P30D
          mode: tombstone
        }
      }
      initial Active
      terminal [Deleted]
      Active -> Deleted
    }
  }

  shape StartOrderProcessingRequest {
    tenant_id string
    order_id string
  }

  shape StartOrderProcessingResponse {
    accepted bool
  }

  entity Account {
    key tenant_id, account_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    children {
      orders: Order by account_id
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      account_id string
    }
    state_machine {
      state Active
      initial Active
    }
  }

  entity Order {
    key tenant_id, order_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    relations {
      parent account_id: ref<Account> {
        kind: composition
        on_parent_delete: block
        parent_must_be_in: [Active]
        unique_within_parent: [order_id]
      }
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      order_id string
      account_id string
      retry_count int?
    }
    invariants {
      valid_status: status != ""
    }
    indexes {
      index by_tenant_status on tenant_id, status
      unique by_tenant_order on tenant_id, order_id
    }
    state_machine {
      state Pending
      state Active
      state Failed {
        terminal: true
        garbage_collection {
          after: P30D
          mode: tombstone
        }
      }
      initial Pending
      terminal Failed
      Pending -> Active
      Pending -> Failed
    }
  }

  queue EmailDispatch {
    namespace workflow_ns
    channel email
    visibility_timeout PT30S
    max_attempts 3
    message SendConfirmation {
      idempotency_key message_id
      payload {
        message_id string
        tenant_id string
        order_id string
      }
    }
  }

  lease OrderReconciler {
    resource "orders:reconciler"
    ttl PT30S
    renew_every PT10S
    holder worker_id
    fencing_token true
    max_ttl PT5M
  }

  workflow OrderProcessing {
    version 2
    on OrderAccepted
    singleton false
    expected_execution_time PT60S
    start validate_order

    step validate_order {
      expected_execution_time PT10S
      max_retries 2
      emit OrderAccepted;
    }
  }

  worker OrderWorker {
    singleton true
    lease OrderReconciler
    polls EmailDispatch.SendConfirmation
    executes OrderProcessing
    concurrency 4
  }

  api StartOrderProcessing {
    method POST
    path "/v1/tenants/{tenantId}/orders/{order_id}/start"
    input StartOrderProcessingRequest
    output StartOrderProcessingResponse
    starts workflow OrderProcessing
  }

  api_server OrderApi {
    serves StartOrderProcessing
    concurrency 16
  }

  policy OrderAccess {
    tenant scoped_by tenant_id
    allow StartOrderProcessing when caller.role == operator;
    quota starts_per_minute: 60;
    audit StartOrderProcessing;
  }
}
SSPEC

NO_SYSTEM_SPEC="$TMPDIR/no-system.sspec"
cat > "$NO_SYSTEM_SPEC" <<'SSPEC'
version "0.1"
SSPEC

INCLUDED_SPEC="$TMPDIR/included-bindings.sspec"
cat > "$INCLUDED_SPEC" <<'SSPEC'
statespec 0.1;

system IncludedBindings {
  tenant scoped_by tenant_id
  system_tenant configured

  entity IncludedEntity {
    key tenant_id, included_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      included_id string
    }
    state_machine {
      state Active
      initial Active
    }
  }

  queue IncludedQueue {
    namespace included_ns
    channel included
    visibility_timeout PT30S
    max_attempts 3
    message IncludedMessage {
      idempotency_key message_id
      payload {
        message_id string
        tenant_id string
        included_id string
      }
    }
  }

  workflow IncludedWorkflow {
    version 1
    singleton false
    expected_execution_time PT30S
    start included_step
    step included_step {
      expected_execution_time PT10S
      max_retries 1
    }
  }
}
SSPEC

INCLUDE_ROOT_SPEC="$TMPDIR/include-bindings-root.sspec"
cat > "$INCLUDE_ROOT_SPEC" <<'SSPEC'
statespec 0.1;
include "./included-bindings.sspec";

system IncludeBindingsRoot {
  feature_flag IncludedEntityLaunch {
    type bool
    default true
    scope entity IncludedEntity
  }
}
SSPEC

run_expect_status() {
    expected="$1"
    shift
    output="$TMPDIR/output.txt"
    set +e
    "$@" > "$output" 2>&1
    status="$?"
    set -e
    if [ "$status" -ne "$expected" ]; then
        echo "expected status $expected, got $status for: $*" >&2
        cat "$output" >&2
        exit 1
    fi
}

assert_output_contains() {
    needle="$1"
    if ! grep -F -- "$needle" "$TMPDIR/output.txt" >/dev/null 2>&1; then
        echo "expected output to contain: $needle" >&2
        cat "$TMPDIR/output.txt" >&2
        exit 1
    fi
}

assert_file_exists() {
    path="$1"
    if [ ! -f "$path" ]; then
        echo "expected generated file: $path" >&2
        cat "$TMPDIR/output.txt" >&2 || true
        exit 1
    fi
}

assert_file_contains() {
    path="$1"
    needle="$2"
    if ! grep -F -- "$needle" "$path" >/dev/null 2>&1; then
        echo "expected $path to contain: $needle" >&2
        cat "$path" >&2 || true
        exit 1
    fi
}

# Help.
run_expect_status 0 "$CLI" help
assert_output_contains "usage:"
assert_output_contains "statespec help"
assert_output_contains "statespec fmt [--check] <file.sspec>"
assert_output_contains "statespec generate bindings --lang <cpp|go|java|rust>"

run_expect_status 0 "$CLI" --help
assert_output_contains "statespec validate <file.sspec>"

run_expect_status 0 "$CLI" -h
assert_output_contains "statespec ast <file.sspec>"

# Positive generation: C++.
run_expect_status 0 "$CLI" generate bindings --lang cpp "$SPEC" --out "$TMPDIR/out-cpp"
assert_output_contains "generated $TMPDIR/out-cpp/backend.hpp"
assert_file_exists "$TMPDIR/out-cpp/json.hpp"
assert_file_exists "$TMPDIR/out-cpp/backend.hpp"
assert_file_exists "$TMPDIR/out-cpp/feature_flag.hpp"
assert_file_exists "$TMPDIR/out-cpp/lease.hpp"
assert_file_exists "$TMPDIR/out-cpp/log.hpp"
assert_file_exists "$TMPDIR/out-cpp/metric.hpp"
assert_file_exists "$TMPDIR/out-cpp/queue.hpp"
assert_file_exists "$TMPDIR/out-cpp/workflow.hpp"
assert_file_exists "$TMPDIR/out-cpp/system_descriptors.hpp"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "FeatureFlagDefinition"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "feature_flag_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ShapeDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "shape_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "struct StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "std::string tenant_id"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "NamespaceDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "namespace_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ValueDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "value_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EnumDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "enum_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EventDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "event_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "struct EventEnvelope"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "make_order_accepted_event"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ExternalSystemDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ExternalSystemMetadataDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "std::optional<std::string> tenant_field"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "std::vector<std::string> key_fields"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "external_system_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ApiDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "api_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "api_server_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ApiRequestContext"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ApiResponse"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "class IApiHandler"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "api_route_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"OrderApi\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"OrderApi.StartOrderProcessing\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "WorkerDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "WorkerContext"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "class IWorker"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "worker_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "worker_contexts"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"OrderWorker\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"EmailDispatch.SendConfirmation\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "PolicyDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "policy_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "GarbageCollectionPolicy"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EntityStateDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EntityOwnershipDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EntityRelationDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EntityChildDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EntityInvariantDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "entity_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "CollectionDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "IndexDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "queue_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "lease_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "workflow_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EmailDispatch"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderReconciler"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderProcessing"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "validate_order"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "NewScheduler"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "MaxPendingOrders"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderAmount"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderStatus"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderAccepted"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "Billing.Stripe"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "StartOrderProcessing"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "/v1/tenants/{tenantId}/orders/{order_id}/start"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderAccess"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "struct LogDefinition"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "log_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "WorkflowLaunchDecision"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "workflow.launch.decision"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "struct MetricDefinition"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "metric_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "ensure_system_collections"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "register_feature_flag_definitionsTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "create_queue_definitionsTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "register_lease_definitionsTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "register_runtime_catalogTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "register_observability_catalogTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "register_workflow_definitionsTx"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "workflow_launch_attempts_total"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"composition\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"account_id\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"valid_status\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"by_tenant_status\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"by_tenant_order\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"P30D\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"tombstone\""

# Positive generation: Go.
run_expect_status 0 "$CLI" generate bindings --lang go "$SPEC" --out "$TMPDIR/out-go"
assert_output_contains "generated $TMPDIR/out-go/backend/backend.go"
assert_file_exists "$TMPDIR/out-go/backend/json.go"
assert_file_exists "$TMPDIR/out-go/backend/backend.go"
assert_file_exists "$TMPDIR/out-go/backend/feature_flag.go"
assert_file_exists "$TMPDIR/out-go/backend/lease.go"
assert_file_exists "$TMPDIR/out-go/backend/log.go"
assert_file_exists "$TMPDIR/out-go/backend/metric.go"
assert_file_exists "$TMPDIR/out-go/backend/queue.go"
assert_file_exists "$TMPDIR/out-go/backend/workflow.go"
assert_file_exists "$TMPDIR/out-go/backend/descriptors.go"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type FeatureFlagDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func FeatureFlagDefinitions() []FeatureFlagDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ShapeDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ShapeDescriptors() []ShapeDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type StartOrderProcessingRequest struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "TenantId string"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type NamespaceDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func NamespaceDescriptors() []NamespaceDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ValueDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ValueDescriptors() []ValueDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EnumDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func EnumDescriptors() []EnumDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EventDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func EventDescriptors() []EventDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EventEnvelope struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func NewOrderAcceptedEvent"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ExternalSystemDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ExternalSystemMetadataDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "TenantField *string"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "KeyFields []string"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "TenantField: stringPtr(\"tenant_id\")"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ExternalSystemDescriptors() []ExternalSystemDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ApiDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ApiDescriptors() []ApiDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ApiServerDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ApiServerDescriptors() []ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type ApiRouteDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type APIRequestContext struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type APIResponse struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type APIHandler interface"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func ApiRouteDescriptors() []ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Name: \"OrderApi\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Serves: []string{\"StartOrderProcessing\"}"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Name: \"OrderApi.StartOrderProcessing\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "ServerName: \"OrderApi\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type WorkerDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type WorkerContext struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type Worker interface"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func WorkerDescriptors() []WorkerDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func WorkerContexts() []WorkerContext"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Name: \"OrderWorker\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Polls: stringPtr(\"EmailDispatch.SendConfirmation\")"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type PolicyDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func PolicyDescriptors() []PolicyDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type GarbageCollectionPolicy struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EntityStateDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EntityOwnershipDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EntityRelationDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EntityChildDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EntityInvariantDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func EntityDescriptors() []EntityDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func CollectionDescriptors() []CollectionDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Indexes: []IndexDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func QueueDefinitions() []QueueDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func LeaseDefinitions() []LeaseDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func WorkflowDefinitions() []WorkflowDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "EmailDispatch"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "OrderReconciler"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "WorkflowVersion: 2"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "validate_order"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "NewScheduler"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "DefaultValue: \"false\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "StartOrderProcessingRequest"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "OrderAmount"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "OrderStatus"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "OrderAccepted"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Billing.Stripe"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "StartOrderProcessing"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Path: stringPtr(\"/v1/tenants/{tenantId}/orders/{order_id}/start\")"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "OrderAccess"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type LogDefinition struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func LogDefinitions() []LogDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "EventName: \"workflow.launch.decision\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type MetricDefinition struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func MetricDefinitions() []MetricDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func EnsureSystemCollections(ctx context.Context, backend Backend) error"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func RegisterFeatureFlagDefinitionsTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func CreateQueueDefinitionsTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func RegisterLeaseDefinitionsTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func RegisterRuntimeCatalogTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func RegisterObservabilityCatalogTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func RegisterWorkflowDefinitionsTx"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "BackendName: \"workflow_launch_attempts_total\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Metadata: JSONObject(map[string]JSON{})"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "RelationKind: stringPtr(\"composition\")"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Name: \"valid_status\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Name: \"by_tenant_status\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "Unique: true"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "GarbageCollection: &GarbageCollectionPolicy{After: \"P30D\", Mode: \"tombstone\"}"

# Positive generation: Java.
run_expect_status 0 "$CLI" generate bindings --lang java "$SPEC" --out "$TMPDIR/out-java"
assert_output_contains "generated $TMPDIR/out-java/com/statespec/backend/Backend.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Json.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Backend.java"
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
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Optional<String> tenantField"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "List<String> keyFields"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "Optional.of(\"tenant_id\")"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiRequestContext"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record ApiResponse"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "interface ApiHandler"
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

# Positive generation: Rust.
run_expect_status 0 "$CLI" generate bindings --lang rust "$SPEC" --out "$TMPDIR/out-rust"
assert_output_contains "generated $TMPDIR/out-rust/backend.rs"
assert_file_exists "$TMPDIR/out-rust/json.rs"
assert_file_exists "$TMPDIR/out-rust/backend.rs"
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
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub tenant_field: Option<String>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub key_fields: Vec<String>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "tenant_field: Some(\"tenant_id\".to_string())"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn external_system_descriptors() -> Vec<ExternalSystemDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn api_descriptors() -> Vec<ApiDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiServerDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn api_server_descriptors() -> Vec<ApiServerDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiRouteDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiRequestContext"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct ApiResponse"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub trait ApiHandler"
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

# Include composition is used by binding generation.
run_expect_status 0 "$CLI" generate bindings --lang cpp "$INCLUDE_ROOT_SPEC" --out "$TMPDIR/out-include-cpp"
assert_output_contains "generated $TMPDIR/out-include-cpp/system_descriptors.hpp"
assert_file_exists "$TMPDIR/out-include-cpp/system_descriptors.hpp"
assert_file_contains "$TMPDIR/out-include-cpp/system_descriptors.hpp" "IncludedEntity"
assert_file_contains "$TMPDIR/out-include-cpp/system_descriptors.hpp" "IncludedQueue"
assert_file_contains "$TMPDIR/out-include-cpp/system_descriptors.hpp" "IncludedWorkflow"
assert_file_contains "$TMPDIR/out-include-cpp/system_descriptors.hpp" "IncludedEntityLaunch"

# Default output directories.
run_expect_status 0 "$CLI" generate bindings --lang cpp "$SPEC"
assert_output_contains "generated generated/cpp/json.hpp"
assert_file_exists "generated/cpp/backend.hpp"
assert_file_exists "generated/cpp/system_descriptors.hpp"
rm -rf generated/cpp
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang go "$SPEC"
assert_output_contains "generated generated/go/backend/json.go"
assert_file_exists "generated/go/backend/backend.go"
assert_file_exists "generated/go/backend/descriptors.go"
rm -rf generated/go
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang java "$SPEC"
assert_output_contains "generated generated/java/com/statespec/backend/Json.java"
assert_file_exists "generated/java/com/statespec/backend/Backend.java"
assert_file_exists "generated/java/com/statespec/generated/Descriptors.java"
rm -rf generated/java
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang rust "$SPEC"
assert_output_contains "generated generated/rust/json.rs"
assert_file_exists "generated/rust/backend.rs"
assert_file_exists "generated/rust/descriptors.rs"
rm -rf generated/rust
rmdir generated 2>/dev/null || true

# --lang is required.
run_expect_status 2 "$CLI" generate bindings "$SPEC"
assert_output_contains "generate bindings requires --lang <cpp|go|java|rust>"

# --lang requires a value.
run_expect_status 2 "$CLI" generate bindings --lang
assert_output_contains "--lang requires one of: cpp|go|java|rust"

# --lang must be supported.
run_expect_status 2 "$CLI" generate bindings --lang python "$SPEC"
assert_output_contains "unsupported binding language 'python'"
assert_output_contains "cpp|go|java|rust"

# Input file is required.
run_expect_status 2 "$CLI" generate bindings --lang cpp
assert_output_contains "generate bindings requires an input .sspec file"

# --out requires a value.
run_expect_status 2 "$CLI" generate bindings --lang cpp "$SPEC" --out
assert_output_contains "--out requires a directory"

# Extra positional arguments are rejected.
run_expect_status 2 "$CLI" generate bindings --lang cpp "$SPEC" extra
assert_output_contains "unexpected argument for generate bindings: extra"

# A system declaration is required for binding generation.
run_expect_status 1 "$CLI" generate bindings --lang cpp "$NO_SYSTEM_SPEC" --out "$TMPDIR/no-system"
assert_output_contains "SSPEC0201"
assert_output_contains "expected system declaration"

# Generic generation is not supported.
run_expect_status 2 "$CLI" generate "$SPEC" legacy
assert_output_contains "unsupported generate kind: $SPEC"

run_expect_status 2 "$CLI" generate legacy "$SPEC"
assert_output_contains "unsupported generate kind: legacy"
