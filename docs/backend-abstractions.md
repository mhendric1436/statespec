# Backend Abstraction Source Artifacts

StateSpec defines an OCC-centered backend abstraction model so generated runtimes can
share one consistency doctrine across multiple concrete storage engines.

The repository includes reference interface definitions in several implementation
languages. Runtime interfaces are decomposed by component so users can depend only on
leases, queues, workflows, or any subset their system requires.

| Language | Backend path | Lease path | Queue path | Workflow path | Intended use |
|---|---|---|---|---|---|
| C++20 | [`bindings/cpp/backend.hpp`](../bindings/cpp/backend.hpp) | [`bindings/cpp/lease.hpp`](../bindings/cpp/lease.hpp) | [`bindings/cpp/queue.hpp`](../bindings/cpp/queue.hpp) | [`bindings/cpp/workflow.hpp`](../bindings/cpp/workflow.hpp) | C++ runtime/backend adapter reference. |
| Rust | [`bindings/rust/backend.rs`](../bindings/rust/backend.rs) | [`bindings/rust/lease.rs`](../bindings/rust/lease.rs) | [`bindings/rust/queue.rs`](../bindings/rust/queue.rs) | [`bindings/rust/workflow.rs`](../bindings/rust/workflow.rs) | Rust runtime/backend adapter reference. |
| Go | [`bindings/go/backend/backend.go`](../bindings/go/backend/backend.go) | [`bindings/go/backend/lease.go`](../bindings/go/backend/lease.go) | [`bindings/go/backend/queue.go`](../bindings/go/backend/queue.go) | [`bindings/go/backend/workflow.go`](../bindings/go/backend/workflow.go) | Go API, worker, and backend adapter reference. |
| Java | [`bindings/java/com/statespec/backend/BackendModel.java`](../bindings/java/com/statespec/backend/BackendModel.java) | [`bindings/java/com/statespec/backend/Lease.java`](../bindings/java/com/statespec/backend/Lease.java) | [`bindings/java/com/statespec/backend/Queue.java`](../bindings/java/com/statespec/backend/Queue.java) | [`bindings/java/com/statespec/backend/Workflow.java`](../bindings/java/com/statespec/backend/Workflow.java) | JVM service/backend adapter reference. |

## Shared Backend Model

Each language defines the same conceptual backend pieces in the backend file:

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

The backend file intentionally stays component-neutral. Lease, queue, and workflow record
shapes live with their respective runtime interface files.

## Shared Runtime Model

Each language also defines runtime-facing interfaces for higher-level primitives in
separate files:

```text
Lease
Queue
Workflow
```

The runtime interfaces expose request, result, and record types for:

```text
lease acquire / renew / release / inspect
queue enqueue / claim / acknowledge / fail / inspect
workflow register definition / inspect definition
workflow start / claim steps / complete step / fail step / cancel / inspect
```

Workflow definitions are registered before executions are started. A
`WorkflowDefinition` includes the workflow name, version, start step, expected execution
time, singleton setting, step definitions, and metadata. Registration uses
`RegisterWorkflowDefinitionRequest` and returns `WorkflowDefinitionRegistration` so a
runtime can report whether the definition was created or replaced.

Workflow records and requests that identify a workflow definition carry both
`workflow_name` and `workflow_version`. The pair `(workflow_name, workflow_version)` is
the unique workflow-definition identity, allowing multiple versions of the same workflow
name to coexist.

Workflow step claim requests also carry `workflow_execution_id` because claiming work is
scoped to a particular execution of a workflow definition.

## Required Semantics

All backend implementations should preserve these semantics:

```text
begin transaction
read versioned records
query records
stage writes
stage deletes
commit atomically
reject stale commits with semantic conflicts
report backend capabilities
```

## OCC Contract

Generated runtimes should rely on the abstraction instead of concrete storage mechanics.
A backend adapter may use version columns, MVCC, compare-and-swap, conditional writes,
transactional key ranges, or in-memory version counters, but it must expose the same
logical behavior:

```text
read versioned state
validate predicates and invariants
prepare deterministic changes
commit only if observed versions are still current
retry safely on conflict
```

## Runtime Use

The higher-level runtimes should compose on this model:

- `mt` uses the entity store contract.
- `dl` implements `Lease` with conditional OCC updates.
- `qu` implements `Queue` with claimable OCC message records.
- `wf` implements `Workflow` with registered definitions and claimable OCC execution and step records.

This keeps StateSpec backend-neutral while still making concrete implementation targets
straightforward to build.
