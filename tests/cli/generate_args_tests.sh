#!/bin/sh
set -eu

CLI="$1"
TMPDIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TMPDIR" generated/cpp generated/go generated/java generated/rust generated/openapi
    rmdir generated 2>/dev/null || true
}
trap cleanup EXIT

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
TESTS_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
. "$SCRIPT_DIR/common.sh"

SPEC="$TESTS_DIR/fixtures/bindings-full.sspec"
NO_SYSTEM_SPEC="$TESTS_DIR/fixtures/no-system.sspec"

run_expect_status 0 "$CLI" generate bindings --lang cpp "$SPEC"
assert_output_contains "generated generated/cpp/json.hpp"
assert_file_exists "generated/cpp/backend.hpp"
assert_file_exists "generated/cpp/system_descriptors.hpp"
rm -rf generated/cpp
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang go "$SPEC"
assert_output_contains "generated generated/go/backend/json.go"
assert_file_exists "generated/go/backend/backend.go"
assert_file_exists "generated/go/backend/descriptors.go"
rm -rf generated/go
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang java "$SPEC"
assert_output_contains "generated generated/java/com/statespec/backend/Json.java"
assert_file_exists "generated/java/com/statespec/backend/Backend.java"
assert_file_exists "generated/java/com/statespec/generated/Descriptors.java"
rm -rf generated/java
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang rust "$SPEC"
assert_output_contains "generated generated/rust/json.rs"
assert_file_exists "generated/rust/backend.rs"
assert_file_exists "generated/rust/descriptors.rs"
rm -rf generated/rust
rmdir generated 2>/dev/null || true

run_expect_status 0 "$CLI" generate bindings --lang cpp "$SPEC" --out "$TMPDIR/tier-common" --tier common
assert_file_exists "$TMPDIR/tier-common/system_descriptors.hpp"
assert_file_not_exists "$TMPDIR/tier-common/api_artifacts.hpp"
assert_file_not_exists "$TMPDIR/tier-common/worker_artifacts.hpp"

run_expect_status 0 "$CLI" generate bindings --lang cpp "$SPEC" --out "$TMPDIR/tier-api" --tier api
assert_file_exists "$TMPDIR/tier-api/system_descriptors.hpp"
assert_file_exists "$TMPDIR/tier-api/api_artifacts.hpp"
assert_file_not_exists "$TMPDIR/tier-api/worker_artifacts.hpp"

run_expect_status 0 "$CLI" generate bindings --lang cpp "$SPEC" --out "$TMPDIR/tier-worker" --tier worker
assert_file_exists "$TMPDIR/tier-worker/system_descriptors.hpp"
assert_file_not_exists "$TMPDIR/tier-worker/api_artifacts.hpp"
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
