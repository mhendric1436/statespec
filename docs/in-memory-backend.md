# In-Memory Backend Contract

The generated in-memory backend is a common-tier runtime adapter for local development,
generated app linking, and tests. It is not a production storage backend.

The in-memory backend exists so generated API apps and Worker apps can be compiled and
exercised without a concrete database, queue service, lease service, workflow service,
log backend, or metrics backend. C++, Go, Java, and Rust emit the in-memory backend now.

Use it as a generated dependency for local app linking and tests. Do not expose it as a
transport-facing API or treat it as a production persistence model.

## Scope

The in-memory backend package covers these binding interfaces in each language:

- `IBackend` / `Backend`
- `ITransaction` / transaction type
- `IFeatureFlagStore`
- `IQueueStore`
- `ILeaseStore`
- `IWorkflowStore`
- `ILogSink`
- `IMetricSink`

The backend and transaction pieces must remain generic. Feature flag, queue, lease,
workflow, log, and metric implementations are separate store/sink clients that use the
backend by registering collections and reading or writing versioned records.

The memory package should be generated under the `common` tier so both API and Worker
tiers can reuse the same implementation.

## Directory Layout

C++:

```text
common/memory/backend.hpp
common/memory/transaction.hpp
common/memory/codec.hpp
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
common/backend/memory/codec.go
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
common/com/statespec/backend/memory/InMemoryCodec.java
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
common/memory/codec.rs
common/memory/feature_flags.rs
common/memory/queues.rs
common/memory/leases.rs
common/memory/workflows.rs
common/memory/logs.rs
common/memory/metrics.rs
```

These paths are part of the cross-language artifact model. C++, Go, Java, and Rust currently
emit the listed files.

## Generated App Linking

The generated API and Worker tiers both depend on the generated common-tier backend
contracts. The in-memory backend is generated in the same common tier so one backend
instance can satisfy both app shells in a local process or test fixture.

Recommended local composition:

```text
Application composition root
  InMemoryBackend
  API app shell
    API handlers
    operator metadata API handlers
  Worker app shell
    worker handlers
    workflow step handlers
    workflow runner
```

Use the same `InMemoryBackend` instance when the API app writes state that the Worker app
must later observe. Use separate instances when tests need isolated state.

The generated in-memory backend is appropriate for:

- compiling generated API app surfaces
- compiling generated Worker app surfaces
- local end-to-end tests where API and Worker code run in one process
- deterministic tests of handlers, workflow steps, descriptors, and catalog registration

It is not appropriate for:

- cross-process coordination
- durable storage
- production queue visibility, lease, or workflow scheduling behavior
- proving a production backend adapter implements the same operational guarantees

Repository-level generated app linking coverage is exercised by:

```sh
make test-generated-apps
```

## Shared State Model

Each in-memory backend instance should own one shared generic state object:

```text
InMemoryBackend
  SharedState
    collections
    records
    versions
```

API and Worker app shells should receive the same backend instance when they need to
observe the same local runtime state.

The backend state is intentionally process-local. Tests that need a clean environment
should construct a new backend instance instead of clearing generated internals directly.

Runtime stores and sinks may use the shared backend to persist their records, but the
backend state object itself must not have native queue, lease, workflow, feature flag,
log, or metric maps. Those records are stored in component-owned collections.

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

Transactions stage generic record puts and deletes only. They should not expose
domain-specific staging maps or append buffers for queues, leases, workflows, feature
flags, logs, or metrics.

## Store Contracts

Feature flag store:

- Register feature flag definitions.
- Persist and evaluate overrides.
- Return descriptor defaults when no override exists.
- Support transaction-scoped evaluation.
- Store definitions and overrides in feature-flag-owned backend collections.

Queue store:

- Register queue definitions.
- Enqueue messages transactionally.
- Claim visible messages.
- Ack and fail messages.
- Track attempts, visibility timeout, and dead-letter metadata where supported.
- Store definitions, messages, and idempotency keys in queue-owned backend collections.

Lease store:

- Register lease definitions.
- Acquire, renew, and release leases.
- Enforce holder identity and fencing token behavior.
- Use an injectable or overridable clock for TTL tests.
- Store definitions and lease ownership records in lease-owned backend collections.

Workflow store:

- Register workflow definitions.
- Start workflow executions.
- Claim workflow steps.
- Keep claimed steps alive.
- Complete or fail steps.
- Preserve retry-visible execution and step state.
- Store definitions, executions, and claim state in workflow-owned backend collections.

Log sink:

- Register log definitions.
- Emit structured log events transactionally.
- Provide inspect APIs for tests.
- Store definitions and events in log-owned backend collections.

Metric sink:

- Register metric definitions.
- Record metric samples transactionally.
- Provide inspect APIs for tests.
- Store definitions and samples in metric-owned backend collections.

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

## Related Docs

- [quick-start.md](quick-start.md) shows the CLI commands for generating and checking
  app fixtures that link against the in-memory backend.
- [generated-extension-points.md](generated-extension-points.md) describes the
  user-owned handlers and composition roots that receive backend dependencies.
- [backend-abstractions.md](backend-abstractions.md) describes the generated OCC backend
  and transaction interfaces that production and in-memory adapters implement.
