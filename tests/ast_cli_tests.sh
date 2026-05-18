#!/usr/bin/env sh
set -eu

CLI="${1:-build/bin/statespec}"
EXAMPLE="examples/order-system.sspec"
FEATURE_FLAGS_EXAMPLE="examples/feature-flags.sspec"
PARITY_FIXTURE="testdata/parity/kitchen-sink.sspec"
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

if [ ! -f "$PARITY_FIXTURE" ]; then
  echo "test failed: missing parity fixture: $PARITY_FIXTURE" >&2
  exit 1
fi

AST_OUTPUT="$($CLI ast "$EXAMPLE")"

printf '%s\n' "$AST_OUTPUT" | grep -q '"version": "0.1"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"system"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"name": "OrderSystem"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"entities"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"shapes"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"feature_flags"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"namespaces"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"values"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"enums"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"events"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"external_systems"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"logs"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"metrics"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"queues"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"leases"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"workers"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"api_servers"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"apis"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"workflows"'
printf '%s\n' "$AST_OUTPUT" | grep -q '"policies"'

printf '%s\n' "$AST_OUTPUT" | python3 -c '
import json
import sys

document = json.load(sys.stdin)
assert document["version"] == "0.1"
assert document["system"]["name"] == "OrderSystem"
assert "shapes" in document["system"]
assert "feature_flags" in document["system"]
assert "namespaces" in document["system"]
assert "values" in document["system"]
assert "enums" in document["system"]
assert "events" in document["system"]
assert "external_systems" in document["system"]
assert len(document["system"]["entities"]) >= 1
assert len(document["system"]["queues"]) >= 1
assert len(document["system"]["workflows"]) >= 1
'

OBSERVABILITY_EXAMPLE="$TMPDIR/observability.sspec"
cat > "$OBSERVABILITY_EXAMPLE" <<'SSPEC'
statespec 0.1;

system ObservabilityDemo {
  log WorkflowLaunchDecision {
    level info
    event_name "workflow.launch.decision"
    fields {
      tenant_id string
      decision string
    }
  }

  metric WorkflowLaunchAttempts {
    kind counter
    name "workflow_launch_attempts_total"
    unit count
    labels {
      tenant_id string
      decision string
    }
  }
}
SSPEC

OBSERVABILITY_AST_OUTPUT="$($CLI ast "$OBSERVABILITY_EXAMPLE")"

printf '%s\n' "$OBSERVABILITY_AST_OUTPUT" | grep -q '"logs"'
printf '%s\n' "$OBSERVABILITY_AST_OUTPUT" | grep -q '"metrics"'
printf '%s\n' "$OBSERVABILITY_AST_OUTPUT" | grep -q '"event_name": "workflow.launch.decision"'
printf '%s\n' "$OBSERVABILITY_AST_OUTPUT" | grep -q '"backend_name": "workflow_launch_attempts_total"'

printf '%s\n' "$OBSERVABILITY_AST_OUTPUT" | python3 -c '
import json
import sys

document = json.load(sys.stdin)
logs = document["system"]["logs"]
metrics = document["system"]["metrics"]
assert len(logs) == 1
assert logs[0]["name"] == "WorkflowLaunchDecision"
assert logs[0]["level"] == "info"
assert logs[0]["event_name"] == "workflow.launch.decision"
assert logs[0]["fields"][0]["name"] == "tenant_id"
assert len(metrics) == 1
assert metrics[0]["name"] == "WorkflowLaunchAttempts"
assert metrics[0]["kind"] == "counter"
assert metrics[0]["backend_name"] == "workflow_launch_attempts_total"
assert metrics[0]["unit"] == "count"
assert metrics[0]["labels"][1]["name"] == "decision"
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

PARITY_AST_OUTPUT="$($CLI ast "$PARITY_FIXTURE")"

printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"name": "KitchenSink"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"feature_flags"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"namespaces"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"values"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"enums"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"events"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"shapes"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"external_systems"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"logs"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"metrics"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"entities"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"queues"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"leases"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"workers"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"api_servers"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"apis"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"workflows"'
printf '%s\n' "$PARITY_AST_OUTPUT" | grep -q '"policies"'

printf '%s\n' "$PARITY_AST_OUTPUT" | python3 -c '
import json
import sys

document = json.load(sys.stdin)
system = document["system"]
assert system["name"] == "KitchenSink"
for key in [
    "feature_flags",
    "namespaces",
    "values",
    "enums",
    "events",
    "shapes",
    "external_systems",
    "logs",
    "metrics",
    "entities",
    "queues",
    "leases",
    "workers",
    "api_servers",
    "apis",
    "workflows",
    "policies",
]:
    assert len(system[key]) >= 1, key
assert system["shapes"][0]["name"] == "StartOrderProcessingRequest"
assert system["values"][0]["name"] == "OrderAmount"
assert system["enums"][0]["name"] == "OrderStatus"
assert system["events"][0]["name"] == "OrderAccepted"
assert system["namespaces"][0]["name"] == "Billing"
assert system["external_systems"][0]["name"] == "Billing.Stripe"
assert system["shapes"][0]["fields"][0]["name"] == "tenant_id"
assert system["entities"][0]["key_fields"] == ["tenant_id", "order_id"]
assert system["queues"][0]["name"] == "EmailDispatch"
assert system["workers"][0]["name"] == "OrderWorker"
assert system["api_servers"][0]["name"] == "OrderApi"
assert system["apis"][0]["name"] == "StartOrderProcessing"
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
