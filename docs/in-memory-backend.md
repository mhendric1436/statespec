# In-Memory Backend Contract

The generated in-memory backend is a common-tier runtime adapter for local development,
generated app linking, and tests. It is not a production storage backend.

The in-memory backend exists so generated API apps and Worker apps can be compiled and
exercised without a concrete database, queue service, lease service, workflow service,
log backend, or metrics backend. C++, Go, Java, and Rust emit the in-memory backend now.

Use it as a generated dependency for local app linking and tests. Do not expose it as a
transport-facing API or treat it as a production persistence model.

## Scope

The generated common tier currently emits two kinds of runtime support in each language:

1. Concrete in-memory backend adapters:
   - `IBackend` / `Backend`
   - `ITransaction` / transaction type
2. Backend-neutral runtime clients:
   - `IFeatureFlagStore`
   - `IQueueStore`
   - `ILeaseStore`
   - `IWorkflowStore`
   - `ILogSink`
   - `IMetricSink`

The backend and transaction pieces must remain generic. Feature flag, queue, lease,
workflow, log, and metric implementations are separate store/sink clients that use the
backend by registering collections and reading or writing versioned records.

Those store/sink clients are backend-neutral runtime components. They should depend on
the public `Backend`/`IBackend` and `Transaction`/`ITransaction` abstractions, not on
`InMemoryBackend`, `InMemoryTransaction`, or other memory-specific concrete types.

The `memory/` package or directory should contain only the concrete in-memory backend
adapter and transaction. The `runtime/` package or directory should contain codecs,
stores, and sinks that are reusable with any backend adapter implementing the OCC
interfaces. Both are generated under the `common` tier so API and Worker tiers can reuse
the same contracts.

## Directory Layout

C++:

```text
common/memory/backend.hpp
common/memory/transaction.hpp
common/runtime/codec.hpp
common/runtime/feature_flag_store.hpp
common/runtime/queue_store.hpp
common/runtime/lease_store.hpp
common/runtime/workflow_store.hpp
common/runtime/log_sink.hpp
common/runtime/metric_sink.hpp
```

Go:

```text
common/backend/memory/backend.go
common/backend/memory/transaction.go
common/backend/runtime/codec.go
common/backend/runtime/feature_flags.go
common/backend/runtime/queues.go
common/backend/runtime/leases.go
common/backend/runtime/workflows.go
common/backend/runtime/logs.go
common/backend/runtime/metrics.go
```

Java:

```text
common/com/statespec/backend/memory/InMemoryBackend.java
common/com/statespec/backend/memory/InMemoryTransaction.java
common/com/statespec/backend/runtime/RuntimeCodec.java
common/com/statespec/backend/runtime/RuntimeFeatureFlagStore.java
common/com/statespec/backend/runtime/RuntimeQueueStore.java
common/com/statespec/backend/runtime/RuntimeLeaseStore.java
common/com/statespec/backend/runtime/RuntimeWorkflowStore.java
common/com/statespec/backend/runtime/RuntimeLogSink.java
common/com/statespec/backend/runtime/RuntimeMetricSink.java
```

Rust:

```text
common/memory/backend.rs
common/memory/transaction.rs
common/runtime/codec.rs
common/runtime/feature_flags.rs
common/runtime/queues.rs
common/runtime/leases.rs
common/runtime/workflows.rs
common/runtime/logs.rs
common/runtime/metrics.rs
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

Repository-level cross-language runtime fixture coverage is exercised by:

```sh
make test
```

The generated binding fixtures for C++, Go, Java, and Rust should stay behaviorally
aligned. Each language fixture should prove that one in-memory backend instance can
compose the backend-neutral feature flag, queue, lease, workflow, log, and metric
clients, and should also exercise generic backend `put`/`query` primitives.

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
