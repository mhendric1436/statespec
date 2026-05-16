# StateSpec Language Model

A StateSpec file describes a distributed system as a set of durable concepts that can be
validated and lowered into runtime artifacts.

## Root Structure

A file normally starts with a version header and then declares a system:

```statespec
statespec 0.1;

system OrderSystem {
  // declarations go here
}
```

The `system` block is the top-level scope for names. Names declared inside a system are
referenced by APIs, workflows, queues, leases, workers, and policies.

Files may also declare `include` directives before imports and the root system:

```statespec
statespec 0.1;
include "./workflow-launch-control.sspec";

system OrderSystem {
  // local declarations go here
}
```

`statespec validate` and `statespec generate bindings` resolve include paths relative to
the including file, detect include cycles, and operate on the composed system. Included
system declarations contribute their members to the root system; the root file keeps the
final system name.

`statespec ast` continues to show the parsed file-level AST without expanding includes.
Formatting also operates on the file being formatted and does not inline included files.

See [includes.md](includes.md) for detailed include resolution and system composition
rules.

## First-Class Concepts

| Concept | Purpose |
|---|---|
| `entity` | Durable domain object with keys, fields, indexes, invariants, and lifecycle state. |
| `state_machine` | Allowed lifecycle states and transitions for an entity. |
| `queue` | Durable async command or event channel. |
| `message` | Typed payload carried by a queue. |
| `lease` | Exclusive ownership or fencing primitive. |
| `worker` | Runtime actor that polls queues or executes workflows. |
| `feature_flag` | Declared rollout or configuration switch referenced from expressions. |
| `log` | Structured event contract for operational logging. |
| `metric` | Counter, gauge, or histogram contract for operational measurement. |
| `workflow` | Long-running orchestration composed from named steps. |
| `step` | Unit of workflow execution with timing and retry metadata. |
| `api` | External operation such as a REST endpoint. |
| `policy` | Authorization, tenancy, quota, and audit intent. |

## Backend-Neutral Authoring

The StateSpec language should describe the system, not the implementation details of a
specific runtime.

Prefer this:

```statespec
entity Order {
  key order_id

  fields {
    order_id string
    created_at timestamp
    updated_at timestamp
    status string
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
```

Avoid putting runtime-specific implementation symbols in the model:

```statespec
// Avoid using runtime implementation names as domain concepts.
entity mt_Table_Order {
  key id

  fields {
    id string
  }
}
```

Runtime-specific output belongs in tooling and binding generators, not in the canonical
model.

## Observability

Logs and metrics are top-level system declarations:

```statespec
log WorkflowLaunchDecision {
  level info
  event_name "workflow.launch.decision"

  fields {
    tenant_id string
    decision string
  }
}

metric WorkflowLaunchAttempts {
  kind counter
  name "workflow_launch_attempts_total"
  unit count

  labels {
    tenant_id string
    decision string
  }
}
```

The declarations define stable signal names and typed payloads. They do not select a
specific logging framework, metrics exporter, backend, sampling policy, or storage
implementation.

See [observability.md](observability.md) for validation rules, generated descriptor
behavior, and runtime binding contracts.

## Type Model

Common scalar types include:

```text
string
bool
int
int32
int64
long
double
decimal
json
timestamp
duration
uuid
```

Optional fields can be expressed with a `?` suffix:

```statespec
fields {
  nickname string?
}
```

Shapes define reusable named payloads for APIs, events, logs, metrics, messages, and
future generated validators:

```statespec
shape StartOrderProcessingRequest {
  tenant_id string
  order_id string
  priority int?
}

api StartOrderProcessing {
  method POST
  path "/v1/orders/{order_id}/start"
  input StartOrderProcessingRequest
}
```

The compiler parses, validates, lowers, formats, and emits descriptors for shapes. Target
language generators do not yet emit dedicated request/response structs or classes from
shape declarations.

The grammar also reserves richer type forms such as `optional<T>`, `list<T>`, `set<T>`,
`map<K,V>`, and `ref<T>`. Support in individual binding generators may lag behind the
grammar as the compiler matures.

## References

References use declared names:

```statespec
api StartOrderProcessing {
  method POST
  path "/v1/orders/{orderId}/start"
  starts workflow OrderProcessing
}
```

Validation should ensure referenced identifiers exist and are used in a compatible
context.

## Binding Generation

Generated bindings are selected outside the `.sspec` file:

```sh
statespec generate bindings --lang cpp system.sspec --out generated/cpp
```

Supported binding languages are `cpp`, `go`, `java`, and `rust`.
