#!/bin/sh
set -eu

CLI="$1"
EXAMPLE="examples/order-system.sspec"
EXTERNAL_METADATA_EXAMPLE="examples/external-system-metadata.sspec"
LAUNCH_CONTROL_EXAMPLE="examples/workflow-launch-control.sspec"
LAUNCH_CONTROL_INCLUDE_EXAMPLE="examples/order-system-with-launch-control.sspec"
PARITY_FIXTURE="testdata/parity/kitchen-sink.sspec"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

INVALID_GC_SPEC="$TMPDIR/invalid-gc.sspec"
VALID_GC_SPEC="$TMPDIR/valid-gc.sspec"
INCLUDE_SHARED_SPEC="$TMPDIR/shared.sspec"
INCLUDE_ROOT_SPEC="$TMPDIR/include-root.sspec"
MISSING_INCLUDE_SPEC="$TMPDIR/missing-include.sspec"
CYCLE_A_SPEC="$TMPDIR/cycle-a.sspec"
CYCLE_B_SPEC="$TMPDIR/cycle-b.sspec"
DUPLICATE_INCLUDE_SPEC="$TMPDIR/duplicate-include.sspec"
DUPLICATE_ROOT_SPEC="$TMPDIR/duplicate-root.sspec"

cat > "$INVALID_GC_SPEC" <<'SSPEC'
system Demo {
  tenant scoped_by tenant_id
  system_tenant configured

  entity Order {
    key tenant_id, order_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      order_id string
    }
    state_machine {
      state Creating {
        garbage_collection {
          after: soon
          mode: compact
        }
      }
      state Deleted {
        terminal: true
        garbage_collection {
          after: P30D
        }
      }
      initial Creating
      terminal Deleted
      Creating -> Deleted
      Deleted -> Creating
    }
  }
}
SSPEC

cat > "$VALID_GC_SPEC" <<'SSPEC'
system Demo {
  tenant scoped_by tenant_id
  system_tenant configured

  entity Order {
    key tenant_id, order_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      order_id string
    }
    state_machine {
      state Creating
      state Deleted {
        terminal: true
        garbage_collection {
          after: P30D
          mode: tombstone
        }
      }
      initial Creating
      terminal Deleted
      Creating -> Deleted
    }
  }
}
SSPEC

cat > "$INCLUDE_SHARED_SPEC" <<'SSPEC'
statespec 0.1;

system SharedModel {
  tenant scoped_by tenant_id
  system_tenant configured

  entity SharedEntity {
    key tenant_id, shared_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      shared_id string
    }
    state_machine {
      state Active
      initial Active
    }
  }
}
SSPEC

cat > "$INCLUDE_ROOT_SPEC" <<SSPEC
statespec 0.1;
include "./shared.sspec";

system IncludeRoot {
  feature_flag SharedEntityLaunch {
    type bool
    default false
    scope entity SharedEntity
  }
}
SSPEC

cat > "$MISSING_INCLUDE_SPEC" <<'SSPEC'
statespec 0.1;
include "./does-not-exist.sspec";

system MissingInclude {}
SSPEC

cat > "$CYCLE_A_SPEC" <<'SSPEC'
statespec 0.1;
include "./cycle-b.sspec";

system CycleA {}
SSPEC

cat > "$CYCLE_B_SPEC" <<'SSPEC'
statespec 0.1;
include "./cycle-a.sspec";

system CycleB {}
SSPEC

cat > "$DUPLICATE_INCLUDE_SPEC" <<'SSPEC'
statespec 0.1;

system DuplicateInclude {
  tenant scoped_by tenant_id
  system_tenant configured

  entity SharedEntity {
    key tenant_id, shared_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      shared_id string
    }
    state_machine {
      state Active
      initial Active
    }
  }
}
SSPEC

cat > "$DUPLICATE_ROOT_SPEC" <<'SSPEC'
statespec 0.1;
include "./duplicate-include.sspec";

system DuplicateRoot {
  tenant scoped_by tenant_id
  system_tenant configured

  entity SharedEntity {
    key tenant_id, shared_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }
    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      shared_id string
    }
    state_machine {
      state Active
      initial Active
    }
  }
}
SSPEC

set +e
"$CLI" validate "$INVALID_GC_SPEC" > "$TMPDIR/invalid-output.txt" 2>&1
STATUS="$?"
set -e

if [ "$STATUS" -eq 0 ]; then
    echo "expected invalid GC spec to fail validation" >&2
    cat "$TMPDIR/invalid-output.txt" >&2
    exit 1
fi

grep -F "invalid" "$TMPDIR/invalid-output.txt" >/dev/null
grep -F "SSPEC3103" "$TMPDIR/invalid-output.txt" >/dev/null
grep -F "garbage_collection for state 'Creating' is valid only on terminal states" "$TMPDIR/invalid-output.txt" >/dev/null
grep -F "SSPEC4004" "$TMPDIR/invalid-output.txt" >/dev/null
grep -F "garbage_collection for state 'Creating' after must be an ISO-8601 duration" "$TMPDIR/invalid-output.txt" >/dev/null
grep -F "SSPEC3104" "$TMPDIR/invalid-output.txt" >/dev/null
grep -F "mode must be delete, tombstone, or archive" "$TMPDIR/invalid-output.txt" >/dev/null
grep -F "SSPEC4001" "$TMPDIR/invalid-output.txt" >/dev/null
grep -F "garbage_collection for state 'Deleted' must declare mode" "$TMPDIR/invalid-output.txt" >/dev/null
grep -F "SSPEC3105" "$TMPDIR/invalid-output.txt" >/dev/null
grep -F "garbage-collected terminal state 'Deleted' must not have outgoing transitions" "$TMPDIR/invalid-output.txt" >/dev/null

"$CLI" validate "$VALID_GC_SPEC" > "$TMPDIR/valid-output.txt" 2>&1
grep -F "valid" "$TMPDIR/valid-output.txt" >/dev/null

"$CLI" validate "$EXAMPLE" > "$TMPDIR/example-output.txt" 2>&1
grep -F "valid" "$TMPDIR/example-output.txt" >/dev/null

"$CLI" validate "$EXTERNAL_METADATA_EXAMPLE" > "$TMPDIR/external-metadata-output.txt" 2>&1
grep -F "valid" "$TMPDIR/external-metadata-output.txt" >/dev/null

"$CLI" validate "$LAUNCH_CONTROL_EXAMPLE" > "$TMPDIR/launch-control-output.txt" 2>&1
grep -F "valid" "$TMPDIR/launch-control-output.txt" >/dev/null

"$CLI" validate "$LAUNCH_CONTROL_INCLUDE_EXAMPLE" > "$TMPDIR/launch-control-include-output.txt" 2>&1
grep -F "valid" "$TMPDIR/launch-control-include-output.txt" >/dev/null

"$CLI" validate "$PARITY_FIXTURE" > "$TMPDIR/parity-output.txt" 2>&1
grep -F "valid" "$TMPDIR/parity-output.txt" >/dev/null

"$CLI" validate "$INCLUDE_ROOT_SPEC" > "$TMPDIR/include-root-output.txt" 2>&1
grep -F "valid" "$TMPDIR/include-root-output.txt" >/dev/null

set +e
"$CLI" validate "$MISSING_INCLUDE_SPEC" > "$TMPDIR/missing-include-output.txt" 2>&1
STATUS="$?"
set -e
if [ "$STATUS" -eq 0 ]; then
    echo "expected missing include spec to fail validation" >&2
    cat "$TMPDIR/missing-include-output.txt" >&2
    exit 1
fi
grep -F "invalid" "$TMPDIR/missing-include-output.txt" >/dev/null
grep -F "SSPEC5001" "$TMPDIR/missing-include-output.txt" >/dev/null
grep -F "2:1" "$TMPDIR/missing-include-output.txt" >/dev/null
grep -F "included file does not exist" "$TMPDIR/missing-include-output.txt" >/dev/null

set +e
"$CLI" validate "$CYCLE_A_SPEC" > "$TMPDIR/cycle-output.txt" 2>&1
STATUS="$?"
set -e
if [ "$STATUS" -eq 0 ]; then
    echo "expected cyclic include spec to fail validation" >&2
    cat "$TMPDIR/cycle-output.txt" >&2
    exit 1
fi
grep -F "invalid" "$TMPDIR/cycle-output.txt" >/dev/null
grep -F "SSPEC5002" "$TMPDIR/cycle-output.txt" >/dev/null
grep -F "2:1" "$TMPDIR/cycle-output.txt" >/dev/null
grep -F "include cycle detected" "$TMPDIR/cycle-output.txt" >/dev/null

set +e
"$CLI" validate "$DUPLICATE_ROOT_SPEC" > "$TMPDIR/duplicate-output.txt" 2>&1
STATUS="$?"
set -e
if [ "$STATUS" -eq 0 ]; then
    echo "expected duplicate composed spec to fail validation" >&2
    cat "$TMPDIR/duplicate-output.txt" >&2
    exit 1
fi
grep -F "invalid" "$TMPDIR/duplicate-output.txt" >/dev/null
grep -F "SSPEC3001" "$TMPDIR/duplicate-output.txt" >/dev/null
grep -F "duplicate declaration 'SharedEntity'" "$TMPDIR/duplicate-output.txt" >/dev/null

echo "validate CLI tests passed"
