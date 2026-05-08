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

test -f "$OUT_DIR/wf-manifest.yaml"
grep -q "target: wf" "$OUT_DIR/wf-manifest.yaml"
grep -q "OrderProcessing" "$OUT_DIR/wf-manifest.yaml"

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

GENERATE_ALL_OUTPUT="$($CLI generate "$EXAMPLE" --out "$OUT_DIR")"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/mt-manifest.yaml"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/mt_entities.hpp"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/mt_metadata.cpp"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/mt-state-machines.yaml"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/dl-manifest.yaml"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/qu-manifest.yaml"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/wf-manifest.yaml"
printf '%s\n' "$GENERATE_ALL_OUTPUT" | grep -q "generated $OUT_DIR/openapi.yaml"

test -f "$OUT_DIR/mt-manifest.yaml"
test -f "$OUT_DIR/mt_entities.hpp"
test -f "$OUT_DIR/mt_metadata.cpp"
test -f "$OUT_DIR/mt-state-machines.yaml"
test -f "$OUT_DIR/dl-manifest.yaml"
test -f "$OUT_DIR/qu-manifest.yaml"
test -f "$OUT_DIR/wf-manifest.yaml"
test -f "$OUT_DIR/openapi.yaml"

grep -q "Order" "$OUT_DIR/mt-manifest.yaml"
grep -q "OrderWorkflowLease" "$OUT_DIR/dl-manifest.yaml"
grep -q "OrderEvents" "$OUT_DIR/qu-manifest.yaml"
grep -q "openapi: 3.1.0" "$OUT_DIR/openapi.yaml"

rm -rf "$OUT_DIR"

echo "generate CLI tests passed"
