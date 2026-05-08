#!/usr/bin/env sh
set -eu

CLI="${1:-build/bin/statespec}"
EXAMPLE="examples/order-system.sspec"

if [ ! -x "$CLI" ]; then
  echo "test failed: CLI not found or not executable: $CLI" >&2
  exit 1
fi

if [ ! -f "$EXAMPLE" ]; then
  echo "test failed: missing example file: $EXAMPLE" >&2
  exit 1
fi

AST_OUTPUT="$($CLI ast "$EXAMPLE")"

printf '%s\n' "$AST_OUTPUT" | grep -q '"version": "0.1"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"system"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"name": "OrderSystem"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"entities"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"queues"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"leases"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"workers"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"apis"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"workflows"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"policies"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"generators"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"target": "mt"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"target": "dl"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"target": "qu"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"target": "wf"'

printf '%s\n' "$AST_OUTPUT" | python3 -c '
import json
import sys

document = json.load(sys.stdin)
assert document["version"] == "0.1"
assert document["system"]["name"] == "OrderSystem"
assert len(document["system"]["entities"]) >= 1
assert len(document["system"]["queues"]) >= 1
assert len(document["system"]["workflows"]) >= 1
assert len(document["system"]["generators"]) >= 1
'

echo "ast CLI tests passed"
