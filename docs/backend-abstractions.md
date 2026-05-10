# Backend Abstraction Source Artifacts

StateSpec defines an OCC-centered backend abstraction model so generated runtimes can
share one consistency doctrine across multiple concrete storage engines.

The repository includes reference interface definitions in several implementation
languages.

| Language | Backend path | Runtime store path | Intended use |
|---|---|---|---|
| C++20 | [`bindings/cpp/backend.hpp`](../bindings/cpp/backend.hpp) | [`bindings/cpp/runtime.hpp`](../bindings/cpp/runtime.hpp) | C++ runtime/backend adapter reference. |
| Rust | [`bindings/rust/backend.rs`](../bindings/rust/backend.rs) | [`bindings/rust/runtime.rs`](../bindings/rust/runtime.rs) | Rust runtime/backend adapter reference. |
| Go | [`bindings/go/backend/backend.go`](../bindings/go/backend/backend.go) | [`bindings/go/backend/runtime.go`](../bindings/go/backend/runtime.go) | Go API, worker, and backend adapter reference. |
| Java | [`bindings/java/com/statespec/backend/BackendModel.java`](../bindings/java/com/statespec/backend/BackendModel.java) | [`bindings/java/com/statespec/backend/RuntimeStores.java`](../bindings/java/com/statespec/backend/RuntimeStores.java) | JVM service/backend adapter reference. |

## Shared Backend Model

Each language defines the same conceptual backend pieces:

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
LeaseRecord
QueueMessageRecord
WorkflowExecutionRecord
```

## Shared Runtime Store Model

Each language also defines runtime-facing interfaces for higher-level primitives:

```text
LeaseStore
QueueStore
WorkflowStore
```

The runtime stores expose request and result types for:

```text
lease acquire / renew / release / inspect
queue enqueue / claim / acknowledge / fail / inspect
workflow start / claim steps / complete step / fail step / cancel / inspect
```

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
- `dl` implements `LeaseStore` with conditional OCC updates.
- `qu` implements `QueueStore` with claimable OCC message records.
- `wf` implements `WorkflowStore` with claimable OCC execution and step records.

This keeps StateSpec backend-neutral while still making concrete implementation targets
straightforward to build.
