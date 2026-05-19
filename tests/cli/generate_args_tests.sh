#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR"
}
trap cleanup EXIT

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TESTS_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
. "$SCRIPT_DIR/common.sh"

SPEC="$TESTS_DIR/fixtures/bindings-full.sspec"
NO_SYSTEM_SPEC="$TESTS_DIR/fixtures/no-system.sspec"

run_expect_status 0 "$CLI" generate bindings --lang cpp "$SPEC" --out "$TMPDIR/out-cpp"
assert_output_contains "generated $TMPDIR/out-cpp/common/json.hpp"
assert_file_exists "$TMPDIR/out-cpp/common/backend.hpp"
assert_file_exists "$TMPDIR/out-cpp/common/system_descriptors.hpp"

run_expect_status 0 "$CLI" generate bindings --lang go "$SPEC" --out "$TMPDIR/out-go"
assert_output_contains "generated $TMPDIR/out-go/common/backend/json.go"
assert_file_exists "$TMPDIR/out-go/common/backend/backend.go"
assert_file_exists "$TMPDIR/out-go/common/backend/descriptors.go"

run_expect_status 0 "$CLI" generate bindings --lang java "$SPEC" --out "$TMPDIR/out-java"
assert_output_contains "generated $TMPDIR/out-java/common/com/statespec/backend/Json.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/backend/Backend.java"
assert_file_exists "$TMPDIR/out-java/common/com/statespec/generated/Descriptors.java"

run_expect_status 0 "$CLI" generate bindings --lang rust "$SPEC" --out "$TMPDIR/out-rust"
assert_output_contains "generated $TMPDIR/out-rust/common/json.rs"
assert_file_exists "$TMPDIR/out-rust/common/backend.rs"
assert_file_exists "$TMPDIR/out-rust/common/descriptors.rs"

run_expect_status 0 "$CLI" generate bindings --lang cpp "$SPEC" --out "$TMPDIR/tier-common" --tier common
assert_file_exists "$TMPDIR/tier-common/common/system_descriptors.hpp"
assert_file_not_exists "$TMPDIR/tier-common/api/api_descriptors.hpp"
assert_file_not_exists "$TMPDIR/tier-common/worker_artifacts.hpp"

run_expect_status 0 "$CLI" generate bindings --lang cpp "$SPEC" --out "$TMPDIR/tier-api" --tier api
assert_file_exists "$TMPDIR/tier-api/common/system_descriptors.hpp"
assert_file_exists "$TMPDIR/tier-api/api/api_descriptors.hpp"
assert_file_not_exists "$TMPDIR/tier-api/worker_artifacts.hpp"

run_expect_status 0 "$CLI" generate bindings --lang cpp "$SPEC" --out "$TMPDIR/tier-worker" --tier worker
assert_file_exists "$TMPDIR/tier-worker/common/system_descriptors.hpp"
assert_file_not_exists "$TMPDIR/tier-worker/api/api_descriptors.hpp"
assert_file_exists "$TMPDIR/tier-worker/worker_artifacts.hpp"

run_expect_status 2 "$CLI" generate bindings "$SPEC"
assert_output_contains "generate bindings requires --lang <cpp|go|java|rust>"

run_expect_status 2 "$CLI" generate bindings --lang
assert_output_contains "--lang requires one of: cpp|go|java|rust"

run_expect_status 2 "$CLI" generate bindings --lang python "$SPEC"
assert_output_contains "unsupported binding language 'python'"
assert_output_contains "cpp|go|java|rust"

run_expect_status 2 "$CLI" generate bindings --lang cpp
assert_output_contains "generate bindings requires an input .sspec file"

run_expect_status 2 "$CLI" generate bindings --lang cpp "$SPEC" --out
assert_output_contains "--out requires a directory"

run_expect_status 2 "$CLI" generate bindings --lang cpp "$SPEC" --tier
assert_output_contains "--tier requires one of: all|common|api|worker"

run_expect_status 2 "$CLI" generate bindings --lang cpp "$SPEC" --tier database
assert_output_contains "unsupported binding generation tier 'database'"
assert_output_contains "all|common|api|worker"

run_expect_status 2 "$CLI" generate bindings --lang cpp "$SPEC" extra
assert_output_contains "unexpected argument for generate bindings: extra"

run_expect_status 2 "$CLI" generate openapi
assert_output_contains "generate openapi requires an input .sspec file"

run_expect_status 2 "$CLI" generate openapi "$SPEC" --out
assert_output_contains "--out requires a directory"

run_expect_status 2 "$CLI" generate openapi "$SPEC" extra
assert_output_contains "unexpected argument for generate openapi: extra"

run_expect_status 1 "$CLI" generate bindings --lang cpp "$NO_SYSTEM_SPEC" --out "$TMPDIR/no-system"
assert_output_contains "SSPEC0201"
assert_output_contains "expected system declaration"

run_expect_status 2 "$CLI" generate "$SPEC" legacy
assert_output_contains "unsupported generate kind: $SPEC"

run_expect_status 2 "$CLI" generate legacy "$SPEC"
assert_output_contains "unsupported generate kind: legacy"
