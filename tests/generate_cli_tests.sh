#!/usr/bin/env sh
set -eu

CLI="${1:-build/bin/statespec}"
EXAMPLE="examples/order-system.sspec"
OUT_DIR="build/test-generated"

if [ ! -x "$CLI" ]; then
  echo "test failed: CLI not found or not executable: $CLI" >&2
  exit 1
fi

if [ ! -f "$EXAMPLE" ]; then
  echo "test failed: missing example file: $EXAMPLE" >&2
  exit 1
fi

rm -rf "$OUT_DIR"

GENERATE_OUTPUT="$($CLI generate "$EXAMPLE" wf --out "$OUT_DIR")"
printf '%s\n' "$GENERATE_OUTPUT" | grep -q "generated $OUT_DIR/wf-manifest.yaml"
printf '%s\n' "$GENERATE_OUTPUT" | grep -q "generated $OUT_DIR/wf_workflows.hpp"
printf '%s\n' "$GENERATE_OUTPUT" | grep -q "generated $OUT_DIR/wf_metadata.cpp"

test -f "$OUT_DIR/wf-manifest.yaml"
test -f "$OUT_DIR/wf_workflows.hpp"
test -f "$OUT_DIR/wf_metadata.cpp"
grep -q "target: wf" "$OUT_DIR/wf-manifest.yaml"
grep -q "OrderProcessing" "$OUT_DIR/wf-manifest.yaml"
grep -q "struct WorkflowMetadata" "$OUT_DIR/wf_workflows.hpp"
grep -q "struct WorkflowStepMetadata" "$OUT_DIR/wf_workflows.hpp"
grep -q "find_workflow" "$OUT_DIR/wf_metadata.cpp"
grep -q "find_step" "$OUT_DIR/wf_metadata.cpp"
grep -q "start_step" "$OUT_DIR/wf_metadata.cpp"

rm -rf "$OUT_DIR"

GENERATE_MT_OUTPUT="$($CLI generate "$EXAMPLE" mt --out "$OUT_DIR")"
printf '%s\n' "$GENERATE_MT_OUTPUT" | grep -q "generated $OUT_DIR/mt-manifest.yaml"
printf '%s\n' "$GENERATE_MT_OUTPUT" | grep -q "generated $OUT_DIR/mt_entities.hpp"
printf '%s\n' "$GENERATE_MT_OUTPUT" | grep -q "generated $OUT_DIR/mt_metadata.cpp"
printf '%s\n' "$GENERATE_MT_OUTPUT" | grep -q "generated $OUT_DIR/mt-state-machines.yaml"

test -f "$OUT_DIR/mt-manifest.yaml"
test -f "$OUT_DIR/mt_entities.hpp"
test -f "$OUT_DIR/mt_metadata.cpp"
test -f "$OUT_DIR/mt-state-machines.yaml"

grep -q "cpp_type: std::optional<std::chrono::system_clock::time_point>" "$OUT_DIR/mt-manifest.yaml"
grep -q "struct Order" "$OUT_DIR/mt_entities.hpp"
grep -q "std::optional<std::chrono::system_clock::time_point> updated_at" "$OUT_DIR/mt_entities.hpp"
grep -q "EntityMetadata" "$OUT_DIR/mt_metadata.cpp"
grep -q "state_machines:" "$OUT_DIR/mt-state-machines.yaml"

rm -rf "$OUT_DIR"

GENERATE_DL_OUTPUT="$($CLI generate "$EXAMPLE" dl --out "$OUT_DIR")"
printf '%s\n' "$GENERATE_DL_OUTPUT" | grep -q "generated $OUT_DIR/dl-manifest.yaml"
printf '%s\n' "$GENERATE_DL_OUTPUT" | grep -q "generated $OUT_DIR/dl_leases.hpp"
printf '%s\n' "$GENERATE_DL_OUTPUT" | grep -q "generated $OUT_DIR/dl_metadata.cpp"

test -f "$OUT_DIR/dl-manifest.yaml"
test -f "$OUT_DIR/dl_leases.hpp"
test -f "$OUT_DIR/dl_metadata.cpp"

grep -q "fencing_token: true" "$OUT_DIR/dl-manifest.yaml"
grep -q "struct LeaseMetadata" "$OUT_DIR/dl_leases.hpp"
grep -q "struct WorkerLeaseBinding" "$OUT_DIR/dl_leases.hpp"
grep -q "OrderWorkflowLease" "$OUT_DIR/dl_metadata.cpp"
grep -q "find_lease" "$OUT_DIR/dl_metadata.cpp"

rm -rf "$OUT_DIR"

GENERATE_QU_OUTPUT="$($CLI generate "$EXAMPLE" qu --out "$OUT_DIR")"
printf '%s\n' "$GENERATE_QU_OUTPUT" | grep -q "generated $OUT_DIR/qu-manifest.yaml"
printf '%s\n' "$GENERATE_QU_OUTPUT" | grep -q "generated $OUT_DIR/qu_messages.hpp"
printf '%s\n' "$GENERATE_QU_OUTPUT" | grep -q "generated $OUT_DIR/qu_metadata.cpp"

test -f "$OUT_DIR/qu-manifest.yaml"
test -f "$OUT_DIR/qu_messages.hpp"
test -f "$OUT_DIR/qu_metadata.cpp"

grep -q "payload_struct: OrderEventsOrderValidatedPayload" "$OUT_DIR/qu-manifest.yaml"
grep -q "struct OrderEventsOrderValidatedPayload" "$OUT_DIR/qu_messages.hpp"
grep -q "std::string message_id" "$OUT_DIR/qu_messages.hpp"
grep -q "idempotency_key_field" "$OUT_DIR/qu_metadata.cpp"

rm -rf "$OUT_DIR"

GENERATE_OPENAPI_OUTPUT="$($CLI generate "$EXAMPLE" openapi --out "$OUT_DIR")"
printf '%s\n' "$GENERATE_OPENAPI_OUTPUT" | grep -q "generated $OUT_DIR/openapi.yaml"

test -f "$OUT_DIR/openapi.yaml"
grep -q "openapi: 3.1.0" "$OUT_DIR/openapi.yaml"
grep -q '"/v1/tenants/{tenantId}/orders/{orderId}/start"' "$OUT_DIR/openapi.yaml"
grep -q "operationId: StartOrderProcessing" "$OUT_DIR/openapi.yaml"
grep -q "parameters:" "$OUT_DIR/openapi.yaml"
grep -q "name: tenantId" "$OUT_DIR/openapi.yaml"
grep -q "requestBody:" "$OUT_DIR/openapi.yaml"
grep -q '"202"' "$OUT_DIR/openapi.yaml"
grep -q "application/problem+json" "$OUT_DIR/openapi.yaml"
grep -q "ProblemDetails:" "$OUT_DIR/openapi.yaml"
grep -q "StartOrderProcessingRequest:" "$OUT_DIR/openapi.yaml"
grep -q "StartOrderProcessingResponse:" "$OUT_DIR/openapi.yaml"
grep -q "OrderEventsOrderValidatedPayload:" "$OUT_DIR/openapi.yaml"
grep -q "format: date-time" "$OUT_DIR/openapi.yaml"

rm -rf "$OUT_DIR"

GENERATE_ALL_OUTPUT="$($CLI generate "$EXAMPLE" --out "$OUT_DIR")"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/mt-manifest.yaml"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/mt_entities.hpp"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/mt_metadata.cpp"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/mt-state-machines.yaml"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/dl-manifest.yaml"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/dl_leases.hpp"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/dl_metadata.cpp"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/qu-manifest.yaml"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/qu_messages.hpp"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/qu_metadata.cpp"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/wf-manifest.yaml"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/wf_workflows.hpp"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/wf_metadata.cpp"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/openapi.yaml"

test -f "$OUT_DIR/mt-manifest.yaml"
test -f "$OUT_DIR/mt_entities.hpp"
test -f "$OUT_DIR/mt_metadata.cpp"
test -f "$OUT_DIR/mt-state-machines.yaml"
test -f "$OUT_DIR/dl-manifest.yaml"
test -f "$OUT_DIR/dl_leases.hpp"
test -f "$OUT_DIR/dl_metadata.cpp"
test -f "$OUT_DIR/qu-manifest.yaml"
test -f "$OUT_DIR/qu_messages.hpp"
test -f "$OUT_DIR/qu_metadata.cpp"
test -f "$OUT_DIR/wf-manifest.yaml"
test -f "$OUT_DIR/wf_workflows.hpp"
test -f "$OUT_DIR/wf_metadata.cpp"
test -f "$OUT_DIR/openapi.yaml"

grep -q "Order" "$OUT_DIR/mt-manifest.yaml"
grep -q "OrderWorkflowLease" "$OUT_DIR/dl-manifest.yaml"
grep -q "OrderEvents" "$OUT_DIR/qu-manifest.yaml"
grep -q "OrderProcessing" "$OUT_DIR/wf-manifest.yaml"
grep -q "openapi: 3.1.0" "$OUT_DIR/openapi.yaml"

rm -rf "$OUT_DIR"

echo "generate CLI tests passed"
