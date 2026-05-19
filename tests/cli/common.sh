#!/bin/sh

require_test_tmpdir() {
    if [ -z "${TMPDIR:-}" ]; then
        echo "test helper requires TMPDIR to be set" >&2
        exit 1
    fi
}

run_expect_status() {
    require_test_tmpdir
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
    require_test_tmpdir
    needle="$1"
    if ! grep -F -- "$needle" "$TMPDIR/output.txt" >/dev/null 2>&1; then
        echo "expected output to contain: $needle" >&2
        cat "$TMPDIR/output.txt" >&2
        exit 1
    fi
}

assert_file_exists() {
    require_test_tmpdir
    path="$1"
    if [ ! -f "$path" ]; then
        echo "expected generated file: $path" >&2
        cat "$TMPDIR/output.txt" >&2 || true
        exit 1
    fi
}

assert_file_not_exists() {
    require_test_tmpdir
    path="$1"
    if [ -e "$path" ]; then
        echo "expected file to be absent: $path" >&2
        cat "$TMPDIR/output.txt" >&2 || true
        exit 1
    fi
}

assert_file_contains() {
    path="$1"
    needle="$2"
    if ! grep -F -- "$needle" "$path" >/dev/null 2>&1; then
        echo "expected $path to contain: $needle" >&2
        cat "$path" >&2 || true
        exit 1
    fi
}
