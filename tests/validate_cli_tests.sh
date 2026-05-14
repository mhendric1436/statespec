#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

INVALID_GC_SPEC="$TMPDIR/invalid-gc.sspec"
VALID_GC_SPEC="$TMPDIR/valid-gc.sspec"

cat > "$INVALID_GC_SPEC" <<'SSPEC'
system Demo {
  entity Order {
    key order_id
    fields {
      order_id string
      created_at timestamp
      updated_at timestamp
      status string
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
  entity Order {
    key order_id
    fields {
      order_id string
      created_at timestamp
      updated_at timestamp
      status string
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

echo "validate CLI tests passed"
