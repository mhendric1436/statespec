# Getting Started With `.sspec` Files

This guide shows the basic workflow for writing a StateSpec file, validating it, and
generating language bindings.

## 1. Create A Specification File

StateSpec files use the `.sspec` extension.

A minimal file has an optional version header and one `system` declaration:

```statespec
statespec 0.1;

system OrderSystem {
  entity Order {
    key order_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }

    fields {
      created_at timestamp
      updated_at timestamp
      status string
      order_id string
    }

    state_machine {
      state Pending
      state Completed {
        terminal: true
        garbage_collection {
          after: P30D
          mode: tombstone
        }
      }

      initial Pending
      terminal [Completed]

      Pending -> Completed
    }
  }
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

## 4. Generate Bindings

Binding generation is selected by CLI option, not by declarations inside `.sspec` text:

```sh
./build/bin/statespec generate bindings --lang cpp examples/order-system.sspec --out build/generated/cpp
```

Supported languages are `cpp`, `go`, `java`, and `rust`.

## 5. Authoring Loop

A practical authoring loop is:

```sh
make cli
./build/bin/statespec validate system.sspec
./build/bin/statespec ast system.sspec
./build/bin/statespec generate bindings --lang cpp system.sspec --out build/generated/cpp
```

Run this loop whenever you add a new entity, state machine, workflow, queue, lease, API,
or policy declaration.
