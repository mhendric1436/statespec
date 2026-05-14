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

echo "formatter CLI tests passed"
