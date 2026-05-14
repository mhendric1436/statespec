# Workflow Launch Control

Use a durable launch-control path when APIs must limit how quickly new workflows begin
or cap the amount of workflow work in progress.

The recommended baseline is:

```text
API
  -> durable launch queue
  -> singleton or sharded launch scheduler
  -> OCC capacity entity
  -> workflow start reservation
  -> workflow start
```

This keeps admission decisions out of direct API handlers. The API records intent by
enqueuing a launch request, and a backend scheduler serializes admission through durable
state.

## Core Declarations

Model these durable pieces:

| Declaration | Purpose |
|---|---|
| `WorkflowLaunchQueue` | Receives launch requests from external APIs. |
| `WorkflowCapacity` | Tracks `max_in_progress`, `in_progress`, and launch-rate budget. |
| `WorkflowStartReservation` | Provides idempotency, auditability, and launch lifecycle state. |
| `WorkflowLaunchScheduler` | Singleton worker that owns admission and workflow start. |
| `WorkflowCompletionQueue` | Receives terminal workflow completion notifications. |
| `WorkflowCompletionAccountingWorker` | Releases capacity and closes reservations after terminal completion. |

The scheduler should use one OCC transaction for the decision:

```text
read WorkflowCapacity
read or create WorkflowStartReservation by idempotency key
verify in_progress and token budget
increment in_progress
transition reservation to Admitted or Rejected
start workflow or record launch intent
commit
```

On workflow terminal completion, run a completion-accounting path that transitions the
reservation to `Completed` and decrements `WorkflowCapacity.in_progress` in an OCC
transaction.

## State Model

`WorkflowStartReservation` should have an explicit state machine:

```text
Pending -> Admitted -> Launched -> Completed
Pending -> Rejected
Pending -> Expired
Launched -> Expired
```

Terminal reservation states should declare garbage-collection policies. Keep completed
reservations longer when they are part of operational audit or replay analysis.

## Scaling

Start with one singleton scheduler and one capacity record. When the singleton becomes a
bottleneck, shard both the queue and capacity entity by tenant, workflow type, region, or
hash bucket.

Each shard should have:

- a stable queue channel or message partition
- one scheduler owner protected by a lease
- one capacity record for OCC admission
- deterministic idempotency keys for launch requests

## Example

See [`examples/workflow-launch-control.sspec`](../examples/workflow-launch-control.sspec)
for a complete validating baseline with launch and completion queues, capacity tracking,
reservations, singleton workers, and API declarations.
