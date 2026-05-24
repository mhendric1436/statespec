# Backend Abstraction Source Artifacts

StateSpec defines an OCC-centered backend abstraction model so generated runtimes can
share one consistency doctrine across multiple concrete storage engines.

The repository includes reference interface definitions in several implementation
languages. Runtime interfaces are decomposed by component so users can depend only on
leases, queues, workflows, or any subset their system requires.

| Language | Backend path | Lease path | Queue path | Workflow path | Log path | Metric path | Intended use |
|---|---|---|---|---|---|---|---|
| C++20 | [`bindings/cpp/backend.hpp`](../bindings/cpp/backend.hpp) | [`bindings/cpp/lease.hpp`](../bindings/cpp/lease.hpp) | [`bindings/cpp/queue.hpp`](../bindings/cpp/queue.hpp) | [`bindings/cpp/workflow.hpp`](../bindings/cpp/workflow.hpp) | [`bindings/cpp/log.hpp`](../bindings/cpp/log.hpp) | [`bindings/cpp/metric.hpp`](../bindings/cpp/metric.hpp) | C++ runtime/backend adapter reference. |
| Rust | [`bindings/rust/backend.rs`](../bindings/rust/backend.rs) | [`bindings/rust/lease.rs`](../bindings/rust/lease.rs) | [`bindings/rust/queue.rs`](../bindings/rust/queue.rs) | [`bindings/rust/workflow.rs`](../bindings/rust/workflow.rs) | [`bindings/rust/log.rs`](../bindings/rust/log.rs) | [`bindings/rust/metric.rs`](../bindings/rust/metric.rs) | Rust runtime/backend adapter reference. |
| Go | [`bindings/go/backend/backend.go`](../bindings/go/backend/backend.go) | [`bindings/go/backend/lease.go`](../bindings/go/backend/lease.go) | [`bindings/go/backend/queue.go`](../bindings/go/backend/queue.go) | [`bindings/go/backend/workflow.go`](../bindings/go/backend/workflow.go) | [`bindings/go/backend/log.go`](../bindings/go/backend/log.go) | [`bindings/go/backend/metric.go`](../bindings/go/backend/metric.go) | Go API, worker, and backend adapter reference. |
| Java | [`bindings/java/com/statespec/backend/Backend.java`](../bindings/java/com/statespec/backend/Backend.java) | [`bindings/java/com/statespec/backend/Lease.java`](../bindings/java/com/statespec/backend/Lease.java) | [`bindings/java/com/statespec/backend/Queue.java`](../bindings/java/com/statespec/backend/Queue.java) | [`bindings/java/com/statespec/backend/Workflow.java`](../bindings/java/com/statespec/backend/Workflow.java) | [`bindings/java/com/statespec/backend/Log.java`](../bindings/java/com/statespec/backend/Log.java) | [`bindings/java/com/statespec/backend/Metric.java`](../bindings/java/com/statespec/backend/Metric.java) | JVM service/backend adapter reference. |

## Shared Backend Model

Each language defines the same conceptual backend pieces in the backend file:

```text
CollectionDescriptor
FieldDescriptor
IndexDescriptor
BackendCapabilities
ConflictKind
VersionedRecord
IndexValue
IndexBound
Query
Transaction
Backend
```

The backend file intentionally stays component-neutral. Lease, queue, workflow, feature
flag, log, and metric record shapes live with their respective runtime interface files.
Backend and transaction implementations must not carry domain-specific state for those
runtime components.

Generated common output always includes the backend and transaction contracts because
they are generic OCC document primitives. Typed runtime clients are usage-pruned: a
queue store, lease store, workflow store, feature flag store, log sink, metric sink, or
related codec is generated only when the specification or generated app requires that
domain.

Generated descriptor values are independent of runtime artifact availability. Descriptor
files report the declarations present in the `.sspec` input, and unused descriptor lists
are empty or absent according to the target language shape. The presence of a generic
backend contract must not synthesize runtime descriptor values.

`FieldDescriptor` has the same conceptual shape in every binding:

```text
name
field type enum
original StateSpec type name
required
```

The field type enum is intentionally smaller than the full grammar type expression. It
classifies fields into the stable categories backends and runtime catalogs need:

```text
string
bool
int
int32
int64
long
double
decimal
json
timestamp
duration
uuid
named
list
set
map
optional
reference
```

The original type name is preserved separately so a descriptor can still distinguish, for
example, a named `OrderStatus` field from another named type, or an optional `int?` from
another optional declaration.

Backends expose both single-collection and bulk collection provisioning APIs:

```text
ensure_collection / EnsureCollection / ensureCollection
ensure_collections / EnsureCollections / ensureCollections
```

`ensure_collections` accepts a list of `CollectionDescriptor` records so generated
runtimes can provision all required collections with one API call rather than one call per
collection.

Collection descriptor registration is also the schema upgrade boundary. Re-registering
an existing collection is valid only when the requested descriptor is identical or
backwards compatible with the stored descriptor. Incompatible descriptor changes must
fail with a schema conflict instead of replacing the registered contract. See
[schema-upgrades.md](schema-upgrades.md) for the compatibility definition and shared
helper APIs available to backend adapters.

Bulk collection registration is atomic. Backend adapters must validate the full
descriptor batch against staged descriptor state first and only publish the staged map
after every descriptor has passed compatibility validation. Generated startup code relies
on this behavior so application registration either completes as one coherent catalog or
does not change the registered descriptor set.

## Backend Boundary

Backends and transactions provide generic OCC document storage. They are not queue,
lease, workflow, feature flag, log, or metric engines.

The backend layer owns only:

```text
collection registration
versioned records
queries
transaction read sets
transaction write/delete staging
atomic commit / abort
capabilities
conflict reporting
```

Higher-level runtime components are typed clients of this generic layer. Each component
must register its required collections and store its records through the generic
`Backend` and `Transaction` interfaces.

Typed runtime components must be backend-neutral. A queue store, lease store, workflow
store, feature flag store, log sink, or metric sink should not depend on
`InMemoryBackend`, `InMemoryTransaction`, or equivalent memory-specific concrete types.
The in-memory backend is one adapter behind the same public OCC interfaces that a
production backend implements.

Examples:

```text
QueueStore
  registers queue definition, message, and idempotency collections
  stores queue records through backend get / query / put / erase

WorkflowStore
  registers workflow definition and execution collections
  stores workflow records through backend get / query / put / erase

LogSink / MetricSink
  register definition and event/sample collections
  write observability records through backend put inside the caller's transaction
```

Backend or transaction implementations should not expose domain-specific staging fields
such as queue message maps, workflow execution maps, lease maps, feature flag maps, log
append buffers, or metric append buffers. Those are component records and should be
represented as versioned records in component-owned collections.

## Index-Aware Queries

`CollectionDescriptor` declares secondary indexes through `IndexDescriptor` records:

```text
IndexDescriptor
  name
  fields
  unique
```

The query model can target those declared indexes without binding generated code to a
specific storage implementation.

The shared index query value model is:

```text
IndexValue
  Null
  String
  Integer
  Decimal
  Boolean
  Timestamp

IndexBound
  values
  inclusive
```

Index values are ordered according to the field order in `IndexDescriptor.fields`.

The query model supports these index-aware variants:

```text
IndexEquals
IndexPrefix
IndexRange
```

Generated entity repositories own the mapping from declared entity indexes to backend
queries. API and worker application code should call the generated entity repository
surface, not hard-code collection names or raw index names. The generated repository
helper selects the collection descriptor, index name, key encoding, and `IndexValue`
ordering, then issues a generic backend query through the caller's transaction.
Generated repository helper implementations should pass generated index-name constants
to the generic query path. The string literal for a declared index belongs in exactly
one generated constant definition for the target language; repository helpers, API
handlers, GC/runtime metadata, and tests should reference that constant where the
constant exists.

For list APIs, generated handlers choose the declared index whose leading fields best
match path parameters. A route such as `/tenants/{tenant_id}/accounts/{account_id}/tasks`
should prefer an index beginning with `tenant_id, account_id` over a less selective
tenant-only index. The generated handler still passes only ordered index values to the
repository; the backend remains unaware of API routes or entity semantics.

### Cross-Language Entity Persistence

Generated C++, Go, Java, and Rust entity repositories are expected to interoperate when
they are generated from the same `.sspec` file and point at a backend implementation
that shares the same persisted OCC collection/document storage contract.

The language-neutral contract is:

- collection names come from generated entity-name constants
- entity key fields are encoded in declared key-field order
- key components are encoded as `field=canonical_json` with a trailing newline per
  component
- persisted document fields use the StateSpec field names from generated field
  constants
- declared index names, index field order, and uniqueness flags come from generated
  descriptor constants
- list helpers pass ordered index values through the generated repository surface
  instead of embedding API route semantics in the backend

This means an entity record written through a generated C++ API app should be readable,
updatable, and listable through generated Go, Java, and Rust API apps from the same
spec, subject to the backend's physical storage compatibility. StateSpec defines the
backend API contract and generated repository encoding; it does not yet define a
portable database file format or wire protocol for every backend implementation.

Timestamp values are ordinary persisted document fields. C++, Go, and Java generated
default API handlers currently write current UTC timestamps for foundational
`created_at` and `updated_at` fields. Rust generated default API handlers currently
write the placeholder string `"0"` for those fields. Cross-language readers should
treat these fields as strings until timestamp generation is normalized across
languages.

### IndexEquals

`IndexEquals` targets a named index and supplies equality values in index field order.

Use it for exact index lookups, including unique-index lookups. Backends may also treat
an `IndexEquals` request with fewer values than the declared index field count as a
leading-prefix query when the generated repository intentionally supplies only the
route-constrained prefix.

```text
index_name = "orders_by_tenant_status"
values = ["tenant-001", "Active"]
```

### IndexPrefix

`IndexPrefix` targets a named compound index and supplies a leading prefix of index field
values.

```text
index_name = "orders_by_tenant_status_created"
prefix_values = ["tenant-001", "Active"]
```

### IndexRange

`IndexRange` targets a named index and supplies optional lower and upper `IndexBound`
values.

```text
index_name = "orders_by_tenant_status_created"
lower_bound = ["tenant-001", "Active", "2026-01-01T00:00:00Z"] inclusive
upper_bound = ["tenant-001", "Active", "2026-02-01T00:00:00Z"] exclusive
```

Phase 1 only defines the portable query model. Backend capability flags and stricter
runtime validation rules can be layered on later.

## Shared Runtime Model

Each language also defines runtime-facing interfaces for higher-level primitives in
separate files:

```text
LeaseStore / QueueStore / WorkflowStore / LogSink / MetricSink  C++, Rust, Go
Lease / Queue / Workflow / Log / Metric                         Java
```

The runtime files expose request, result, and record types for:

```text
lease acquire / renew / release / inspect
queue create / inspect definition
queue enqueue / claim / acknowledge / fail / inspect
workflow register definition / inspect definition
workflow start / claim steps / complete step / fail step / cancel / inspect
log event emission
metric sample recording
```

Runtime APIs support two call styles in the same component interface:

1. **Backend-managed transaction calls** receive a backend and manage the transaction internally:

```text
begin transaction
perform component operation
commit on success
abort on failure
```

2. **Caller-managed transaction calls** receive an existing transaction so a user can compose
   multiple lease, queue, workflow, and entity operations into one transaction.

All languages use a `Tx` suffix for caller-managed transaction variants:

```text
operation(backend, request)      C++
operationTx(tx, request)         C++
operation(backend, request)      Rust
operation_tx(tx, request)        Rust
Operation(ctx, backend, req)     Go
OperationTx(ctx, tx, req)        Go
operation(backend, request)      Java
operationTx(tx, request)         Java
```

Logs and metrics follow the same transaction doctrine. A runtime may emit logs or record
metrics through backend-managed calls, or it may use `Tx` variants to make each operation
part of the same caller-managed transaction that mutates entity, queue, lease, or
workflow state.

Each runtime component is responsible for registering the backend collections it uses.
The backend provides `ensure_collection` / `ensure_collections`; the component defines
the collection descriptors, record keys, indexes, and JSON record shape required by its
contract.

Queues are registered explicitly and idempotently with `QueueDefinition` and
`RegisterQueueDefinitionRequest`. The queue identity is the pair `(queue, channel)`.
`QueueDefinitionRegistration` returns the registered definition and whether the queue
definition was newly created. If the same queue definition is already registered,
registration may return `created = false`.

Queue definitions include visibility timeout, maximum attempts, optional dead-letter
queue, and metadata. Enqueue and claim operations should target an existing queue
identity.

Workflow definitions are registered before executions are started. A
`WorkflowDefinition` includes the workflow name, version, start step, expected execution
time, singleton setting, step definitions, and metadata. Workflow definition registration
is immutable: an existing `(workflow_name, workflow_version)` definition cannot be
replaced because doing so would not be idempotent for currently executing workflows. A
changed definition must be registered with an incremented `workflow_version`.

`RegisterWorkflowDefinitionRequest` carries only the definition to register.
`WorkflowDefinitionRegistration` reports the registered definition and whether it was
newly created; if the same definition is already registered, registration may return
`created = false`.

Workflow records and requests that identify a workflow definition carry both
`workflow_name` and `workflow_version`. The pair `(workflow_name, workflow_version)` is
the unique workflow-definition identity, allowing multiple versions of the same workflow
name to coexist.

Workflow step claim requests also carry `workflow_execution_id` because claiming work is
scoped to a particular execution of a workflow definition.

Workflow clients that hold a claimed step can call the keep-alive API to extend the
claim lease before completing or failing the step. The keep-alive request identifies the
workflow execution, worker, current step, current time, and lease duration. Workflow
stores should only extend the lease when the execution is still claimed by that worker
for the same current step.

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

- Entity persistence uses the backend collection and versioned-record contract.
- Lease runtimes implement conditional ownership and fencing through backend records.
- Queue runtimes implement idempotent queue creation and claimable message records.
- Workflow runtimes implement immutable registered definitions and claimable execution records.
- Observability runtimes implement log emission and metric sample recording with typed JSON
  fields and labels.
- Feature flag runtimes implement definition registration and override evaluation through
  backend records.

This keeps StateSpec backend-neutral while still making concrete implementation targets
straightforward to build.
