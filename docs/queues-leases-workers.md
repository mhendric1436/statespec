# Queues, Leases, And Workers

Queues, leases, and workers describe asynchronous execution and durable coordination.
They let a StateSpec file model background work without relying on implicit in-memory
behavior.

## Queues

A queue declares a durable channel for async work.

```statespec
queue EmailQueue {
  namespace workflow
  channel email
  visibility_timeout PT30S
  max_attempts 5

  message SendConfirmation {
    idempotency_key message_id

    payload {
      message_id string
      workflowExecutionId string
      orderId string
      email string
    }
  }
}
```

Recommended queue fields:

| Member | Purpose |
|---|---|
| `namespace` | Logical grouping for queue state. |
| `channel` | Durable channel name. |
| `visibility_timeout` | Claim duration before a message can be retried. |
| `max_attempts` | Retry limit before failure/dead-letter behavior. |
| `dead_letter` | Queue or message target for failed work, when supported. |

The canonical queue shape requires explicit `namespace`, `channel`,
`visibility_timeout`, `max_attempts`, and at least one `message`. StateSpec does not infer
queue retry or visibility defaults.

## Messages

Messages define queue payload shape.

```statespec
message SendConfirmation {
  idempotency_key message_id

  payload {
    message_id string
    order_id string
    email string
  }
}
```

Use an idempotency key when duplicate submission is possible. The key should refer to a
payload field or generated message metadata supported by the selected runtime.

## Leases

A lease declares exclusive ownership of a durable resource.

```statespec
lease OrderReconcilerLease {
  resource "reconciler:orders"
  ttl PT30S
  renew_every PT10S
  holder worker_id
  fencing_token true
  max_ttl PT5M
}
```

Recommended lease fields:

| Member | Purpose |
|---|---|
| `resource` | Durable resource identifier. |
| `ttl` | Maximum lease lifetime without renewal. |
| `renew_every` | Expected renewal interval. |
| `holder` | Holder identity source. |
| `fencing_token` | Enables stale-owner protection where supported. |
| `max_ttl` | Upper bound for total lease lifetime. |

The canonical lease shape requires explicit `resource`, `ttl`, `renew_every`, `holder`,
`fencing_token`, and `max_ttl`. Validation requires `renew_every < ttl <= max_ttl`.

## Workers

Workers describe runtime actors that poll queues or execute workflows.

```statespec
worker EmailWorker {
  singleton false
  polls EmailQueue.SendConfirmation
  concurrency 8
}
```

Singleton worker with a lease:

```statespec
worker OrderReconciler {
  singleton true
  lease OrderReconcilerLease
  executes OrderProcessing
  concurrency 1
}
```

## Authoring Guidelines

Use queues for durable handoff between services, workflows, and workers.

Use leases for exclusive ownership, singleton workers, leader election, reconcilers, and
stale-owner protection.

Use workers to document who consumes queue messages or executes workflows.

Avoid making workflow progress depend on transient process memory. Durable queue state,
entity state, and lease state should describe restart-safe progress.
