#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
. "$SCRIPT_DIR/common.sh"

run_expect_status 0 "$CLI" help
assert_output_contains "usage:"
assert_output_contains "statespec help"
assert_output_contains "statespec fmt [--check] <file.sspec>"
assert_output_contains "statespec generate bindings --lang <cpp|go|java|rust>"
assert_output_contains "[--tier <all|common|api|worker>]"
assert_output_contains "[--template-root DIR]"
assert_output_contains "statespec generate openapi <file.sspec> [--out DIR]"

run_expect_status 0 "$CLI" --help
assert_output_contains "statespec validate <file.sspec>"

run_expect_status 0 "$CLI" -h
assert_output_contains "statespec ast <file.sspec>"
