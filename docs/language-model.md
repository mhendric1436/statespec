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

## First-Class Concepts

| Concept | Purpose |
|---|---|
| `entity` | Durable domain object with keys, fields, indexes, invariants, and lifecycle state. |
| `state_machine` | Allowed lifecycle states and transitions for an entity. |
| `queue` | Durable async command or event channel. |
| `message` | Typed payload carried by a queue. |
| `lease` | Exclusive ownership or fencing primitive. |
| `worker` | Runtime actor that polls queues or executes workflows. |
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
    status string
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

The grammar also reserves richer type forms such as `optional<T>`, `list<T>`, `set<T>`,
`map<K,V>`, `ref<T>`, and named shapes. Support in individual binding generators may lag
behind the grammar as the compiler matures.

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
