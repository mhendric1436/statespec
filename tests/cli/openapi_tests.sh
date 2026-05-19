#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR" generated/openapi
    rmdir generated 2>/dev/null || true
}
trap cleanup EXIT

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TESTS_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
. "$SCRIPT_DIR/common.sh"

SPEC="$TESTS_DIR/fixtures/bindings-full.sspec"
E2E_SPEC="testdata/generators/external-system-metadata-e2e.sspec"

run_expect_status 0 "$CLI" generate openapi "$SPEC" --out "$TMPDIR/out-openapi"
assert_output_contains "generated $TMPDIR/out-openapi/openapi.json"
assert_file_exists "$TMPDIR/out-openapi/openapi.json"
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"openapi\": \"3.0.3\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"title\": \"Demo API\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"/v1/tenants/{tenantId}/orders/{order_id}/start\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"operationId\": \"StartOrderProcessing\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"StartOrderProcessingRequest\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"tenant_id\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"parameters\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"/v1/tenants/{tenant_id}/operators/external-systems/{external_system_id}/profiles/{profile}\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"operationId\": \"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"operationId\": \"GetExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"operationId\": \"DisableExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"operationId\": \"DeleteExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"ExternalSystemEndpointRequest\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"ExternalSystemEndpointResponse\""
assert_file_contains "$TMPDIR/out-openapi/openapi.json" "\"base_url\""

run_expect_status 0 "$CLI" validate "$E2E_SPEC"

run_expect_status 0 "$CLI" generate openapi "$E2E_SPEC" --out "$TMPDIR/out-e2e-openapi"
assert_output_contains "generated $TMPDIR/out-e2e-openapi/openapi.json"
assert_file_contains "$TMPDIR/out-e2e-openapi/openapi.json" "\"title\": \"ExternalMetadataE2E API\""
assert_file_contains "$TMPDIR/out-e2e-openapi/openapi.json" "\"operationId\": \"PlacePayment\""
assert_file_contains "$TMPDIR/out-e2e-openapi/openapi.json" "\"operationId\": \"UpsertExternalSystemEndpoint\""
assert_file_contains "$TMPDIR/out-e2e-openapi/openapi.json" "\"patch\""
assert_file_contains "$TMPDIR/out-e2e-openapi/openapi.json" "\"retry_policy\""

run_expect_status 0 "$CLI" generate openapi "$SPEC"
assert_output_contains "generated generated/openapi/openapi.json"
assert_file_exists "generated/openapi/openapi.json"
