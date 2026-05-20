# In-Memory Backend Contract

The generated in-memory backend is a common-tier runtime adapter for local development,
generated app linking, and tests. It is not a production storage backend.

The in-memory backend exists so generated API apps and Worker apps can be compiled and
exercised without a concrete database, queue service, lease service, workflow service,
log backend, or metrics backend. C++, Go, and Java emit the in-memory backend now; Rust
paths are part of the shared contract and are planned for follow-up implementation.

## Scope

The in-memory backend contract covers these binding interfaces in each language:

- `IBackend` / `Backend`
- `ITransaction` / transaction type
- `IFeatureFlagStore`
- `IQueueStore`
- `ILeaseStore`
- `IWorkflowStore`
- `ILogSink`
- `IMetricSink`

The backend should be generated under the `common` tier so both API and Worker tiers can
reuse the same implementation.

## Directory Layout

C++:

```text
common/memory/backend.hpp
common/memory/transaction.hpp
common/memory/feature_flag_store.hpp
common/memory/queue_store.hpp
common/memory/lease_store.hpp
common/memory/workflow_store.hpp
common/memory/log_sink.hpp
common/memory/metric_sink.hpp
```

Go:

```text
common/backend/memory/backend.go
common/backend/memory/transaction.go
common/backend/memory/feature_flags.go
common/backend/memory/queues.go
common/backend/memory/leases.go
common/backend/memory/workflows.go
common/backend/memory/logs.go
common/backend/memory/metrics.go
```

Java:

```text
common/com/statespec/backend/memory/InMemoryBackend.java
common/com/statespec/backend/memory/InMemoryTransaction.java
common/com/statespec/backend/memory/InMemoryFeatureFlagStore.java
common/com/statespec/backend/memory/InMemoryQueueStore.java
common/com/statespec/backend/memory/InMemoryLeaseStore.java
common/com/statespec/backend/memory/InMemoryWorkflowStore.java
common/com/statespec/backend/memory/InMemoryLogSink.java
common/com/statespec/backend/memory/InMemoryMetricSink.java
```

Rust:

```text
common/memory/backend.rs
common/memory/transaction.rs
common/memory/feature_flags.rs
common/memory/queues.rs
common/memory/leases.rs
common/memory/workflows.rs
common/memory/logs.rs
common/memory/metrics.rs
```

These paths are part of the cross-language artifact model. C++, Go, and Java currently emit the
listed files; Rust is still contract-only.

## Shared State Model

Each in-memory backend instance should own one shared state object:

```text
InMemoryBackend
  SharedState
    documents
    feature_flag_definitions
    feature_flag_values
    queue_definitions
    queue_messages
    lease_definitions
    leases
    workflow_definitions
    workflow_executions
    workflow_steps
    log_definitions
    log_events
    metric_definitions
    metric_samples
    versions
```

API and Worker app shells should receive the same backend instance when they need to
observe the same local runtime state.

## OCC Semantics

The in-memory backend must preserve the optimistic-concurrency shape of the production
backend interfaces:

1. `begin` opens an in-memory transaction.
2. Reads capture the current record version in the transaction read set.
3. Writes are staged in the transaction write set.
4. `commit` verifies that read and write versions still match shared state.
5. `commit` applies staged writes and increments written record versions.
6. `abort` or dropped transactions discard staged writes.

Backend-managed methods may open and commit their own transaction. Caller-managed `Tx`
methods must compose with an existing transaction and must not commit or abort it.

## Store Contracts

Feature flag store:

- Register feature flag definitions.
- Persist and evaluate overrides.
- Return descriptor defaults when no override exists.
- Support transaction-scoped evaluation.

Queue store:

- Register queue definitions.
- Enqueue messages transactionally.
- Claim visible messages.
- Ack and fail messages.
- Track attempts, visibility timeout, and dead-letter metadata where supported.

Lease store:

- Register lease definitions.
- Acquire, renew, and release leases.
- Enforce holder identity and fencing token behavior.
- Use an injectable or overridable clock for TTL tests.

Workflow store:

- Register workflow definitions.
- Start workflow executions.
- Claim workflow steps.
- Keep claimed steps alive.
- Complete or fail steps.
- Preserve retry-visible execution and step state.

Log sink:

- Register log definitions.
- Emit structured log events transactionally.
- Provide inspect APIs for tests.

Metric sink:

- Register metric definitions.
- Record metric samples transactionally.
- Provide inspect APIs for tests.

## Runtime Boundary

The in-memory backend is a runtime adapter. It must not reinterpret StateSpec semantics.
Grammar, parser, validator, IR, and generators remain the semantic authority.

The generated in-memory backend should be sufficient for:

- generated API app linking
- generated Worker app linking
- local smoke tests
- examples
- deterministic unit tests

It should not be used as evidence that a production backend adapter is correct.
