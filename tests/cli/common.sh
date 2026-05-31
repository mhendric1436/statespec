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

assert_file_not_contains() {
    path="$1"
    needle="$2"
    if grep -F -- "$needle" "$path" >/dev/null 2>&1; then
        echo "expected $path not to contain: $needle" >&2
        cat "$path" >&2 || true
        exit 1
    fi
}

assert_file_occurrence_count() {
    path="$1"
    needle="$2"
    expected="$3"
    actual="$(grep -F -- "$needle" "$path" | wc -l | tr -d ' ')"
    if [ "$actual" != "$expected" ]; then
        echo "expected $path to contain $expected occurrences of: $needle" >&2
        echo "actual occurrences: $actual" >&2
        cat "$path" >&2 || true
        exit 1
    fi
}

assert_tree_files_not_contains() {
    root="$1"
    file_pattern="$2"
    needle="$3"
    if [ ! -d "$root" ]; then
        echo "expected generated directory: $root" >&2
        exit 1
    fi
    matches="$(
        find "$root" -type f -name "$file_pattern" -exec grep -lF -- "$needle" {} + 2>/dev/null || true
    )"
    if [ -n "$matches" ]; then
        echo "expected files under $root matching $file_pattern not to contain: $needle" >&2
        printf '%s\n' "$matches" >&2
        exit 1
    fi
}

assert_file_manifest_equals() {
    require_test_tmpdir
    root="$1"
    expected="$2"
    actual="$TMPDIR/actual-manifest.txt"
    if [ ! -d "$root" ]; then
        echo "expected generated directory: $root" >&2
        exit 1
    fi
    (cd "$root" && find . -type f | sed 's#^\./##' | LC_ALL=C sort) > "$actual"
    if ! diff -u "$expected" "$actual" >/dev/null 2>&1; then
        echo "generated file manifest differed for: $root" >&2
        diff -u "$expected" "$actual" >&2 || true
        exit 1
    fi
}
