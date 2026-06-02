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

SPEC="$TESTS_DIR/fixtures/bindings-full.sspec"
E2E_SPEC="testdata/generators/external-system-metadata-e2e.sspec"
ENTITY_CRUD_SPEC="testdata/generators/entity-crud-openapi.sspec"
SCHEMA_LOWERING_SPEC="testdata/generators/openapi-schema-lowering.sspec"

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

run_expect_status 0 "$CLI" validate "$ENTITY_CRUD_SPEC"

run_expect_status 0 "$CLI" generate openapi "$ENTITY_CRUD_SPEC" --out "$TMPDIR/out-entity-crud-openapi"
assert_output_contains "generated $TMPDIR/out-entity-crud-openapi/openapi.json"
assert_file_contains "$TMPDIR/out-entity-crud-openapi/openapi.json" "\"title\": \"EntityCrudOpenApi API\""
assert_file_contains "$TMPDIR/out-entity-crud-openapi/openapi.json" "\"/v1/tenants/{tenant_id}/accounts/{account_id}\""
assert_file_contains "$TMPDIR/out-entity-crud-openapi/openapi.json" "\"/v1/tenants/{tenant_id}/accounts/{account_id}/status\""
assert_file_contains "$TMPDIR/out-entity-crud-openapi/openapi.json" "\"/v1/tenants/{tenant_id}/account-status/{status}/accounts\""
assert_file_contains "$TMPDIR/out-entity-crud-openapi/openapi.json" "\"operationId\": \"CreateAccount\""
assert_file_contains "$TMPDIR/out-entity-crud-openapi/openapi.json" "\"operationId\": \"ListAccountsByStatus\""
assert_file_contains "$TMPDIR/out-entity-crud-openapi/openapi.json" "\"CreateAccountRequest\""
assert_file_contains "$TMPDIR/out-entity-crud-openapi/openapi.json" "\"UpdateAccountStatusRequest\""
assert_file_contains "$TMPDIR/out-entity-crud-openapi/openapi.json" "\"ListAccountsByStatusResponse\""

python3 - "$TMPDIR/out-entity-crud-openapi/openapi.json" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as handle:
    document = json.load(handle)

schemas = document["components"]["schemas"]
create_fields = set(schemas["CreateAccountRequest"]["properties"].keys())
if create_fields != {"display_name"}:
    raise SystemExit(f"unexpected create request fields: {sorted(create_fields)}")

status_fields = set(schemas["UpdateAccountStatusRequest"]["properties"].keys())
if status_fields != {"status"}:
    raise SystemExit(f"unexpected status request fields: {sorted(status_fields)}")

account_response_fields = set(schemas["AccountResponse"]["properties"].keys())
for field in ["tenant_id", "account_id", "display_name", "status", "created_at", "updated_at"]:
    if field not in account_response_fields:
        raise SystemExit(f"missing AccountResponse field: {field}")

list_response = schemas["ListAccountsByStatusResponse"]["properties"]
if set(list_response.keys()) != {"tenant_id", "status", "accounts"}:
    raise SystemExit(f"unexpected list response fields: {sorted(list_response.keys())}")
if list_response["accounts"]["items"]["$ref"] != "#/components/schemas/AccountResponse":
    raise SystemExit("list response should reference AccountResponse items")

create_operation = document["paths"]["/v1/tenants/{tenant_id}/accounts/{account_id}"]["post"]
create_params = {parameter["name"] for parameter in create_operation["parameters"]}
if create_params != {"tenant_id", "account_id"}:
    raise SystemExit(f"unexpected create path params: {sorted(create_params)}")
create_ref = create_operation["requestBody"]["content"]["application/json"]["schema"]["$ref"]
if create_ref != "#/components/schemas/CreateAccountRequest":
    raise SystemExit(f"unexpected create request ref: {create_ref}")

status_operation = document["paths"]["/v1/tenants/{tenant_id}/accounts/{account_id}/status"]["patch"]
status_ref = status_operation["requestBody"]["content"]["application/json"]["schema"]["$ref"]
if status_ref != "#/components/schemas/UpdateAccountStatusRequest":
    raise SystemExit(f"unexpected status request ref: {status_ref}")

list_operation = document["paths"]["/v1/tenants/{tenant_id}/account-status/{status}/accounts"]["get"]
list_params = {parameter["name"] for parameter in list_operation["parameters"]}
if list_params != {"tenant_id", "status"}:
    raise SystemExit(f"unexpected list path params: {sorted(list_params)}")
list_ref = list_operation["responses"]["200"]["content"]["application/json"]["schema"]["$ref"]
if list_ref != "#/components/schemas/ListAccountsByStatusResponse":
    raise SystemExit(f"unexpected list response ref: {list_ref}")
PY

run_expect_status 0 "$CLI" validate "$SCHEMA_LOWERING_SPEC"

run_expect_status 0 "$CLI" generate openapi "$SCHEMA_LOWERING_SPEC" --out "$TMPDIR/out-schema-lowering-openapi"
assert_output_contains "generated $TMPDIR/out-schema-lowering-openapi/openapi.json"

python3 - "$TMPDIR/out-schema-lowering-openapi/openapi.json" <<'PY'
import json
import sys

with open(sys.argv[1], "r", encoding="utf-8") as handle:
    document = json.load(handle)

schemas = document["components"]["schemas"]

money = schemas["Money"]
if money["type"] != "number":
    raise SystemExit(f"Money should lower to number: {money}")
if money.get("x-statespec-constraint") != "Money >= 0":
    raise SystemExit(f"Money constraint missing: {money}")

status = schemas["PaymentStatus"]
if status["type"] != "string" or status["enum"] != ["pending", "completed", "failed"]:
    raise SystemExit(f"PaymentStatus enum lowered incorrectly: {status}")

line = schemas["PaymentLine"]["properties"]
if line["amount"]["$ref"] != "#/components/schemas/Money":
    raise SystemExit(f"PaymentLine.amount should reference Money: {line['amount']}")
if line["status"]["$ref"] != "#/components/schemas/PaymentStatus":
    raise SystemExit(f"PaymentLine.status should reference PaymentStatus: {line['status']}")

request = schemas["SubmitPaymentRequest"]["properties"]
line_items = request["line_items"]
if line_items["type"] != "array":
    raise SystemExit(f"line_items should be an array: {line_items}")
if line_items["items"]["$ref"] != "#/components/schemas/PaymentLine":
    raise SystemExit(f"line_items should reference PaymentLine: {line_items}")

status_history = request["status_history"]
status_items = status_history["additionalProperties"]["items"]
if status_history["type"] != "object":
    raise SystemExit(f"status_history should be an object: {status_history}")
if status_history["additionalProperties"]["type"] != "array":
    raise SystemExit(f"status_history values should be arrays: {status_history}")
if status_items["$ref"] != "#/components/schemas/PaymentStatus":
    raise SystemExit(f"status_history items should reference PaymentStatus: {status_history}")

audit_matrix = request["audit_matrix"]
if audit_matrix["items"]["type"] != "array":
    raise SystemExit(f"audit_matrix should lower nested arrays: {audit_matrix}")
if audit_matrix["items"]["items"]["type"] != "string":
    raise SystemExit(f"audit_matrix inner item should be string: {audit_matrix}")

response = schemas["SubmitPaymentResponse"]["properties"]
if response["totals"]["additionalProperties"]["$ref"] != "#/components/schemas/Money":
    raise SystemExit(f"totals map should reference Money values: {response['totals']}")

error = schemas["ProblemDetails"]["properties"]["field_errors"]
if error["additionalProperties"]["items"]["type"] != "string":
    raise SystemExit(f"field_errors should lower nested string arrays: {error}")

operation = document["paths"]["/v1/tenants/{tenant_id}/payments/{payment_id}"]["post"]
default_response = operation["responses"]["default"]
error_ref = default_response["content"]["application/json"]["schema"]["$ref"]
if error_ref != "#/components/schemas/ProblemDetails":
    raise SystemExit(f"default response should reference ProblemDetails: {error_ref}")
PY

run_expect_status 0 "$CLI" generate openapi "$SPEC" --out "$TMPDIR/default-openapi"
assert_output_contains "generated $TMPDIR/default-openapi/openapi.json"
assert_file_exists "$TMPDIR/default-openapi/openapi.json"
