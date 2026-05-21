# Backend-Neutral Runtime Store Contract

Feature flags, queues, leases, workflows, logs, and metrics are typed runtime clients
of the StateSpec OCC backend model. They are not in-memory backend features.

This contract applies to generated and hand-written binding implementations for C++,
Go, Java, and Rust.

## Boundary

Runtime stores and sinks must depend on the public backend and transaction interfaces:

```text
FeatureFlagStore
QueueStore
LeaseStore
WorkflowStore
LogSink
MetricSink
    use
Backend / IBackend
Transaction / ITransaction
    implemented by
InMemoryBackend, PostgresBackend, or another concrete adapter
```

The concrete backend owns generic optimistic-concurrency mechanics only:

- collection registration
- versioned records
- queries
- read-set tracking
- staged puts and deletes
- commit and abort
- backend capabilities and conflict reporting

The typed runtime store owns domain behavior:

- collection names
- key construction
- record encoding and decoding
- registration, inspect, evaluate, enqueue, claim, acquire, renew, start, complete, fail, emit, and record operations
- domain-specific conflict classification

## Dependency Rule

Typed runtime stores must not depend on memory-specific backend or transaction classes.

Avoid signatures like:

```text
Store(InMemoryBackend&)
StoreTx(InMemoryTransaction&)
impl Store<InMemoryBackend>
```

Prefer signatures that accept the backend abstraction:

```text
Store(IBackend&)
StoreTx(ITransaction&)
Store<B: Backend>
```

Language-specific generic constraints may vary, but the dependency direction must not:
typed stores depend on backend abstractions, and concrete backends implement those
abstractions.

## Transaction Rule

Every persisted runtime operation should have both forms:

- a backend-managed method that begins, commits, and aborts its own transaction
- a caller-managed `Tx` method that participates in an existing transaction

Caller-managed methods must not begin, commit, or abort the transaction they receive.
They should only read, query, put, or erase records through the transaction model.

## Collection Ownership

Runtime stores own their collections. For example:

- feature flags own definition and override collections
- queues own definition, message, and idempotency collections
- leases own definition and lease-record collections
- workflows own definition and execution collections
- logs own definition and event collections
- metrics own definition and sample collections

Backends should not expose native queue maps, lease maps, workflow maps, feature flag
maps, log buffers, or metric buffers. Those records are normal versioned documents in
component-owned collections.

## Generated Layout Direction

The long-term generated layout should separate backend-neutral runtime stores from
concrete backend adapters:

```text
common/runtime/
  feature_flags.*
  queues.*
  leases.*
  workflows.*
  logs.*
  metrics.*

common/memory/
  backend.*
  transaction.*
  storage/codec helpers as needed
```

Some current generated artifacts may still live under memory-specific directories while
the refactor is in progress. Even in those transitional paths, runtime store code should
use only the public backend and transaction abstractions.

## Test Expectations

Each language should have tests proving that:

- the in-memory backend and transaction contain no typed runtime state
- runtime stores can operate through backend abstractions
- transaction-scoped methods compose multiple runtime operations atomically
- generated API and Worker app fixtures can link stores with the in-memory backend

This contract keeps StateSpec runtime semantics portable across in-memory, PostgreSQL,
or future backend adapters without reimplementing queues, leases, workflows, feature
flags, logs, or metrics per backend.
