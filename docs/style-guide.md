# StateSpec Style Guide

This guide describes practical conventions for authoring readable and maintainable
`.sspec` files.

## Goals

A good `.sspec` file should be:

- explicit enough for review
- deterministic enough for generation
- stable enough for long-term maintenance
- readable enough to replace ambiguous prose design notes

## File Layout

Recommended top-level order inside a `system` block:

```text
1. tenant and system_tenant
2. namespaces
3. values, enums, shapes, events, and external systems
4. feature flags
5. logs and metrics
6. entities
7. queues
8. leases
9. workers
10. workflows
11. APIs and API servers
12. policies
```

Keep related declarations close together. For example, put queue messages inside their
queue and put workflow steps inside their workflow.
The validator emits a warning when root `system` members appear out of canonical order.

## Naming

Use PascalCase for top-level named declarations:

```statespec
entity Order
workflow OrderProcessing
feature_flag NewScheduler
log WorkflowLaunchDecision
metric WorkflowLaunchAttempts
queue EmailQueue
lease OrderReconcilerLease
api StartOrderProcessing
policy WorkflowAccess
```

Use snake_case for fields, keys, state buckets, and step names:

```statespec
fields {
  tenant_id string
  order_id string
  created_at timestamp
}

step validate_order {
  expected_execution_time PT10S
  max_retries 1
}
```

Use lifecycle states that read like durable entity states:

```text
Creating
Active
Updating
Deleting
Deleted
Failed
```

## Entity Style

Use canonical entity member order:

```text
key, ownership, version, fields, state_machine, relations, children, invariants, indexes
```

```statespec
entity Order {
  key tenant_id, order_id
  ownership {
    authority: system
    system_of_record: self
    lifecycle: authoritative
  }

  fields {
    created_at timestamp
    updated_at timestamp
    status string
    tenant_id string
    order_id string
  }

  state_machine {
    state Active
    initial Active
  }
}
```

The validator emits a warning when entity members appear out of canonical order.

For lifecycle entities, include a `state_machine` block. Prefer explicit state machines
over prose descriptions.

## Workflow Style

Workflow steps should describe durable progress, not implementation details.

Use canonical workflow member order:

```text
version, singleton, expected_execution_time, start, on, input, state, load, step
```

The validator emits a warning when workflow members appear out of canonical order.
The validator rejects workflows that omit `singleton`, workflow-level
`expected_execution_time`, or step-level `max_retries`; these runtime semantics must be
authored explicitly.

Good:

```text
validate_order
reserve_capacity
charge_payment
send_confirmation
```

Avoid:

```text
step1
do_stuff
handler
callback
```

Every step should normally include:

```statespec
expected_execution_time PT10S
max_retries 3
```

Use ISO-8601-style duration literals such as `PT5S`, `PT30S`, `PT5M`, and `P1D`.

## Observability Style

Use canonical log member order:

```text
level, event_name, fields
```

Use canonical metric member order:

```text
kind, name, unit, labels
```

The validator emits warnings when log or metric members appear out of canonical order.
Metric labels should be low-cardinality dimensions; use log fields for high-cardinality
identifiers and detailed payload data.

## Queue Style

Queue names should end in `Queue` unless the domain name is already clearly a queue.

Message names should be commands or events:

```text
SendConfirmation
CreateChildEntity
OrderCreated
PaymentCaptured
```

Payload fields should use stable identifiers rather than transient implementation terms.

## Lease Style

Lease names should end in `Lease`.

Use resource names that are stable and scoped:

```statespec
resource "reconciler:orders"
resource "tenant:{tenant_id}:order:{order_id}"
```

Prefer fencing tokens when stale-owner protection matters:

```statespec
fencing_token true
```

## API Style

API names should describe intent:

```text
CreateOrder
StartOrderProcessing
DeleteOrder
GetOrder
```

Paths should be versioned and resource-oriented:

```statespec
path "/v1/tenants/{tenantId}/orders/{orderId}/start"
```

Use APIs to declare external contracts. Use workflows and queues to model durable async
work started by those APIs.

## Policy Style

Policy names should describe the protected area:

```text
WorkflowAccess
OrderAccess
TenantQuotaPolicy
```

Use canonical policy member order:

```text
tenant, allow, deny, quota, audit
```

Each policy must declare `tenant scoped_by` and at least one policy action. The validator
warns when policy members appear out of canonical order.

Keep conditions simple and reviewable:

```statespec
allow StartOrderProcessing when caller.role in ["operator", "control-plane"]
```

## Comments

Use comments to explain why, not to replace missing structure.

Good:

```statespec
// External payment provider may report terminal failure after initial authorization.
Charging -> Failed
```

Avoid comments that duplicate syntax:

```statespec
// This is the Order entity.
entity Order { ... }
```

## Review Checklist

Before submitting a `.sspec` change:

- Run `statespec validate`.
- Inspect `statespec ast` for parser surprises.
- Generate the affected targets.
- Check that all references resolve.
- Check that lifecycle transitions are explicit.
- Check that async work is durable through queues, workflows, leases, or entity state.
- Check that generator output paths are expected.
