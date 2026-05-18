# Workflows And APIs

Workflows describe long-running orchestration. APIs describe external operations that
start workflows, enqueue messages, load entities, allocate entities, and return results.
API servers describe runtime actors that host one or more declared APIs.

## Workflow Skeleton

```statespec
workflow OrderProcessing {
  version 1
  singleton false
  expected_execution_time PT5M
  start validate_order

  step validate_order {
    expected_execution_time PT10S
    max_retries 2
  }

  step charge_payment {
    expected_execution_time PT30S
    max_retries 3
  }

  step send_confirmation {
    expected_execution_time PT10S
    max_retries 3
    enqueue EmailQueue.SendConfirmation
  }
}
```

Recommended workflow members:

| Member | Purpose |
|---|---|
| `version` | Workflow definition version. |
| `singleton` | Whether only one execution may run for a logical scope. |
| `expected_execution_time` | Overall expected workflow duration. |
| `start` | First step name. |
| `step` | Named execution unit. |

The canonical workflow shape requires explicit `version`, `singleton`,
`expected_execution_time`, `start`, and at least one `step`. Each step must declare both
`expected_execution_time` and `max_retries`; StateSpec does not infer runtime retry or
timing defaults for workflow definitions.

## Steps

Each step should have a stable name, expected execution time, and retry intent.

```statespec
step reserve_capacity {
  expected_execution_time PT20S
  max_retries 3
}
```

Step names should be verbs or verb phrases that describe durable progress.

Good:

```text
validate_order
reserve_capacity
charge_payment
send_confirmation
```

Avoid vague names:

```text
step1
do_work
handler
```

## Workflow Statements

The grammar reserves workflow statements for deterministic orchestration:

```statespec
step send_confirmation {
  expected_execution_time PT10S
  max_retries 3

  require order.status == "Active"
  enqueue EmailQueue.SendConfirmation {
    orderId = order.order_id
    email = order.email
  }
  complete
}
```

Common statement families include:

| Statement | Purpose |
|---|---|
| `require` | Assert a predicate before continuing. |
| `set` | Update durable state. |
| `atomic` | Group statements that must commit together. |
| `for_each` | Iterate over a collection. |
| `when` | Conditional execution. |
| `enqueue` | Send a durable queue message. |
| `acquire lease` | Acquire exclusive ownership. |
| `renew lease` | Renew ownership. |
| `release lease` | Release ownership. |
| `start workflow` | Start another workflow. |
| `transition_to` | Move to another workflow step. |
| `complete` | Mark step or workflow success. |
| `fail` | Mark failure with optional reason. |

Generator support for workflow statements can evolve over time. Use the validator and
generator output to confirm which statements are active for a given target.

The compiler currently preserves the first linear workflow behavior slice in AST,
semantic model, and IR: `on`, `input`, `state`, workflow-level `load`, and step-level
`require`, `set`, `emit`, `enqueue`, lease operations, `start workflow`, `transition_to`,
`complete`, and `fail`. Payload-bearing statements should use an explicit semicolon after
the payload block in the current parser milestone.

Validation checks that workflow triggers, input/state types, loaded entities, load key
fields, queue messages, leases, started workflows, transition targets, and feature flag
references resolve against the current system model. General expression type checking and
nested workflow blocks remain future compiler work.

## Parent-Child Orchestration

StateSpec supports a durable parent-child orchestration pattern.

Recommended three-phase convention:

```text
generate_child_ids
creating_children
waiting_children
```

The grammar reserves child-set declarations and child operations:

```statespec
workflow CreateChildren {
  version 1
  singleton false
  expected_execution_time PT10M
  start generate_child_ids

  child_set children_to_create {
    entity ChildEntity
    parent_field parent_id
    id_type uuid
    pending pending_child_ids
    creating creating_child_ids
    succeeded active_child_ids
    failed failed_child_ids
    desired_count 3
  }

  step generate_child_ids {
    expected_execution_time PT5S
    max_retries 1
    reserve child_set children_to_create
    transition_to creating_children
  }

  step creating_children {
    expected_execution_time PT1M
    max_retries 3
    materialize child_set children_to_create
    transition_to waiting_children
  }

  step waiting_children {
    expected_execution_time PT5M
    max_retries 3
    reconcile child_set children_to_create
  }
}
```

Use durable parent fields to track pending, creating, succeeded, and failed child IDs.

## API Skeleton

```statespec
api StartOrderProcessing {
  method POST
  path "/v1/tenants/{tenantId}/orders/{orderId}/start"
  input StartOrderProcessingRequest
  output StartOrderProcessingResponse
  starts workflow OrderProcessing
}
```

API members include:

| Member | Purpose |
|---|---|
| `method` | HTTP method such as `GET`, `POST`, `PUT`, `PATCH`, or `DELETE`. |
| `path` | External path template. |
| `input` | Request shape or type. |
| `output` | Response shape or type. |
| `error` | Error shape or type. |
| `starts workflow` | Workflow started by the API. |
| `enqueues` | Queue message submitted by the API. |
| `behavior` | Detailed semantic behavior. |

Mutating APIs (`POST`, `PUT`, and `PATCH`) must declare `input`. All non-`DELETE` APIs
must declare `output`. This keeps generated contracts explicit instead of relying on
transport-specific empty request or response defaults.

In tenant-scoped systems, API paths must expose tenant identity. Use either the system
tenant field as a path parameter, such as `{tenant_id}` or `{tenantId}`, or a `/tenants/`
route segment.

## API Behavior

Behavior blocks describe implementation intent without committing to a server framework.

```statespec
api CreateOrder {
  method POST
  path "/v1/tenants/{tenantId}/orders"
  input CreateOrderRequest
  output Order

  behavior {
    allocates Order as order
    set order.status = "Creating"
    start workflow OrderProcessing {
      orderId = order.order_id
    }
    returns 202
  }
}
```

Keep API behavior deterministic and side-effect aware. External side effects should be
modeled through durable queues, workflows, or explicit integration declarations.

## API Server Skeleton

`api_server` declarations model the synchronous ingress/runtime actor that serves API
contracts. They are the API-facing counterpart to `worker`: workers consume queues or
execute workflows asynchronously, while API servers serve request/response operations.

```statespec
api_server OrderApi {
  serves StartOrderProcessing
  serves CreateOrder
  concurrency 16
}
```

API server members include:

| Member | Purpose |
|---|---|
| `serves` | API declaration served by this runtime actor. |
| `concurrency` | Intended request handling concurrency for generated metadata and runtime scaffolding. |

Each `serves` target must resolve to an `api` declaration. An API can be served by zero
or more API servers, which keeps API contracts reusable across deployment topologies and
does not force every spec to model runtime hosting immediately.

Generated bindings emit API server descriptors, route descriptors, request/response
context types, and handler interfaces. Runnable HTTP server loops, concrete framework
route registration, and request/response adapter generation remain future work.
