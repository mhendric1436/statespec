# Backend Abstraction Source Artifacts

StateSpec defines an OCC-centered backend abstraction model so generated runtimes can
share one consistency doctrine across multiple concrete storage engines.

The repository includes reference interface definitions in several implementation
languages.

| Language | Path | Intended use |
|---|---|---|
| C++20 | [`bindings/cpp/backend.hpp`](../bindings/cpp/backend.hpp) | C++ runtime/backend adapter reference. |
| Rust | [`bindings/rust/backend.rs`](../bindings/rust/backend.rs) | Rust runtime/backend adapter reference. |
| Go | [`bindings/go/backend/backend.go`](../bindings/go/backend/backend.go) | Go API, worker, and backend adapter reference. |
| Java | [`bindings/java/com/statespec/backend/BackendModel.java`](../bindings/java/com/statespec/backend/BackendModel.java) | JVM service/backend adapter reference. |

## Shared Model

Each language defines the same conceptual pieces:

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
- `dl` models leases as conditional OCC updates.
- `qu` models messages as claimable OCC records.
- `wf` models workflow executions and steps as claimable OCC records.

This keeps StateSpec backend-neutral while still making concrete implementation targets
straightforward to build.
