# StateSpec Java Binding

This directory contains the Java reference binding for the StateSpec backend abstraction
and runtime component model.

## Files

| File | Purpose |
|---|---|
| [`com/statespec/backend/Backend.java`](com/statespec/backend/Backend.java) | Core backend, transaction, collection, query, capability, and conflict interfaces. |
| [`com/statespec/backend/Lease.java`](com/statespec/backend/Lease.java) | Lease records and lease runtime API. |
| [`com/statespec/backend/Queue.java`](com/statespec/backend/Queue.java) | Queue definitions, message records, and queue runtime API. |
| [`com/statespec/backend/Workflow.java`](com/statespec/backend/Workflow.java) | Workflow definitions, execution records, and workflow runtime API. |

## Package

All Java declarations live in package:

```java
package com.statespec.backend;
```

## Backend Interface

The core backend interface is:

```java
Backend
```

It provides:

```java
capabilities()
ensureCollection(descriptor)
ensureCollections(descriptors)
begin()
get(tx, collection, key)
query(tx, collection, query)
put(tx, collection, key, documentJson)
erase(tx, collection, key)
commit(tx)
```

`ensureCollections` provisions a full set of `CollectionDescriptor` records in one
backend API call.

## Index-Aware Queries

Java defines index-aware query model records in
[`Backend.java`](com/statespec/backend/Backend.java):

```java
Backend.IndexValue
Backend.IndexBound
Backend.Query.IndexEquals
Backend.Query.IndexPrefix
Backend.Query.IndexRange
```

`IndexValue` supports:

```java
NullValue
StringValue
IntegerValue
DecimalValue
BooleanValue
TimestampValue
```

Index values must be ordered according to the target `IndexDescriptor.fields` order.

## Transaction Interface

Transactions implement:

```java
Backend.Transaction
```

```java
isOpen()
abort()
```

The backend owns commit behavior through:

```java
backend.commit(tx)
```

## Runtime Components

Java uses direct component interface names:

```java
Lease
Queue
Workflow
```

Each runtime component supports two method styles.

### Backend-managed transaction methods

These methods take a `Backend` and are expected to manage transaction lifecycle
internally.

```java
operation(backend, request)
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

```java
operationTx(tx, request)
```

Use these when composing multiple entity, lease, queue, or workflow operations into one
transaction.

## Lease API

Defined in [`Lease.java`](com/statespec/backend/Lease.java):

```java
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

Defined in [`Queue.java`](com/statespec/backend/Queue.java):

```java
create(...)
createTx(...)
inspectDefinition(...)
inspectDefinitionTx(...)
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

Defined in [`Workflow.java`](com/statespec/backend/Workflow.java):

```java
registerDefinition(...)
registerDefinitionTx(...)
inspectDefinition(...)
inspectDefinitionTx(...)
start(...)
startTx(...)
claimSteps(...)
claimStepsTx(...)
completeStep(...)
completeStepTx(...)
failStep(...)
failStepTx(...)
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
