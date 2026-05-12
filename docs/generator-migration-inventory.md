# Generator Migration Inventory

This document inventories the current StateSpec generator surface before migrating from
runtime-specific targets to language-specific backend binding generation.

Milestone 1 is informational only. It records the current CLI shape, generator dispatch,
output paths, tests/docs touch points, and recommended migration actions for later
milestones.

## Migration Goal

Replace runtime-specific generation targets with language binding generation.

Current runtime-specific targets to remove from active generator behavior:

```text
mt
dl
qu
wf
```

Target generator model:

```text
statespec generate bindings --lang cpp  <file.sspec> --out <dir>
statespec generate bindings --lang go   <file.sspec> --out <dir>
statespec generate bindings --lang java <file.sspec> --out <dir>
statespec generate bindings --lang rust <file.sspec> --out <dir>
```

## Current CLI Surface

Current CLI usage is implemented in `cmd/statespec.cpp`.

Current usage text:

```text
statespec validate <file.sspec>
statespec tokens <file.sspec>
statespec ast <file.sspec>
statespec generate <file.sspec> [target] [--out DIR]
```

Current generate parsing behavior:

```text
argv[1] = command
argv[2] = file path
generate target is optional positional argument after the file path
--out DIR is optional
```

Current generate call path:

```text
main
  -> generate_file(path, target, out)
  -> parse_and_validate_file(...)
  -> Generator::generate(spec, options, diagnostics)
```

Current `GenerationOptions` fields:

```text
dry_run
target_override
out_override
```

## Current Generator Dispatch Surface

Current generator public API is in:

```text
include/statespec/generator.hpp
```

Important current types:

```text
GeneratedFile
GenerationOptions
GenerationResult
Generator
```

Current generator implementation is in:

```text
src/generator.cpp
```

The current dispatch model selects `generate` declarations from the parsed system and
optionally applies `target_override` from the CLI.

If no matching declaration exists and a target override is present, the generator creates
an implicit `GenerateDecl` for that target.

## Current Supported Targets

`src/generator.cpp` currently declares this supported target set:

```text
mt
dl
qu
wf
openapi
proto
docs
tests
```

The special target `all` expands to:

```text
mt
dl
qu
wf
openapi
```

## Current Runtime-Specific Dispatch Branches

`src/generator.cpp` currently dispatches runtime-specific targets as follows:

```text
mt -> generator_backend::generate_mt(...)
dl -> generator_backend::generate_dl(...)
qu -> generator_backend::generate_qu(...)
wf -> generator_backend::generate_wf(...)
```

These dispatch branches should be removed in the migration milestone that replaces
runtime-specific generator behavior with language binding generation.

## Current Runtime-Specific Generator Declarations

`src/generator_backend.hpp` currently declares runtime-specific generator functions:

```text
generate_mt
generate_dl
generate_qu
generate_wf
```

It also declares generalized or non-runtime helper generation:

```text
generate_openapi
generate_scaffold
```

The helper namespace also contains path, type, and formatting utilities that may remain
useful during the migration:

```text
join_path
output_root
bool_text
optional_or_empty
optional_bool_or_empty
is_optional_type
strip_optional_suffix
to_lower
cpp_type_for_statespec_type
append_header
```

## Current Runtime-Specific Source Files

The current runtime-specific generator implementations are:

```text
src/generator_mt.cpp
src/generator_dl.cpp
src/generator_qu.cpp
src/generator_wf.cpp
```

### `src/generator_mt.cpp`

Current responsibility:

```text
entity-oriented metadata and mt codegen schema generation
```

Current generated outputs include:

```text
mt-manifest.yaml
schemas/<entity>.mt.json
mt_entities.hpp
mt_metadata.cpp
mt-state-machines.yaml
```

Current namespace/output identity includes:

```text
statespec_generated::mt
```

Recommended migration action:

```text
remove from active generator dispatch
salvage entity field/key/index extraction logic for future backend descriptor generation
replace mt-specific JSON schema output with backend-neutral CollectionDescriptor output
```

### `src/generator_dl.cpp`

Current responsibility:

```text
lease and worker lease metadata generation
```

Current generated outputs include:

```text
dl-manifest.yaml
dl_leases.hpp
dl_metadata.cpp
```

Current namespace/output identity includes:

```text
statespec_generated::dl
```

Recommended migration action:

```text
remove from active generator dispatch
salvage lease metadata extraction for future generalized lease definition generation
replace dl-specific names with backend abstraction lease definition artifacts
```

### `src/generator_qu.cpp`

Current responsibility:

```text
queue, message, payload, and idempotency metadata generation
```

Current generated outputs include:

```text
qu-manifest.yaml
qu_messages.hpp
qu_metadata.cpp
```

Current namespace/output identity includes:

```text
statespec_generated::qu
```

Recommended migration action:

```text
remove from active generator dispatch
salvage queue/message extraction for future generalized QueueDefinition output
replace qu-specific names with backend abstraction queue definition artifacts
```

### `src/generator_wf.cpp`

Current responsibility:

```text
workflow and workflow step metadata generation
```

Current generated outputs include:

```text
wf-manifest.yaml
wf_workflows.hpp
wf_metadata.cpp
```

Current namespace/output identity includes:

```text
statespec_generated::wf
```

Recommended migration action:

```text
remove from active generator dispatch
salvage workflow extraction for future generalized WorkflowDefinition output
preserve immutable workflow version semantics in new descriptor generation
```

## Current Build Surface

The Makefile discovers source and header files with wildcards:

```text
HEADERS := $(wildcard include/statespec/*.hpp)
SRC_ALL := $(wildcard src/*.cpp)
SRC := $(SRC_ALL)
CLI_SRC := $(wildcard cmd/*.cpp)
TEST_SRC := $(wildcard tests/*.cpp)
TEST_SCRIPTS := $(wildcard tests/*_tests.sh)
```

Because `SRC_ALL` includes all `src/*.cpp`, removing or adding generator files only
requires file-level changes, not a hard-coded source-list update.

Recommended migration action:

```text
remove old generator source files only after active dispatch no longer references them
add new generator source files under src/ and headers under include/statespec/
let Makefile wildcard discovery pick them up
```

## Current Output Root Behavior

`generator_backend::output_root(...)` currently resolves output root in this order:

```text
options.out_override
declaration.out
generated/<target>
```

For the future language binding generator, the equivalent default should be:

```text
generated/<language>
```

or, for the proposed CLI shape:

```text
--out value when specified
otherwise generated/<lang>
```

## Current `all` Behavior

The current `all` target expands to runtime-specific outputs plus OpenAPI:

```text
mt
dl
qu
wf
openapi
```

Recommended migration action:

```text
remove all as a runtime aggregation target
if retained later, redefine it as a language-independent utility only after the new generator model is stable
```

## Current Docs Surface

Known docs already updated toward backend abstraction language:

```text
README.md
docs/backend-abstractions.md
bindings/cpp/README.md
bindings/go/README.md
bindings/java/README.md
bindings/rust/README.md
```

Recommended migration action:

```text
search README.md docs/ bindings/ for old runtime target names before final acceptance
old target names should remain only in migration inventory, migration tests, or release notes
```

## Current Test Surface

The Makefile discovers tests through:

```text
tests/*.cpp
tests/*_tests.sh
```

Milestone 1 did not modify tests. Later milestones should add explicit tests for:

```text
statespec generate bindings --lang cpp
statespec generate bindings --lang go
statespec generate bindings --lang java
statespec generate bindings --lang rust
```

and negative migration tests for old targets:

```text
statespec generate <file.sspec> mt
statespec generate <file.sspec> dl
statespec generate <file.sspec> qu
statespec generate <file.sspec> wf
```

Expected negative behavior:

```text
non-zero exit code
clear migration guidance to use generate bindings --lang <cpp|go|java|rust>
```

## Recommended Files to Keep Temporarily

Keep these until the new binding generator is implemented and tests are passing:

```text
src/generator_mt.cpp
src/generator_dl.cpp
src/generator_qu.cpp
src/generator_wf.cpp
src/generator_backend.hpp
src/generator.cpp
include/statespec/generator.hpp
```

Reason:

```text
they preserve current build behavior while the new language generator is introduced
some extraction logic can be reused for descriptor generation milestones
```

## Recommended Files to Add in Later Milestones

```text
include/statespec/binding_language.hpp
src/binding_language.cpp
include/statespec/generator_bindings.hpp
src/generator_bindings.cpp
include/statespec/generator_cpp.hpp
src/generator_cpp.cpp
include/statespec/generator_go.hpp
src/generator_go.cpp
include/statespec/generator_java.hpp
src/generator_java.cpp
include/statespec/generator_rust.hpp
src/generator_rust.cpp
```

## Recommended Files to Remove Later

After new language binding generation is active and tested, remove or archive:

```text
src/generator_mt.cpp
src/generator_dl.cpp
src/generator_qu.cpp
src/generator_wf.cpp
```

Remove the corresponding declarations from:

```text
src/generator_backend.hpp
```

Remove runtime-specific dispatch branches from:

```text
src/generator.cpp
```

## Migration Risks

### CLI compatibility

Current CLI order is:

```text
statespec generate <file.sspec> [target] [--out DIR]
```

Proposed CLI order is:

```text
statespec generate bindings --lang <language> <file.sspec> --out DIR
```

This is a breaking syntax change and should include migration guidance for old targets.

### Generate declarations in `.sspec` files

Existing `.sspec` files may contain declarations like:

```text
generate mt
generate dl
generate qu
generate wf
generate all
```

Later milestones must decide whether to:

```text
reject these with migration diagnostics
ignore generator declarations when CLI language is explicit
introduce a new language-oriented generator declaration form
```

### Reuse versus deletion

The old generator files contain useful model extraction logic but runtime-specific naming.
The recommended approach is to reuse extraction ideas, not runtime-specific output names,
file names, namespaces, or manifest formats.

## Milestone 1 Acceptance Checklist

- [x] Current CLI shape documented.
- [x] Current generator dispatch behavior documented.
- [x] Current runtime-specific generator source files documented.
- [x] Current output paths documented.
- [x] Current build wildcard behavior documented.
- [x] Recommended files to keep, add, and remove documented.
- [x] Migration risks documented.
- [x] Later test requirements documented.
