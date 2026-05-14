# StateSpec Rust Binding

This directory contains the Rust reference binding for the StateSpec backend abstraction
and runtime component model.

## Files

| File | Purpose |
|---|---|
| [`backend.rs`](backend.rs) | Core backend, transaction, collection, query, capability, and conflict traits/types. |
| [`feature_flag.rs`](feature_flag.rs) | Feature flag types, descriptors, evaluation requests, and runtime API. |
| [`json.rs`](json.rs) | Typed JSON value model, parser, canonical serializer, and JSON utility helpers. |
| [`lease.rs`](lease.rs) | Lease records and lease runtime API. |
| [`queue.rs`](queue.rs) | Queue definitions, message records, and queue runtime API. |
| [`workflow.rs`](workflow.rs) | Workflow definitions, execution records, and workflow runtime API. |

## Typed JSON Model

The Rust binding uses `Json` for backend documents, JSON equality predicates, and index values instead of raw serialized JSON strings.

The typed JSON implementation provides:

```rust
Json::parse(...)
Json::canonical_string()
Json::find(...)
```

The `Json` enum supports:

```rust
Json::Null
Json::Bool(...)
Json::Integer(...)
Json::Decimal(...)
Json::String(...)
Json::Array(...)
Json::Object(...)
```

Example:

```rust
use std::collections::BTreeMap;

let mut object = BTreeMap::new();
object.insert("id".to_string(), Json::String("order-123".to_string()));
object.insert("active".to_string(), Json::Bool(true));
object.insert("retry_count".to_string(), Json::Integer(3));

let document = Json::Object(object);
```

`Json::parse` rejects malformed input such as duplicate object members, trailing commas, leading-zero numbers, invalid escapes, and unescaped control characters.

`canonical_string()` serializes object members in sorted key order using `BTreeMap` so backend storage, hashing, snapshotting, logging, and cross-binding comparisons can use deterministic JSON text.

## Backend Trait

The core trait is `Backend`.

It provides:

```rust
capabilities()
ensure_collection(...)
ensure_collections(...)
begin()
get(...)
query(...)
put(...)
erase(...)
commit(...)
```

`ensure_collections` provisions a full set of `CollectionDescriptor` records in one backend API call.

## Index-Aware Queries

Rust defines index-aware query model types in [`backend.rs`](backend.rs):

```rust
IndexValue
IndexBound
Query::IndexEquals
Query::IndexPrefix
Query::IndexRange
```

`IndexValue` supports:

```rust
Null
String(...)
Integer(...)
Decimal(...)
Boolean(...)
Timestamp(...)
```

Typed JSON conversion is available through:

```rust
IndexValue::json_value()
```

Index values must be ordered according to the target `IndexDescriptor.fields` order.

## Transaction Trait

Transactions implement `Transaction`.

```rust
is_open()
abort()
```

The backend owns commit behavior through:

```rust
backend.commit(tx)
```

## Runtime Components

Rust uses `*Store` trait names:

```rust
LeaseStore
QueueStore
WorkflowStore
FeatureFlagStore
```

Each runtime component supports two method styles.

### Backend-managed transaction methods

These methods take a `Backend` and are expected to manage transaction lifecycle internally.

```rust
operation(&backend, &request)
```

Expected behavior:

```text
begin transaction
perform component operation
commit on success
abort on failure
```

### Caller-managed transaction methods

These methods use the `_tx` suffix and take an existing transaction.

```rust
operation_tx(&mut tx, &request)
```

Use these when composing multiple entity, lease, queue, workflow, or feature flag operations into one transaction.

## Feature Flag API

Defined in [`feature_flag.rs`](feature_flag.rs):

```rust
register_definition(...)
register_definition_tx(...)
inspect_definition(...)
inspect_definition_tx(...)
evaluate(...)
evaluate_tx(...)
```

Feature flag evaluation is transaction-aware. Use `evaluate_tx(...)` when a workflow, queue, lease, or entity update must observe flags in the same optimistic-concurrency transaction.

## Lease API

Defined in [`lease.rs`](lease.rs):

```rust
acquire(...)
acquire_tx(...)
renew(...)
renew_tx(...)
release(...)
release_tx(...)
inspect(...)
inspect_tx(...)
```

Leases provide exclusive ownership, expiry, and fencing-token semantics.

## Queue API

Defined in [`queue.rs`](queue.rs):

```rust
create(...)
create_tx(...)
inspect_definition(...)
inspect_definition_tx(...)
enqueue(...)
enqueue_tx(...)
claim(...)
claim_tx(...)
acknowledge(...)
acknowledge_tx(...)
fail(...)
fail_tx(...)
inspect(...)
inspect_tx(...)
```

Queue identity is:

```text
(queue, channel)
```

Queue creation is idempotent. Re-creating an identical definition may return `created = false`.

## Workflow API

Defined in [`workflow.rs`](workflow.rs):

```rust
register_definition(...)
register_definition_tx(...)
inspect_definition(...)
inspect_definition_tx(...)
start(...)
start_tx(...)
claim_steps(...)
claim_steps_tx(...)
complete_step(...)
complete_step_tx(...)
fail_step(...)
fail_step_tx(...)
cancel(...)
cancel_tx(...)
inspect(...)
inspect_tx(...)
```

Workflow definition identity is:

```text
(workflow_name, workflow_version)
```

Workflow definitions are immutable. A changed definition must be registered as a new version.
