#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

UNFORMATTED="$TMPDIR/unformatted.sspec"
FORMATTED="$TMPDIR/formatted.sspec"
EXPECTED="$TMPDIR/expected.sspec"
GC_UNFORMATTED="$TMPDIR/gc-unformatted.sspec"
GC_FORMATTED="$TMPDIR/gc-formatted.sspec"
GC_EXPECTED="$TMPDIR/gc-expected.sspec"

cat > "$UNFORMATTED" <<'SSPEC'
statespec 0.1; system Demo { feature_flag NewScheduler { type bool default false scope tenant owner "platform" description "New scheduler" expires "2026-12-31" } }
SSPEC

cat > "$EXPECTED" <<'SSPEC'
statespec 0.1;
system Demo {
  feature_flag NewScheduler {
    type bool
    default false
    scope tenant
    owner "platform"
    description "New scheduler"
    expires "2026-12-31"
  }
}
SSPEC

"$CLI" fmt "$UNFORMATTED" > "$FORMATTED"
diff -u "$EXPECTED" "$FORMATTED"

"$CLI" fmt --check "$FORMATTED"

set +e
"$CLI" fmt --check "$UNFORMATTED" > "$TMPDIR/check-output.txt" 2>&1
STATUS="$?"
set -e

if [ "$STATUS" -eq 0 ]; then
    echo "expected fmt --check to reject unformatted file" >&2
    exit 1
fi

grep -F "is not formatted" "$TMPDIR/check-output.txt" >/dev/null

cat > "$GC_UNFORMATTED" <<'SSPEC'
system Demo { entity Order { key order_id; fields { order_id string; created_at timestamp; updated_at timestamp; status string; } state_machine { state Creating; state Deleted { terminal: true; garbage_collection { after: P30D; mode: tombstone; } } initial Creating; terminal [Deleted]; Creating -> Deleted; } } }
SSPEC

cat > "$GC_EXPECTED" <<'SSPEC'
system Demo {
  entity Order {
    key order_id;
    fields {
      order_id string;
      created_at timestamp;
      updated_at timestamp;
      status string;
    }
    state_machine {
      state Creating;
      state Deleted {
        terminal: true;
        garbage_collection {
          after: P30D;
          mode: tombstone;
        }
      }
      initial Creating;
      terminal[Deleted];
      Creating -> Deleted;
    }
  }
}
SSPEC

"$CLI" fmt "$GC_UNFORMATTED" > "$GC_FORMATTED"
diff -u "$GC_EXPECTED" "$GC_FORMATTED"

echo "formatter CLI tests passed"
