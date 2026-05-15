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

  entity Order {
    key tenant_id, order_id
    fields {
      tenant_id string
      order_id string
      created_at timestamp
      updated_at timestamp
      status string
      retry_count int?
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
        order_id string
      }
    }
  }

  lease OrderReconciler {
    resource "orders:reconciler"
    ttl PT30S
    renew_every PT10S
    fencing_token true
  }

  workflow OrderProcessing {
    version 2
    singleton false
    expected_execution_time PT60S
    start validate_order

    step validate_order {
      expected_execution_time PT10S
      max_retries 2
    }
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
  entity IncludedEntity {
    key included_id
    fields {
      included_id string
      created_at timestamp
      updated_at timestamp
      status string
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
assert_file_exists "$TMPDIR/out-cpp/backend.hpp"
assert_file_exists "$TMPDIR/out-cpp/lease.hpp"
assert_file_exists "$TMPDIR/out-cpp/observability.hpp"
assert_file_exists "$TMPDIR/out-cpp/queue.hpp"
assert_file_exists "$TMPDIR/out-cpp/workflow.hpp"
assert_file_exists "$TMPDIR/out-cpp/system_descriptors.hpp"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "FeatureFlagDefinition"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "feature_flag_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "GarbageCollectionPolicy"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EntityStateDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "entity_descriptors"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "CollectionDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "queue_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "lease_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "workflow_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EmailDispatch"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderReconciler"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderProcessing"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "validate_order"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "NewScheduler"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "MaxPendingOrders"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "struct LogDefinition"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "log_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "WorkflowLaunchDecision"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "workflow.launch.decision"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "struct MetricDefinition"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "metric_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "workflow_launch_attempts_total"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"P30D\""
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "\"tombstone\""

# Positive generation: Go.
run_expect_status 0 "$CLI" generate bindings --lang go "$SPEC" --out "$TMPDIR/out-go"
assert_output_contains "generated $TMPDIR/out-go/backend/backend.go"
assert_file_exists "$TMPDIR/out-go/backend/backend.go"
assert_file_exists "$TMPDIR/out-go/backend/lease.go"
assert_file_exists "$TMPDIR/out-go/backend/observability.go"
assert_file_exists "$TMPDIR/out-go/backend/queue.go"
assert_file_exists "$TMPDIR/out-go/backend/workflow.go"
assert_file_exists "$TMPDIR/out-go/backend/descriptors.go"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type FeatureFlagDefinition struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func FeatureFlagDefinitions() []FeatureFlagDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type GarbageCollectionPolicy struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type EntityStateDescriptor struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func EntityDescriptors() []EntityDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func CollectionDescriptors() []CollectionDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func QueueDefinitions() []QueueDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func LeaseDefinitions() []LeaseDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func WorkflowDefinitions() []WorkflowDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "EmailDispatch"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "OrderReconciler"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "WorkflowVersion: 2"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "validate_order"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "NewScheduler"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "DefaultValue: \"false\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type LogDefinition struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func LogDefinitions() []LogDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "EventName: \"workflow.launch.decision\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "type MetricDefinition struct"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func MetricDefinitions() []MetricDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "BackendName: \"workflow_launch_attempts_total\""
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "GarbageCollection: &GarbageCollectionPolicy{After: \"P30D\", Mode: \"tombstone\"}"

# Positive generation: Java.
run_expect_status 0 "$CLI" generate bindings --lang java "$SPEC" --out "$TMPDIR/out-java"
assert_output_contains "generated $TMPDIR/out-java/com/statespec/backend/Backend.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Backend.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Lease.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Observability.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Queue.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Workflow.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/generated/Descriptors.java"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "class Descriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record FeatureFlagDefinition"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record GarbageCollectionPolicy"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record EntityStateDescriptor"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "entityDescriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "featureFlagDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "queueDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "leaseDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workflowDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "EmailDispatch"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "OrderReconciler"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "2L"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "validate_order"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "NewScheduler"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record LogDefinition"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "logDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workflow.launch.decision"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "record MetricDefinition"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "metricDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workflow_launch_attempts_total"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "new GarbageCollectionPolicy(\"P30D\", \"tombstone\")"

# Positive generation: Rust.
run_expect_status 0 "$CLI" generate bindings --lang rust "$SPEC" --out "$TMPDIR/out-rust"
assert_output_contains "generated $TMPDIR/out-rust/backend.rs"
assert_file_exists "$TMPDIR/out-rust/backend.rs"
assert_file_exists "$TMPDIR/out-rust/lease.rs"
assert_file_exists "$TMPDIR/out-rust/observability.rs"
assert_file_exists "$TMPDIR/out-rust/queue.rs"
assert_file_exists "$TMPDIR/out-rust/workflow.rs"
assert_file_exists "$TMPDIR/out-rust/descriptors.rs"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct FeatureFlagDefinition"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn feature_flag_definitions() -> Vec<FeatureFlagDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct GarbageCollectionPolicy"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct EntityStateDescriptor"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn entity_descriptors() -> Vec<EntityDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn collection_descriptors() -> Vec<CollectionDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn queue_definitions() -> Vec<QueueDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn lease_definitions() -> Vec<LeaseDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn workflow_definitions() -> Vec<WorkflowDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "EmailDispatch"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "OrderReconciler"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "workflow_version: 2"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "validate_order"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "NewScheduler"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct LogDefinition"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn log_definitions() -> Vec<LogDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "event_name: \"workflow.launch.decision\".to_string()"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub struct MetricDefinition"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn metric_definitions() -> Vec<MetricDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "backend_name: \"workflow_launch_attempts_total\".to_string()"
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
assert_output_contains "generated generated/cpp/backend.hpp"
assert_file_exists "generated/cpp/backend.hpp"
assert_file_exists "generated/cpp/system_descriptors.hpp"
rm -rf generated/cpp
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang go "$SPEC"
assert_output_contains "generated generated/go/backend/backend.go"
assert_file_exists "generated/go/backend/backend.go"
assert_file_exists "generated/go/backend/descriptors.go"
rm -rf generated/go
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang java "$SPEC"
assert_output_contains "generated generated/java/com/statespec/backend/Backend.java"
assert_file_exists "generated/java/com/statespec/backend/Backend.java"
assert_file_exists "generated/java/com/statespec/generated/Descriptors.java"
rm -rf generated/java
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang rust "$SPEC"
assert_output_contains "generated generated/rust/backend.rs"
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
