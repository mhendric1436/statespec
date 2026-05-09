# Getting Started With `.sspec` Files

This guide shows the basic workflow for writing a StateSpec file, validating it, and
generating runtime artifacts.

## 1. Create A Specification File

StateSpec files use the `.sspec` extension.

A minimal file has an optional version header and one `system` declaration:

```statespec
statespec 0.1;

system OrderSystem {
  entity Order {
    key order_id

    fields {
      order_id string
      status string
      created_at timestamp
    }
  }

  generate mt
}
```

## 2. Validate The File

Build the CLI and validate the spec:

```sh
make cli
./build/bin/statespec validate examples/order-system.sspec
```

Expected success output:

```text
valid
```

If validation fails, the CLI prints source locations and diagnostic codes.

## 3. Inspect The AST

Use the `ast` command when debugging how the parser interpreted a file:

```sh
./build/bin/statespec ast examples/order-system.sspec
```

This is useful when a declaration parses successfully but does not validate the way you
expected.

## 4. Generate One Target

Generate a specific target with either a `generate` declaration inside the file or a CLI
target override.

```sh
./build/bin/statespec generate examples/order-system.sspec mt
```

To write generated files under a specific output root:

```sh
./build/bin/statespec generate examples/order-system.sspec mt --out build/generated/mt
```

## 5. Generate All Concrete Targets

Use `all` to expand the supported concrete generator targets:

```sh
./build/bin/statespec generate examples/order-system.sspec all
```

With no `--out`, generated files are written to target-specific default roots:

```text
generated/mt/
generated/dl/
generated/qu/
generated/wf/
generated/openapi/
```

With `--out`, each target is written under the supplied root:

```sh
./build/bin/statespec generate examples/order-system.sspec all --out build/generated
```

Expected layout:

```text
build/generated/mt/
build/generated/dl/
build/generated/qu/
build/generated/wf/
build/generated/openapi/
```

## 6. Authoring Loop

A practical authoring loop is:

```sh
make cli
./build/bin/statespec validate system.sspec
./build/bin/statespec ast system.sspec
./build/bin/statespec generate system.sspec all --out build/generated
```

Run this loop whenever you add a new entity, state machine, workflow, queue, lease, API,
or generator declaration.
