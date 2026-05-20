#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
REPO_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../../.." && pwd)"
TESTS_DIR="$REPO_DIR/tests"
. "$TESTS_DIR/cli/common.sh"

E2E_SPEC="$REPO_DIR/testdata/generators/external-system-metadata-e2e.sspec"

# End-to-end external-system metadata generated binding regression.
run_expect_status 0 "$CLI" generate bindings --lang cpp "$E2E_SPEC" --out "$TMPDIR/out-e2e-cpp"
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "api_route_descriptors"
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-cpp/common/descriptors.hpp" "IExternalSystemOperatorMetadataApiHandler"

run_expect_status 0 "$CLI" generate bindings --lang go "$E2E_SPEC" --out "$TMPDIR/out-e2e-go"
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "func ApiRouteDescriptors"
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-go/common/backend/descriptors.go" "ExternalSystemOperatorMetadataAPIHandler"

run_expect_status 0 "$CLI" generate bindings --lang java "$E2E_SPEC" --out "$TMPDIR/out-e2e-java"
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "apiRouteDescriptors"
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-java/common/com/statespec/generated/Descriptors.java" "ExternalSystemOperatorMetadataApiHandler"

run_expect_status 0 "$CLI" generate bindings --lang rust "$E2E_SPEC" --out "$TMPDIR/out-e2e-rust"
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "pub fn api_route_descriptors"
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "\"OperatorApi\""
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "\"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "\"metadata.retry_policy\""
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "\"input.payment_id\""
assert_file_contains "$TMPDIR/out-e2e-rust/common/descriptors.rs" "ExternalSystemOperatorMetadataApiHandler"
