# Workflows And APIs

Workflows describe long-running orchestration. APIs describe external operations that
start workflows, enqueue messages, load entities, allocate entities, and return results.
API servers describe runtime actors that host one or more declared APIs.

Canonical CRUD is entity-owned. Use an entity `api` block for standard create, get,
key/index-backed list, status update, and delete operations. Use top-level `api`
declarations for business actions that are not standard entity lifecycle operations,
such as starting a workflow, enqueueing a command, or exposing a domain-specific action.

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

Use workflow-level `load` declarations to name the primary durable entity a workflow
operates on. A workflow that coordinates child entity work should load the parent entity
and then declare child orchestration intent with `child_workflow` blocks before its
steps.

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

The compiler preserves workflow behavior in AST, semantic model, and IR: `on`, `input`,
`state`, workflow-level `load`, `child_set`, step-level linear statements, and nested
`atomic`, `for_each`, and `when` blocks. Child orchestration statements such as
`create child`, `observe child`, `move`, `reserve child_set`, `materialize child_set`,
and `reconcile child_set` are represented as workflow statements.

Validation checks that workflow triggers, input/state types, loaded entities, load key
fields, child-set entities and parent fields, queue messages, leases, started workflows,
transition targets, child entities, child-set statement targets, and feature flag
references resolve against the current system model. Expression syntax and allowed
built-ins are validated recursively through nested workflow blocks. Expression type
checking and generated business workflow bodies remain future compiler work.

## Parent-Child Orchestration

StateSpec supports a durable parent-child orchestration pattern.

The recommended form is the entity-specific `child_workflow` block. It removes most of
the boilerplate from the three-phase pattern while keeping the generated names
deterministic.

```statespec
workflow AccountLifecycle {
  version 1
  singleton false
  expected_execution_time PT5M
  start inspect_account

  load Account by account_id as account;

  child_workflow projects {
    child_entity Project
    child_workflow ProjectLifecycle
    child_id project_id string
    parent_ref account_id = account.account_id
    desired_count account.desired_project_count
    create {
      tenant_id: account.tenant_id
      account_id: account.account_id
      project_id: project_id
      name: account.display_name
    }
    success when Project.status == Active
    failure when Project.status == Deleted
  }

  step inspect_account {
    expected_execution_time PT10S
    max_retries 2
    transition_to reconcile_account
  }
}
```

The block declares:

| Member | Purpose |
|---|---|
| `child_entity` | Entity type created or monitored by the parent workflow. |
| `child_workflow` | Workflow that owns each child entity's lifecycle. |
| `child_id` | Child identity field and type used to derive bucket names. |
| `parent_ref` | Child field that links back to the loaded parent expression. |
| `desired_count` | Expression that determines how many child IDs are expected. |
| `create` | Child entity fields populated by the parent workflow. |
| `success when` | Child state expression that marks a child as succeeded. |
| `failure when` | Child state expression that marks a child as failed. |

Bucket and parent step names are generated from the child ID field. For
`child_id project_id string`, StateSpec derives:

```text
pending_project_ids
creating_project_ids
succeeded_project_ids
failed_project_ids
generate_project_ids
create_projects
wait_for_projects
```

Generated C++, Go, Java, and Rust workflow descriptors include this child-workflow
metadata under `metadata.child_workflows`. Descriptor metadata is used for generated
catalogs, inspection, and future worker generation. User-owned workflow handlers remain
responsible for performing idempotent child creation and observing child entity state
until StateSpec adopts a fully generated child-orchestration runner.

The lower-level `child_set` form remains reserved for explicit bucket declarations and
child-set operations. Prefer `child_workflow` unless the spec needs to model the bucket
fields manually.

The underlying three-phase convention is:

```text
generate_child_ids
creating_children
waiting_children
```

The grammar also reserves child-set declarations and child operations:

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

An API must choose one primary action model. Use `starts workflow` when the API admits a
durable workflow, use `enqueues` when the API submits a queue message, or omit both for a
synchronous request/response operation. Do not combine `starts workflow` and `enqueues`
on the same API.

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
User-facing APIs should expose caller intent, not operator-only remote execution metadata
such as endpoint URLs, credential references, retry policy, timeout policy, or service
routing. Use tenant-scoped operator metadata APIs for those fields and resolve the
metadata through backend/OCC helpers at execution time.

See [external-system-metadata.md](external-system-metadata.md) for the recommended
external-system metadata model.

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
