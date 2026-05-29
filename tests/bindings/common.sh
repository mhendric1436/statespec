#!/bin/sh

binding_test_init() {
    CLI="$1"
    SCRIPT_PATH="$2"
    TMPDIR="$(mktemp -d)"
    cleanup() {
        rm -rf "$TMPDIR"
    }
    trap cleanup EXIT

    SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$SCRIPT_PATH")" && pwd)"
    TESTS_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
    . "$TESTS_DIR/cli/common.sh"
    SPEC="$TESTS_DIR/fixtures/bindings-full.sspec"
}

generate_binding_fixture() {
    lang="$1"
    out_dir="$2"
    expected_generated_file="$3"
    run_expect_status 0 "$CLI" generate bindings --lang "$lang" "$SPEC" --out "$out_dir"
    assert_output_contains "generated $out_dir/$expected_generated_file"
}

assert_generated_files_exist() {
    root="$1"
    while IFS= read -r path; do
        if [ -n "$path" ]; then
            assert_file_exists "$root/$path"
        fi
    done
}

assert_generated_files_absent() {
    root="$1"
    while IFS= read -r path; do
        if [ -n "$path" ]; then
            assert_file_not_exists "$root/$path"
        fi
    done
}

assert_generated_tree_has_no_format_guards() {
    root="$1"
    extension="$2"
    assert_tree_files_not_contains "$root" "*.$extension" "// clang-format off"
    assert_tree_files_not_contains "$root" "*.$extension" "// clang-format on"
}
