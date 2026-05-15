![Project Logo](images/statespec4.png)

# StateSpec

> A canonical design language for distributed systems and their backend-neutral runtime model.

StateSpec is a structured specification language for describing distributed systems with
explicit entities, lifecycle state, APIs, workflows, queues, leases, logs, metrics,
policies, and backend requirements.

A StateSpec file is intended to be the source of truth for system behavior. From that
source, tooling can derive schemas, API contracts, workflow definitions, queue and lease
metadata, log and metric descriptors, validators, tests, diagrams, and integration
scaffolding.

---

## Why StateSpec?

Modern distributed systems are difficult to reason about because behavior is usually
spread across disconnected artifacts:

- API contracts describe external shape but not lifecycle state.
- Database schemas describe storage but not invariants or orchestration.
- Queue and worker code often embeds retry, ownership, and idempotency rules in ad hoc ways.
- Lease and coordination logic is often reimplemented differently across services.
- Logs and metrics are frequently implicit in application code rather than declared as
  stable operational contracts.
- Design documents become stale and are not executable or mechanically validated.

StateSpec addresses this by making system behavior explicit, structured, canonical, and
generator-friendly.

---

## Core Idea

A distributed system can be modeled as durable state plus deterministic transitions over
that state.

StateSpec captures the main concepts in one place:

| Concept | Purpose |
|---|---|
| Entities | Durable domain objects, fields, keys, relationships, indexes, and invariants |
| State machines | Legal lifecycle states, transitions, and transition guards |
| APIs | External contracts and intent-oriented operations |
| Workflows | Long-running asynchronous orchestration |
| Queues | Durable async commands, events, and worker dispatch |
| Leases | Exclusive ownership, leadership, fencing, and coordination |
| Logs | Structured operational event contracts |
| Metrics | Counter, gauge, and histogram contracts |
| Policies | Tenant, authorization, quota, and operational constraints |
| Generators | Deterministic output from the canonical model |

---

## Canonical Language Model

StateSpec models these first-class concepts:

```text
system
entity
state_machine
api
workflow
step
queue
message
lease
log
metric
worker
policy
generator
```

The language should remain backend-neutral. A specification should describe system
semantics rather than binding itself to a specific storage engine, queue implementation,
or workflow runtime.

Example:

```statespec
system OrderSystem {
  entity Order { ... }
  workflow OrderProcessing { ... }
  queue EmailDispatch { ... }
  lease OrderReconciler { ... }
  log WorkflowLaunchDecision { ... }
  metric WorkflowLaunchAttempts { ... }
  api StartOrderProcessing { ... }
}
```

Log and metric declarations are system-level grammar constructs:

```statespec
log Name {
  level info
  event_name "stable.event.name"

  fields {
    field_name string
  }
}

metric Name {
  kind counter
  name "stable_metric_name_total"
  unit count

  labels {
    label_name string
  }
}
```

Logs require `level` and `event_name`. Supported levels are `debug`, `info`, `warn`, and
`error`. Metrics require `kind`, `name`, and `unit`. Supported kinds are `counter`,
`gauge`, and `histogram`.

See [`docs/observability.md`](docs/observability.md) for the full log and metric
authoring guide.

---

## Design Principles

### Canonical structure

There should be one preferred way to express each concept.

### Explicit state

Important lifecycle states, transitions, ownership rules, retry behavior, and invariants
should be represented directly in the specification.

### Deterministic behavior

Generated artifacts should be deterministic. Hidden side effects and ambiguous
transitions should be avoided.

### Backend independence

The language should describe the system without depending on a concrete runtime,
storage engine, database, or transport.

### Durable coordination

Workflow communication, queue dispatch, and exclusive ownership should be expressed in
terms of durable state, durable messages, or explicit leases rather than transient
in-memory coordination.

### Declared observability

Logs and metrics should be declared as stable contracts with typed fields and labels
rather than hidden in runtime implementation code.

### Text is the source of truth

Visual tools, diagrams, generated code, runtime configuration, and tests should be
derived from the canonical text specification.

---

## Backend Abstraction Model

StateSpec defines an OCC-centered backend abstraction so generated runtimes can share one
consistency doctrine across multiple concrete implementations.

The backend model is intentionally small:

```text
CollectionDescriptor
FieldDescriptor
IndexDescriptor
BackendCapabilities
ConflictKind
VersionedRecord
Query
Transaction
Backend
```

The core backend contract provides:

```text
capabilities
ensure_collection
ensure_collections
begin
get
query
put
erase
commit
abort
```

`ensure_collections` accepts a list of `CollectionDescriptor` records so generated
systems can provision all required collections with one API call.

---

## Optimistic Concurrency Control

Optimistic concurrency control is the shared consistency model behind the backend
abstraction.

A transaction reads versioned records, computes deterministic changes, and commits only
if the state it depended on has not changed. If another writer changes a record or
predicate first, the commit reports a semantic conflict and the caller can retry from a
fresh read.

The common rule is:

```text
read versioned state
validate predicates and invariants
prepare deterministic changes
commit only if observed versions are still current
retry safely on conflict
```

Concrete backend adapters may implement this with different native mechanisms:

```text
in-memory maps and version counters
SQL rows, indexes, transactions, and version columns
key-value prefixes and conditional updates
MVCC snapshots and conflict ranges
future durable storage engines
```

StateSpec-generated semantics should depend on the abstraction rather than the physical
implementation details.

---

## Runtime Component Abstractions

The backend bindings define generalized component abstractions for common distributed
systems primitives.

### Leases

Leases model exclusive ownership with expiry and fencing tokens.

Typical operations:

```text
acquire
renew
release
inspect
```

### Queues

Queues model durable messages with explicit queue definitions, visibility timeouts,
claiming, retries, acknowledgement, and optional dead-letter routing.

Typical operations:

```text
create
inspect definition
enqueue
claim
acknowledge
fail
inspect message
```

Queue creation is idempotent. The queue identity is:

```text
(queue, channel)
```

### Workflows

Workflows model immutable workflow definitions and durable workflow executions.

Typical operations:

```text
register definition
inspect definition
start
claim steps
complete step
fail step
cancel
inspect execution
```

Workflow definition registration is immutable. The workflow definition identity is:

```text
(workflow_name, workflow_version)
```

A changed workflow definition should be registered with a new incremented version.

### Logs

Logs model structured operational events with stable backend event names and typed
fields.

Typical operations:

```text
emit log event
```

Example declaration:

```statespec
log WorkflowLaunchDecision {
  level info
  event_name "workflow.launch.decision"

  fields {
    tenant_id string
    workflow_name string
    decision string
  }
}
```

### Metrics

Metrics model operational measurements with stable backend metric names, units, and
low-cardinality labels.

Typical operations:

```text
record metric sample
```

Example declaration:

```statespec
metric WorkflowLaunchAttempts {
  kind counter
  name "workflow_launch_attempts_total"
  unit count

  labels {
    tenant_id string
    workflow_name string
    decision string
  }
}
```

---

## Transaction Call Styles

Runtime component APIs support two call styles.

### Backend-managed calls

Backend-managed methods receive a backend and manage the transaction internally:

```text
begin transaction
perform component operation
commit on success
abort on failure
```

### Caller-managed calls

Transaction methods receive an existing transaction so a caller can compose entity,
lease, queue, workflow, log, metric, and policy operations into one atomic unit.

All bindings use a `Tx` suffix for caller-managed transaction variants, adapted to each
language's naming conventions.

---

## Language Bindings

Reference backend and runtime component bindings are provided for:

| Language | Binding guide |
|---|---|
| C++ | [`bindings/cpp/README.md`](bindings/cpp/README.md) |
| Go | [`bindings/go/README.md`](bindings/go/README.md) |
| Java | [`bindings/java/README.md`](bindings/java/README.md) |
| Rust | [`bindings/rust/README.md`](bindings/rust/README.md) |

The shared binding documentation is in
[`docs/backend-abstractions.md`](docs/backend-abstractions.md).

Generated binding packages include split runtime component files for logs and metrics:

| Language | Log runtime | Metric runtime |
|---|---|---|
| C++ | `log.hpp` | `metric.hpp` |
| Go | `backend/log.go` | `backend/metric.go` |
| Java | `com/statespec/backend/Log.java` | `com/statespec/backend/Metric.java` |
| Rust | `log.rs` | `metric.rs` |

These files expose separate log and metric sink interfaces so applications can depend on
only the runtime surface they need.

Build tool installation and binding-local build instructions are documented in
[`docs/build-tools.md`](docs/build-tools.md).

---

## Repository Layout

```text
bindings/            reference backend and runtime component interfaces
cmd/                 StateSpec CLI entry points
docs/                programmer guides and abstraction documentation
examples/            example specifications
grammar/             grammar reference
include/             public compiler/generator headers
src/                 lexer, parser, validator, and generator implementation
tests/               regression tests
```

---

## Example Specification Shape

```statespec
system OrderSystem {
  entity Order {
    key tenant_id, order_id

    fields {
      tenant_id string
      order_id string
      status string
      created_at timestamp
    }

    state_machine {
      Creating -> Active
      Creating -> Failed
      Active -> Updating
      Active -> Deleting
      Updating -> Active
      Updating -> Failed
      Deleting -> Deleted
      Deleting -> Failed
      Failed -> Deleting
    }
  }

  queue EmailDispatch {
    channel email
    visibility_timeout PT30S
  }

  lease OrderReconciler {
    resource "reconciler:orders"
    ttl PT30S
    fencing_token true
  }

  log WorkflowLaunchDecision {
    level info
    event_name "workflow.launch.decision"

    fields {
      tenant_id string
      order_id string
      decision string
    }
  }

  metric WorkflowLaunchAttempts {
    kind counter
    name "workflow_launch_attempts_total"
    unit count

    labels {
      tenant_id string
      workflow_name string
      decision string
    }
  }

  workflow OrderProcessing {
    version 1
    start validate_order

    step validate_order {
      expected_execution_time PT10S
      max_retries 2
    }

    step send_confirmation {
      expected_execution_time PT10S
      max_retries 3
    }
  }
}
```

---

## What StateSpec Can Generate

From a single `.sspec` file, StateSpec tooling can derive:

| Artifact type | Examples |
|---|---|
| Schemas | entity metadata, collection descriptors, indexes, validation metadata |
| APIs | OpenAPI, RPC contracts, request/response models, error models |
| Runtime metadata | workflow definitions, queue definitions, lease definitions, log definitions, metric definitions |
| Code scaffolding | server stubs, client stubs, integration helpers, worker skeletons |
| Tests | state transition tests, invariant tests, runtime behavior tests |
| Documentation | architecture docs, diagrams, state machine graphs |
| Tooling | syntax highlighting, validation, autocomplete, symbol navigation |

---

## CLI Shape

```sh
statespec validate <file>
statespec fmt <file>
statespec generate <target> <file>
statespec graph <file>
statespec diff <old-file> <new-file>
```

Generator targets are implementation choices layered on top of the canonical model.
StateSpec should keep the source language stable even as generator targets evolve.

---

## Policy Model

StateSpec can represent policy declarations for hosted or multi-tenant systems.

Example:

```statespec
policy WorkflowAccess {
  tenant scoped_by tenant_id

  allow startWorkflowExecution when caller.role in ["operator", "control-plane"]
  allow pollAndClaimWorkflowSteps when caller.role == "worker"
}
```

Policies can derive:

```text
authorization hooks
ownership checks
tenant scoping rules
quota checks
audit event definitions
service-layer validation scaffolding
```

---

## File Extension

```text
.sspec
```

---

## Use Cases

StateSpec is intended for systems where behavior must be precise, durable, and
reviewable:

```text
cloud control planes
infrastructure services
event-driven systems
API and worker architectures
distributed workflow systems
systems with complex lifecycle management
systems requiring explicit state transition rules
internal platforms and service orchestration layers
```

---

## Comparison

| Tool | Scope | Limitation |
|---|---|---|
| OpenAPI | APIs | Does not model lifecycle state, queues, leases, logs, metrics, or workflows |
| Protobuf | RPC schemas | Does not model lifecycle or orchestration semantics |
| Terraform | Infrastructure | Does not model runtime behavior |
| BPMN | Business workflows | Not usually a system implementation specification |
| UML | Diagrams | Not usually executable or canonical |
| Prose docs | Everything | Ambiguous, inconsistent, difficult to validate |
| StateSpec | System behavior | Structured, canonical, generator-friendly |

---

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md).

---

## License

See [`LICENSE.md`](LICENSE.md).

---

## Philosophy

> Systems fail when design is implicit.

StateSpec makes design explicit, structured, reviewable, and executable.

---

## One-Line Summary

> From state machines to backend-neutral distributed systems.
