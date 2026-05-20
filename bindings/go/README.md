# StateSpec Go Binding

This directory contains the Go reference binding for the StateSpec backend abstraction
and runtime component model.

## Files

| File | Purpose |
|---|---|
| [`backend/backend.go`](backend/backend.go) | Core backend, transaction, collection, query, capability, and conflict interfaces. |
| [`backend/external_system.go`](backend/external_system.go) | External system metadata lookup requests, resolution records, required-field inspection, and runtime resolver API. |
| [`backend/feature_flag.go`](backend/feature_flag.go) | Feature flag types, descriptors, evaluation requests, and runtime API. |
| [`backend/json.go`](backend/json.go) | Typed JSON value model, parser, canonical serializer, and JSON utility helpers. |
| [`backend/lease.go`](backend/lease.go) | Lease records and lease runtime API. |
| [`backend/queue.go`](backend/queue.go) | Queue definitions, message records, and queue runtime API. |
| [`backend/workflow.go`](backend/workflow.go) | Workflow definitions, execution records, and workflow runtime API. |

## Package

All Go declarations live in package:

```go
package backend
```

## Typed JSON Model

The Go binding uses `backend.JSON` for backend documents, JSON equality predicates, and index values instead of raw `[]byte` payloads.

The typed JSON implementation lives in [`backend/json.go`](backend/json.go).

Useful constructors and helpers:

```go
ParseJSON(...)
JSONNull()
JSONBool(...)
JSONInt(...)
JSONFloat(...)
JSONString(...)
JSONArray(...)
JSONObject(...)
CanonicalString()
Find(...)
AsBool()
AsInt()
AsFloat()
AsString()
AsArray()
AsObject()
```

Example:

```go
document := backend.JSONObject(map[string]backend.JSON{
    "id": backend.JSONString("order-123"),
    "active": backend.JSONBool(true),
    "retry_count": backend.JSONInt(3),
})
```

`ParseJSON` rejects duplicate object members, trailing content, invalid numbers, and non-finite floating point values. `CanonicalString` serializes objects with sorted keys so storage, hashing, snapshotting, and logs can use deterministic JSON text.

`JSON` implements Go JSON marshaling through `MarshalJSON` and `UnmarshalJSON`.

## Backend Interface

The core interface is `Backend`.

It provides:

```go
Capabilities(ctx)
EnsureCollection(ctx, descriptor)
EnsureCollections(ctx, descriptors)
Begin(ctx)
Get(ctx, tx, collection, key)
Query(ctx, tx, collection, query)
Put(ctx, tx, collection, key, document)
Erase(ctx, tx, collection, key)
Commit(ctx, tx)
```

`EnsureCollections` provisions a full set of `CollectionDescriptor` records in one backend API call.

## Index-Aware Queries

Go defines index-aware query model types in [`backend/backend.go`](backend/backend.go):

```go
IndexValueKind
IndexValue
IndexBound
QueryIndexEquals
QueryIndexPrefix
QueryIndexRange
```

Factory helpers are available:

```go
IndexEqualsQuery(indexName, values)
IndexPrefixQuery(indexName, prefixValues)
IndexRangeQuery(indexName, lowerBound, upperBound)
```

`IndexValueKind` supports:

```go
IndexNull
IndexString
IndexInteger
IndexDecimal
IndexBoolean
IndexTimestamp
```

Typed index value helpers are available:

```go
NullIndexValue()
StringIndexValue(...)
IntegerIndexValue(...)
DecimalIndexValue(...)
BooleanIndexValue(...)
TimestampIndexValue(...)
```

Index values are strongly typed and internally represented as `JSON` values. Index values must be ordered according to the target `IndexDescriptor.Fields` order.

## Transaction Interface

Transactions implement `Transaction`.

```go
IsOpen()
Abort(ctx)
```

The backend owns commit behavior through:

```go
backend.Commit(ctx, tx)
```

## Runtime Components

Go uses `*Store` interface names:

```go
LeaseStore
QueueStore
WorkflowStore
FeatureFlagStore
ExternalSystemMetadataResolver
```

Each runtime component supports two method styles.

### Backend-managed transaction methods

These methods take a `Backend` and are expected to manage transaction lifecycle internally.

```go
Operation(ctx, backend, request)
```

Expected behavior:

```text
begin transaction
perform component operation
commit on success
abort on failure
```

### Caller-managed transaction methods

These methods use the `Tx` suffix and take an existing `Transaction`.

```go
OperationTx(ctx, tx, request)
```

Use these when composing multiple entity, lease, queue, workflow, or feature flag operations into one transaction.

## Feature Flag API

Defined in [`backend/feature_flag.go`](backend/feature_flag.go):

```go
RegisterDefinition(...)
RegisterDefinitionTx(...)
InspectDefinition(...)
InspectDefinitionTx(...)
Evaluate(...)
EvaluateTx(...)
```

Feature flag evaluation is transaction-aware. Use `EvaluateTx(...)` when a workflow, queue, lease, or entity update must observe flags in the same optimistic-concurrency transaction.

## Lease API

Defined in [`backend/lease.go`](backend/lease.go):

```go
Acquire(...)
AcquireTx(...)
Renew(...)
RenewTx(...)
Release(...)
ReleaseTx(...)
Inspect(...)
InspectTx(...)
```

Leases provide exclusive ownership, expiry, and fencing-token semantics.

## Queue API

Defined in [`backend/queue.go`](backend/queue.go):

```go
RegisterDefinition(...)
RegisterDefinitionTx(...)
InspectDefinition(...)
InspectDefinitionTx(...)
Enqueue(...)
EnqueueTx(...)
Claim(...)
ClaimTx(...)
Acknowledge(...)
AcknowledgeTx(...)
Fail(...)
FailTx(...)
Inspect(...)
InspectTx(...)
```

Queue identity is:

```text
(queue, channel)
```

Queue creation is idempotent. Re-creating an identical definition may return `Created = false`.

## Workflow API

Defined in [`backend/workflow.go`](backend/workflow.go):

```go
RegisterDefinition(...)
RegisterDefinitionTx(...)
InspectDefinition(...)
InspectDefinitionTx(...)
Start(...)
StartTx(...)
ClaimSteps(...)
ClaimStepsTx(...)
KeepAliveStep(...)
KeepAliveStepTx(...)
CompleteStep(...)
CompleteStepTx(...)
FailStep(...)
FailStepTx(...)
Cancel(...)
CancelTx(...)
Inspect(...)
InspectTx(...)
```

Workflow definition identity is:

```text
(workflow_name, workflow_version)
```

Workflow definitions are immutable. A changed definition must be registered as a new version.
