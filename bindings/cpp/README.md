# StateSpec C++ Binding

This directory contains the C++ reference binding for the StateSpec backend abstraction
and runtime component model.

## Files

| File | Purpose |
|---|---|
| [`backend.hpp`](backend.hpp) | Core backend, transaction, collection, query, capability, and conflict interfaces. |
| [`external_system.hpp`](external_system.hpp) | External system metadata lookup requests, resolution records, required-field inspection, and runtime resolver API. |
| [`feature_flag.hpp`](feature_flag.hpp) | Feature flag types, descriptors, evaluation requests, and runtime API. |
| [`json.hpp`](json.hpp) | Typed JSON value model, parser, canonical serializer, and JSON utility helpers. |
| [`lease.hpp`](lease.hpp) | Lease records and lease runtime API. |
| [`queue.hpp`](queue.hpp) | Queue definitions, message records, and queue runtime API. |
| [`workflow.hpp`](workflow.hpp) | Workflow definitions, execution records, and workflow runtime API. |

## Typed JSON Model

The C++ binding does not use raw `std::string` values for JSON documents.

All backend APIs use the typed:

```cpp
statespec::backend::Json
```

Defined in:

```cpp
json.hpp
```

The JSON implementation provides:

```cpp
Json::parse(...)
Json::object(...)
Json::array(...)
Json::null()
canonical_string()
contains(...)
find(...)
at(...)
```

Example:

```cpp
using namespace statespec::backend;

auto document = Json::object(
    {
        {"id", "order-123"},
        {"active", true},
        {"retry_count", std::int64_t{3}},
        {"tags", Json::array({"a", "b"})},
    }
);

backend.put(tx, "orders", "order-123", document);
```

JSON parsing is intentionally strict.

The parser rejects:

```text
duplicate object members
leading-zero numbers
trailing commas
invalid unicode surrogate pairs
unescaped control characters
non-finite numbers
```

Object members are stored in canonical sorted order using:

```cpp
std::map<std::string, Json>
```

Canonical serialization is deterministic and locale-independent.

```cpp
auto canonical = document.canonical_string();
```

This enables stable hashing, equality comparison, snapshotting, and backend-independent
serialization behavior.

## Backend Interface

The backend interface is `statespec::backend::IBackend`.

It provides:

```cpp
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

`ensure_collections` provisions a full set of `CollectionDescriptor` records in one
backend API call.

## Index-Aware Queries

C++ defines index-aware query model types in [`backend.hpp`](backend.hpp):

```cpp
IndexValue
IndexBound
Query::Kind::IndexEquals
Query::Kind::IndexPrefix
Query::Kind::IndexRange
```

Factory helpers are available:

```cpp
Query::index_equals(index_name, values)
Query::index_prefix(index_name, values)
Query::index_range(index_name, lower_bound, upper_bound)
```

`IndexValue` supports:

```cpp
null_value()
string_value(...)
integer_value(...)
decimal_value(...)
boolean_value(...)
timestamp_value(...)
```

Index values are strongly typed and internally represented as `Json` values.

Index values must be ordered according to the target `IndexDescriptor.fields` order.

## Transaction Interface

Transactions implement `statespec::backend::ITransaction`.

```cpp
is_open()
abort()
```

The backend owns commit behavior through:

```cpp
backend.commit(tx)
```

## Runtime Components

C++ uses `I*Store` interface names:

```cpp
ILeaseStore
IQueueStore
IWorkflowStore
IFeatureFlagStore
IExternalSystemMetadataResolver
```

Each runtime component supports two method styles.

### Backend-managed transaction methods

These methods take an `IBackend&` and are expected to manage transaction lifecycle
internally.

```cpp
operation(IBackend& backend, const Request& request)
```

Expected behavior:

```text
begin transaction
perform component operation
commit on success
abort on failure
```

### Caller-managed transaction methods

These methods use the `Tx` suffix and take an existing `ITransaction&`.

```cpp
operationTx(ITransaction& tx, const Request& request)
```

Use these when composing multiple entity, lease, queue, workflow, or feature flag
operations into one transaction.

## Feature Flag API

Defined in [`feature_flag.hpp`](feature_flag.hpp):

```cpp
register_definition(...)
register_definitionTx(...)
inspect_definition(...)
inspect_definitionTx(...)
evaluate(...)
evaluateTx(...)
```

Feature flag evaluation is transaction-aware. Use `evaluateTx(...)` when a workflow,
queue, lease, or entity update must observe flags in the same optimistic-concurrency
transaction.

## Lease API

Defined in [`lease.hpp`](lease.hpp):

```cpp
acquire(...)
acquireTx(...)
renew(...)
renewTx(...)
release(...)
releaseTx(...)
inspect(...)
inspectTx(...)
```

Leases provide exclusive ownership, expiry, and fencing-token semantics.

## Queue API

Defined in [`queue.hpp`](queue.hpp):

```cpp
register_definition(...)
register_definitionTx(...)
inspect_definition(...)
inspect_definitionTx(...)
enqueue(...)
enqueueTx(...)
claim(...)
claimTx(...)
acknowledge(...)
acknowledgeTx(...)
fail(...)
failTx(...)
inspect(...)
inspectTx(...)
```

Queue identity is:

```text
(queue, channel)
```

Queue creation is idempotent. Re-creating an identical definition may return
`created = false`.

## Workflow API

Defined in [`workflow.hpp`](workflow.hpp):

```cpp
register_definition(...)
register_definitionTx(...)
inspect_definition(...)
inspect_definitionTx(...)
start(...)
startTx(...)
claim_steps(...)
claim_stepsTx(...)
keep_alive_step(...)
keep_alive_stepTx(...)
complete_step(...)
complete_stepTx(...)
fail_step(...)
fail_stepTx(...)
cancel(...)
cancelTx(...)
inspect(...)
inspectTx(...)
```

Workflow definition identity is:

```text
(workflow_name, workflow_version)
```

Workflow definitions are immutable. A changed definition must be registered as a new
version.

## Namespaces

All C++ declarations are under:

```cpp
statespec::backend
```
