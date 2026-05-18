# Feature Flags

Feature flags declare rollout and configuration switches as part of the canonical
system model. They should be referenced from expressions, not hidden in annotations or
generator-specific configuration.

## Feature Flag Skeleton

```statespec
feature_flag NewScheduler {
  type bool
  default false
  scope tenant
  owner "platform"
  description "Route order processing through the new scheduler"
  expires "2026-12-31"
}
```

Recommended members:

| Member | Purpose |
|---|---|
| `type` | The flag value type. Initial support is `bool`, `string`, `int`, and `decimal`. |
| `default` | The deterministic fallback value used when no rollout override exists. |
| `scope` | The rollout scope: `system`, `tenant`, `user`, or `entity EntityName`. |
| `owner` | The team or service responsible for the flag lifecycle. |
| `description` | Human-readable intent for review and generated catalogs. |
| `expires` | Date or policy marker for removing temporary flags. |

## Expression Usage

Boolean flags should be used with `feature_enabled`:

```statespec
when feature_enabled(NewScheduler) {
  transition_to schedule_with_new_engine
}
```

Typed flags should be read with `feature_value`:

```statespec
require feature_value(MaxPendingOrders) >= count(order.pending_ids)
```

Feature flag expressions can appear anywhere normal expressions are accepted, including
workflow `when` blocks, `require` statements, transition conditions, and policy rules.
See [expressions.md](expressions.md) for the shared expression grammar and allowed
built-ins.

## Invalid Examples

Missing defaults are invalid because fallback behavior must be deterministic:

```statespec
feature_flag NewScheduler {
  type bool
  scope tenant
}
```

Defaults must match the declared type:

```statespec
feature_flag MaxPendingOrders {
  type int
  default "many"
  scope tenant
}
```

`feature_enabled` is only valid for boolean flags:

```statespec
feature_flag SchedulerMode {
  type string
  default "current"
  scope system
}

workflow OrderProcessing {
  version 1
  singleton false
  expected_execution_time PT1M
  start route_order

  step route_order {
    expected_execution_time PT10S
    max_retries 1
    when feature_enabled(SchedulerMode) {
      complete
    }
  }
}
```

## Feature Flags Versus State

Feature flags control rollout and configuration. They should not replace durable entity
state.

Use a feature flag for:

- gradual rollout of a workflow branch
- temporary enablement of a new integration path
- tenant-specific operational limits

Use entity state for:

- lifecycle progress that must survive retries
- auditable business decisions
- state observed by other workflows or systems

## Authoring Guidelines

- Use PascalCase flag names.
- Declare `type` and `default` for every flag.
- Prefer `scope tenant` for multi-tenant rollout behavior.
- Prefer `scope entity EntityName` only when the rollout decision is tied to a durable entity.
- Add `expires` for temporary rollout flags.
- Remove flags after the old behavior is no longer supported.
