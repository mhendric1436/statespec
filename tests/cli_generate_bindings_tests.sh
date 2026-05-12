#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
trap 'rm -rf "$TMPDIR"' EXIT

SPEC="$TMPDIR/minimal.sspec"
cat > "$SPEC" <<'SSPEC'
system Demo {
}
SSPEC

run_expect_status() {
    expected="$1"
    shift
    output="$TMPDIR/output.txt"
    set +e
    "$@" > "$output" 2>&1
    status="$?"
    set -e
    if [ "$status" -ne "$expected" ]; then
        echo "expected status $expected, got $status for: $*" >&2
        cat "$output" >&2
        exit 1
    fi
}

assert_output_contains() {
    needle="$1"
    if ! grep -F "$needle" "$TMPDIR/output.txt" >/dev/null 2>&1; then
        echo "expected output to contain: $needle" >&2
        cat "$TMPDIR/output.txt" >&2
        exit 1
    fi
}

# New shape is accepted and routed to binding generator dispatch. Until the
# per-language generators are added, Milestone 4 reports a language-specific
# not-implemented diagnostic.
run_expect_status 1 "$CLI" generate bindings --lang cpp "$SPEC" --out "$TMPDIR/out-cpp"
assert_output_contains "SSPEC5103"
assert_output_contains "binding generator for language 'cpp' is not implemented yet"

run_expect_status 1 "$CLI" generate bindings --lang go "$SPEC"
assert_output_contains "SSPEC5103"
assert_output_contains "binding generator for language 'go' is not implemented yet"

run_expect_status 1 "$CLI" generate bindings --lang java "$SPEC" --out "$TMPDIR/out-java"
assert_output_contains "binding generator for language 'java' is not implemented yet"

run_expect_status 1 "$CLI" generate bindings --lang rust "$SPEC" --out "$TMPDIR/out-rust"
assert_output_contains "binding generator for language 'rust' is not implemented yet"

# --lang is required.
run_expect_status 2 "$CLI" generate bindings "$SPEC"
assert_output_contains "generate bindings requires --lang <cpp|go|java|rust>"

# --lang must be supported.
run_expect_status 2 "$CLI" generate bindings --lang python "$SPEC"
assert_output_contains "unsupported binding language 'python'"
assert_output_contains "cpp|go|java|rust"

# Input file is required.
run_expect_status 2 "$CLI" generate bindings --lang cpp
assert_output_contains "generate bindings requires an input .sspec file"

# Pre-alpha cleanup: old generate shape is no longer accepted.
run_expect_status 2 "$CLI" generate "$SPEC" mt
assert_output_contains "unsupported generate kind: $SPEC"

run_expect_status 2 "$CLI" generate mt "$SPEC"
assert_output_contains "unsupported generate kind: mt"
