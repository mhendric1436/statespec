# StateSpec Go Binding

This directory contains the Go reference binding for the StateSpec backend abstraction
and runtime component model.

## Files

| File | Purpose |
|---|---|
| [`backend/backend.go`](backend/backend.go) | Core backend, transaction, collection, query, capability, and conflict interfaces. |
| [`backend/lease.go`](backend/lease.go) | Lease records and lease runtime API. |
| [`backend/queue.go`](backend/queue.go) | Queue definitions, message records, and queue runtime API. |
| [`backend/workflow.go`](backend/workflow.go) | Workflow definitions, execution records, and workflow runtime API. |

## Package

All Go declarations live in package:

```go
package backend
```

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

`EnsureCollections` provisions a full set of `CollectionDescriptor` records in one
backend API call.

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
```

Each runtime component supports two method styles.

### Backend-managed transaction methods

These methods take a `Backend` and are expected to manage transaction lifecycle
internally.

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

Use these when composing multiple entity, lease, queue, or workflow operations into one
transaction.

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
Create(...)
CreateTx(...)
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

Queue creation is idempotent. Re-creating an identical definition may return
`Created = false`.

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

Workflow definitions are immutable. A changed definition must be registered as a new
version.
