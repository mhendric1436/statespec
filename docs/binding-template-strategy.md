# Binding Template Strategy

StateSpec binding generation currently uses the checked-in reference bindings as the
source templates for generated language outputs.

This document defines the Milestone 6 template strategy and the constraints that future
model-driven generation should preserve.

## Decision

Use the existing binding directories as the canonical template source for Phase 1 binding
generation:

```text
bindings/cpp/
bindings/go/
bindings/java/
bindings/rust/
```

The generator reads those files and emits them into the user-selected output directory.

This keeps the generated binding surface aligned with the reference binding source files
and avoids maintaining a second copy of nearly identical templates.

## Current Generator Behavior

The language generators are implemented as deterministic template-copy generators:

```text
src/generator_cpp.cpp
src/generator_go.cpp
src/generator_java.cpp
src/generator_rust.cpp
```

The dispatch entry point is:

```text
src/generator_bindings.cpp
```

The public generator API is:

```text
include/statespec/generator_bindings.hpp
include/statespec/generator_cpp.hpp
include/statespec/generator_go.hpp
include/statespec/generator_java.hpp
include/statespec/generator_rust.hpp
```

## Template Inputs and Outputs

### C++

Template source:

```text
bindings/cpp/backend.hpp
bindings/cpp/lease.hpp
bindings/cpp/queue.hpp
bindings/cpp/workflow.hpp
```

Generated output layout:

```text
<out>/backend.hpp
<out>/lease.hpp
<out>/queue.hpp
<out>/workflow.hpp
```

### Go

Template source:

```text
bindings/go/backend/backend.go
bindings/go/backend/lease.go
bindings/go/backend/queue.go
bindings/go/backend/workflow.go
```

Generated output layout:

```text
<out>/backend/backend.go
<out>/backend/lease.go
<out>/backend/queue.go
<out>/backend/workflow.go
```

### Java

Template source:

```text
bindings/java/com/statespec/backend/Backend.java
bindings/java/com/statespec/backend/Lease.java
bindings/java/com/statespec/backend/Queue.java
bindings/java/com/statespec/backend/Workflow.java
```

Generated output layout:

```text
<out>/com/statespec/backend/Backend.java
<out>/com/statespec/backend/Lease.java
<out>/com/statespec/backend/Queue.java
<out>/com/statespec/backend/Workflow.java
```

### Rust

Template source:

```text
bindings/rust/backend.rs
bindings/rust/lease.rs
bindings/rust/queue.rs
bindings/rust/workflow.rs
```

Generated output layout:

```text
<out>/backend.rs
<out>/lease.rs
<out>/queue.rs
<out>/workflow.rs
```

## Why Use Reference Bindings as Templates?

### One source of truth

The binding files under `bindings/` are both human-readable reference interfaces and the
source for generated output. This avoids drift between documentation, reference code, and
generator templates.

### Deterministic generation

The generator copies files byte-for-byte into deterministic paths. The same input binding
files, language, and output directory produce the same generated files.

### Simple review model

Binding API changes are reviewed directly in the `bindings/` tree. Once committed, those
changes automatically become the emitted binding templates.

### Good pre-model-driven step

This strategy allows the CLI, dispatch layer, output layout, and tests to stabilize
before adding model-derived descriptor generation.

## Constraints

Generated binding outputs must remain independent of any runtime-specific target names.
The binding generator must not branch on or emit names tied to previous target-specific
runtime generators.

The active generator model should use only:

```text
BindingLanguage
BindingGeneratorOptions
generate_bindings
generate_cpp_bindings
generate_go_bindings
generate_java_bindings
generate_rust_bindings
```

Reference binding files should describe generalized backend abstractions and generalized
runtime component abstractions only.

## Error Handling

If a template file cannot be read, the generator reports:

```text
SSPEC5201 failed to read binding template: <path>
```

This makes missing or moved template files visible during generation and tests.

## Acceptance Criteria

Milestone 6 is accepted when:

```text
binding template strategy is documented
generated output is deterministic
generator sources use language-based binding names
generator sources do not introduce runtime-specific target awareness
CLI tests verify generated files for all four languages
```

The current CLI test coverage is in:

```text
tests/cli_generate_bindings_tests.sh
```

It verifies generated file paths for:

```text
cpp
go
java
rust
```

## Future Model-Driven Generation

Later milestones can add model-derived files beside the copied binding templates.

Recommended future outputs:

```text
cpp/system_descriptors.hpp
go/backend/descriptors.go
java/com/statespec/generated/Descriptors.java
rust/descriptors.rs
```

Those files should use the generalized backend abstraction types:

```text
CollectionDescriptor
FieldDescriptor
IndexDescriptor
QueueDefinition
WorkflowDefinition
WorkflowStepDefinition
```

Model-derived generation should be additive. The reference binding templates should
remain stable and continue to define the shared API surface.

## Alternative Considered

A separate `templates/` directory was considered:

```text
templates/bindings/cpp/
templates/bindings/go/
templates/bindings/java/
templates/bindings/rust/
```

That was rejected for now because it creates a second copy of the same binding source and
increases the risk of drift.

A separate template directory can be introduced later only if the generator needs template
features that should not appear in the reference binding source files.
