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
  entity Order {
    key tenant_id, order_id
    fields {
      tenant_id string
      order_id string
      status string
      retry_count int?
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
assert_file_exists "$TMPDIR/out-cpp/queue.hpp"
assert_file_exists "$TMPDIR/out-cpp/workflow.hpp"
assert_file_exists "$TMPDIR/out-cpp/system_descriptors.hpp"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "CollectionDescriptor"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "queue_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "lease_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "workflow_definitions"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "EmailDispatch"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderReconciler"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "OrderProcessing"
assert_file_contains "$TMPDIR/out-cpp/system_descriptors.hpp" "validate_order"

# Positive generation: Go.
run_expect_status 0 "$CLI" generate bindings --lang go "$SPEC" --out "$TMPDIR/out-go"
assert_output_contains "generated $TMPDIR/out-go/backend/backend.go"
assert_file_exists "$TMPDIR/out-go/backend/backend.go"
assert_file_exists "$TMPDIR/out-go/backend/lease.go"
assert_file_exists "$TMPDIR/out-go/backend/queue.go"
assert_file_exists "$TMPDIR/out-go/backend/workflow.go"
assert_file_exists "$TMPDIR/out-go/backend/descriptors.go"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func CollectionDescriptors() []CollectionDescriptor"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func QueueDefinitions() []QueueDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func LeaseDefinitions() []LeaseDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "func WorkflowDefinitions() []WorkflowDefinition"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "EmailDispatch"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "OrderReconciler"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "WorkflowVersion: 2"
assert_file_contains "$TMPDIR/out-go/backend/descriptors.go" "validate_order"

# Positive generation: Java.
run_expect_status 0 "$CLI" generate bindings --lang java "$SPEC" --out "$TMPDIR/out-java"
assert_output_contains "generated $TMPDIR/out-java/com/statespec/backend/Backend.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Backend.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Lease.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Queue.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/backend/Workflow.java"
assert_file_exists "$TMPDIR/out-java/com/statespec/generated/Descriptors.java"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "class Descriptors"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "queueDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "leaseDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "workflowDefinitions"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "EmailDispatch"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "OrderReconciler"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "2L"
assert_file_contains "$TMPDIR/out-java/com/statespec/generated/Descriptors.java" "validate_order"

# Positive generation: Rust.
run_expect_status 0 "$CLI" generate bindings --lang rust "$SPEC" --out "$TMPDIR/out-rust"
assert_output_contains "generated $TMPDIR/out-rust/backend.rs"
assert_file_exists "$TMPDIR/out-rust/backend.rs"
assert_file_exists "$TMPDIR/out-rust/lease.rs"
assert_file_exists "$TMPDIR/out-rust/queue.rs"
assert_file_exists "$TMPDIR/out-rust/workflow.rs"
assert_file_exists "$TMPDIR/out-rust/descriptors.rs"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn collection_descriptors() -> Vec<CollectionDescriptor>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn queue_definitions() -> Vec<QueueDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn lease_definitions() -> Vec<LeaseDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "pub fn workflow_definitions() -> Vec<WorkflowDefinition>"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "EmailDispatch"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "OrderReconciler"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "workflow_version: 2"
assert_file_contains "$TMPDIR/out-rust/descriptors.rs" "validate_order"

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
