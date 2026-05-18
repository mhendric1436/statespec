# Policies

Policies describe tenant scoping, authorization, quotas, and audit intent. They keep
security and operational constraints close to the system behavior they govern.

## Policy Skeleton

```statespec
policy WorkflowAccess {
  tenant scoped_by tenant_id

  allow StartOrderProcessing when caller.role in ["operator", "control-plane"]
  allow PollOrderWorkflowSteps when caller.role == "worker"
  deny DeleteOrder when caller.role == "readonly"

  quota activeWorkflows: count(active_workflows) <= 1000
  audit StartOrderProcessing
}
```

## Tenant Scoping

Use tenant scoping to identify the field used for tenant isolation.

```statespec
tenant scoped_by tenant_id
```

Policy tenant scoping is required and must match the system-level tenant field. For a
system declared with `tenant scoped_by tenant_id`, every policy uses
`tenant scoped_by tenant_id`.

The scoped field should exist on the relevant entity, request shape, payload, or generated
context used by the target runtime.

Validation enforces this for tenant-addressable declarations: each entity must include the
scoped tenant field in its key and fields, each queue message payload must include it, and
each API input shape must carry it. Observability contracts must also carry the scoped
tenant field through log fields and metric labels.

## Allow And Deny Rules

Authorization rules name an action and a boolean condition.

```statespec
allow StartOrderProcessing when caller.role in ["operator", "control-plane"]
deny DeleteOrder when caller.role == "readonly"
```

Rules should be deterministic and should reference stable inputs such as caller context,
tenant identity, request fields, entity fields, or workflow state.

## Quotas

Quotas name operational limits.

```statespec
quota activeOrders: count(active_orders) <= 10000
quota queueDepth: count(email_queue.pending) <= 50000
```

Use quotas to document limits that runtime services or generated policy hooks should
enforce.

## Audit Points

Audit declarations identify actions that should produce audit records.

```statespec
audit StartOrderProcessing
audit DeleteOrder
```

Audit declarations should refer to meaningful business or control-plane actions, not low
level helper functions.

## Authoring Guidelines

- Keep policy names aligned with the system area they protect.
- Use canonical policy member order: `tenant`, `allow`, `deny`, `quota`, `audit`.
- Include at least one `allow`, `deny`, `quota`, or `audit` declaration.
- Prefer positive `allow` rules and reserve `deny` for explicit exclusions.
- Keep conditions simple enough to review.
- Do not hide authorization-critical behavior in prose comments.
- Treat policies as part of the canonical system design, not a later service-layer add-on.
