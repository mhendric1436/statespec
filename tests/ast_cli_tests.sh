#!/usr/bin/env sh
set -eu

CLI="${1:-build/bin/statespec}"
EXAMPLE="examples/order-system.sspec"
FEATURE_FLAGS_EXAMPLE="examples/feature-flags.sspec"
TMPDIR="$(mktemp -d)"
cleanup() {
  rm -rf "$TMPDIR"
}
trap cleanup EXIT

if [ ! -x "$CLI" ]; then
  echo "test failed: CLI not found or not executable: $CLI" >&2
  exit 1
fi

if [ ! -f "$EXAMPLE" ]; then
  echo "test failed: missing example file: $EXAMPLE" >&2
  exit 1
fi

if [ ! -f "$FEATURE_FLAGS_EXAMPLE" ]; then
  echo "test failed: missing example file: $FEATURE_FLAGS_EXAMPLE" >&2
  exit 1
fi

AST_OUTPUT="$($CLI ast "$EXAMPLE")"

printf '%s\n' "$AST_OUTPUT" | grep -q '"version": "0.1"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"system"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"name": "OrderSystem"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"entities"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"feature_flags"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"queues"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"leases"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"workers"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"apis"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"workflows"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"policies"'

printf '%s\n' "$AST_OUTPUT" | python3 -c '
import json
import sys

document = json.load(sys.stdin)
assert document["version"] == "0.1"
assert document["system"]["name"] == "OrderSystem"
assert "feature_flags" in document["system"]
assert len(document["system"]["entities"]) >= 1
assert len(document["system"]["queues"]) >= 1
assert len(document["system"]["workflows"]) >= 1
'

FEATURE_FLAGS_AST_OUTPUT="$($CLI ast "$FEATURE_FLAGS_EXAMPLE")"

printf '%s\n' "$FEATURE_FLAGS_AST_OUTPUT" | grep -q '"feature_flags"'
printf '%s\n' "$FEATURE_FLAGS_AST_OUTPUT" | grep -q '"name": "NewScheduler"'
printf '%s\n' "$FEATURE_FLAGS_AST_OUTPUT" | grep -q '"type": "bool"'
printf '%s\n' "$FEATURE_FLAGS_AST_OUTPUT" | grep -q '"default": "false"'
printf '%s\n' "$FEATURE_FLAGS_AST_OUTPUT" | grep -q '"scope": "tenant"'

printf '%s\n' "$FEATURE_FLAGS_AST_OUTPUT" | python3 -c '
import json
import sys

document = json.load(sys.stdin)
flags = document["system"]["feature_flags"]
assert len(flags) == 2
assert flags[0]["name"] == "NewScheduler"
assert flags[0]["type"] == "bool"
assert flags[0]["default"] == "false"
assert flags[0]["scope"] == "tenant"
assert flags[0]["owner"] == "platform"
assert flags[1]["name"] == "MaxPendingOrders"
assert flags[1]["type"] == "int"
assert flags[1]["default"] == "100"
'

INCLUDE_EXAMPLE="$TMPDIR/include-root.sspec"
cat > "$INCLUDE_EXAMPLE" <<'SSPEC'
statespec 0.1;
include "./workflow-launch-control.sspec";

system IncludeDemo {}
SSPEC

INCLUDE_AST_OUTPUT="$($CLI ast "$INCLUDE_EXAMPLE")"

printf '%s\n' "$INCLUDE_AST_OUTPUT" | grep -q '"includes"'
printf '%s\n' "$INCLUDE_AST_OUTPUT" | grep -q '"path": "./workflow-launch-control.sspec"'

printf '%s\n' "$INCLUDE_AST_OUTPUT" | python3 -c '
import json
import sys

document = json.load(sys.stdin)
assert document["includes"] == [{"path": "./workflow-launch-control.sspec"}]
assert document["system"]["name"] == "IncludeDemo"
'

echo "ast CLI tests passed"
