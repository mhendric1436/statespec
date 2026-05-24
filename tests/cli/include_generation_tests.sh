#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TESTS_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
. "$SCRIPT_DIR/common.sh"

INCLUDED_SPEC="$TMPDIR/included-bindings.sspec"
INCLUDE_ROOT_SPEC="$TMPDIR/include-bindings-root.sspec"
cp "$TESTS_DIR/fixtures/included-bindings.sspec" "$INCLUDED_SPEC"
cp "$TESTS_DIR/fixtures/include-bindings-root.sspec" "$INCLUDE_ROOT_SPEC"

run_expect_status 0 "$CLI" generate bindings --lang cpp "$INCLUDE_ROOT_SPEC" --out "$TMPDIR/out-include-cpp"
assert_output_contains "generated $TMPDIR/out-include-cpp/common/descriptors.hpp"
assert_file_exists "$TMPDIR/out-include-cpp/common/descriptors.hpp"
assert_file_contains "$TMPDIR/out-include-cpp/common/descriptors.hpp" "IncludedEntity"
assert_file_contains "$TMPDIR/out-include-cpp/common/descriptors.hpp" "IncludedQueue"
assert_file_exists "$TMPDIR/out-include-cpp/common/workflows/included_workflow.hpp"
assert_file_contains "$TMPDIR/out-include-cpp/common/workflows/included_workflow.hpp" "IncludedWorkflow"
assert_file_contains "$TMPDIR/out-include-cpp/common/descriptors.hpp" "IncludedEntityLaunch"
