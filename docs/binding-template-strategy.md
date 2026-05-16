# Binding Template Strategy

StateSpec binding generation currently uses the checked-in reference bindings as the
source templates for generated language outputs, then adds model-derived descriptor files
beside those copied bindings.

This document defines the template strategy and the constraints that model-driven
generation should preserve.

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
It then emits model-derived descriptor files from the parsed `.sspec` model.

This keeps the generated binding surface aligned with the reference binding source files
and avoids maintaining a second copy of nearly identical templates.

## Current Generator Behavior

The language generators are implemented as deterministic template-copy generators with
additive model-derived descriptor generation:

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
bindings/cpp/json.hpp
bindings/cpp/backend.hpp
bindings/cpp/feature_flag.hpp
bindings/cpp/lease.hpp
bindings/cpp/log.hpp
bindings/cpp/metric.hpp
bindings/cpp/queue.hpp
bindings/cpp/workflow.hpp
```

Generated output layout:

```text
<out>/json.hpp
<out>/backend.hpp
<out>/feature_flag.hpp
<out>/lease.hpp
<out>/log.hpp
<out>/metric.hpp
<out>/queue.hpp
<out>/workflow.hpp
<out>/system_descriptors.hpp
```

### Go

Template source:

```text
bindings/go/backend/json.go
bindings/go/backend/backend.go
bindings/go/backend/feature_flag.go
bindings/go/backend/lease.go
bindings/go/backend/log.go
bindings/go/backend/metric.go
bindings/go/backend/queue.go
bindings/go/backend/workflow.go
```

Generated output layout:

```text
<out>/backend/json.go
<out>/backend/backend.go
<out>/backend/feature_flag.go
<out>/backend/lease.go
<out>/backend/log.go
<out>/backend/metric.go
<out>/backend/queue.go
<out>/backend/workflow.go
<out>/backend/descriptors.go
```

### Java

Template source:

```text
bindings/java/com/statespec/backend/Json.java
bindings/java/com/statespec/backend/Backend.java
bindings/java/com/statespec/backend/FeatureFlag.java
bindings/java/com/statespec/backend/Lease.java
bindings/java/com/statespec/backend/Log.java
bindings/java/com/statespec/backend/Metric.java
bindings/java/com/statespec/backend/Queue.java
bindings/java/com/statespec/backend/Workflow.java
```

Generated output layout:

```text
<out>/com/statespec/backend/Json.java
<out>/com/statespec/backend/Backend.java
<out>/com/statespec/backend/FeatureFlag.java
<out>/com/statespec/backend/Lease.java
<out>/com/statespec/backend/Log.java
<out>/com/statespec/backend/Metric.java
<out>/com/statespec/backend/Queue.java
<out>/com/statespec/backend/Workflow.java
<out>/com/statespec/generated/Descriptors.java
```

### Rust

Template source:

```text
bindings/rust/json.rs
bindings/rust/backend.rs
bindings/rust/feature_flag.rs
bindings/rust/lease.rs
bindings/rust/log.rs
bindings/rust/metric.rs
bindings/rust/queue.rs
bindings/rust/workflow.rs
```

Generated output layout:

```text
<out>/json.rs
<out>/backend.rs
<out>/feature_flag.rs
<out>/lease.rs
<out>/log.rs
<out>/metric.rs
<out>/queue.rs
<out>/workflow.rs
<out>/descriptors.rs
```

## Model-Derived Descriptor Generation

The generator emits collection descriptor artifacts from entity declarations.

Current descriptor generation includes:

```text
CollectionDescriptor records
FieldDescriptor records
key_fields
schema_version = 1
```

The current AST does not yet expose explicit entity index declarations, so generated
`indexes` lists are empty. Once index declarations are added to the grammar and AST, the
same descriptor files should populate `IndexDescriptor` values from the parsed model.

## Why Use Reference Bindings as Templates?

### One source of truth

The binding files under `bindings/` are both human-readable reference interfaces and the
source for generated output. This avoids drift between documentation, reference code, and
generator templates.

### Deterministic generation

The generator copies files byte-for-byte into deterministic paths and emits descriptor
files from the parsed model in declaration order. The same input binding files, language,
StateSpec model, and output directory produce the same generated files.

### Simple review model

Binding API changes are reviewed directly in the `bindings/` tree. Once committed, those
changes automatically become the emitted binding templates.

### Good model-driven step

This strategy allows the CLI, dispatch layer, output layout, and tests to stabilize while
model-derived files are added incrementally beside the reference bindings.

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

The current strategy is accepted when:

```text
binding template strategy is documented
generated output is deterministic
generator sources use language-based binding names
generator sources do not introduce runtime-specific target awareness
CLI tests verify generated binding files for all four languages
CLI tests verify generated descriptor files for all four languages
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

Later milestones can add more model-derived files beside the copied binding templates.

Recommended future outputs:

```text
queue definitions
lease definitions
workflow definitions
workflow step definitions
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
