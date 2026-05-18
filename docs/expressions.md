# Expressions

StateSpec expressions are deterministic predicates and values used by constraints,
invariants, workflow requirements, workflow assignments, and policy rules.

Supported operators are:

```text
||
&&
== !=
< <= > >= in
+ -
* / %
! -
```

Expressions support literals, qualified names, selectors, list literals, function calls,
and parentheses. There is no implicit runtime function namespace beyond the built-ins
listed here.

Allowed built-ins:

| Group | Functions |
|---|---|
| Collections | `count`, `empty`, `contains`, `disjoint`, `exists` |
| Entity/state | `current_state`, `can_transition`, `changed` |
| Child orchestration | `generate_ids`, `ordinal_of`, `child_state`, `all_children`, `any_child` |
| Time | `now`, `duration`, `expired` |
| Feature flags | `feature_enabled`, `feature_value` |

Any other function call is invalid. Feature flag calls must reference declared feature
flags, and `feature_enabled(flag)` requires a boolean feature flag.

Examples:

```statespec
allow StartOrderProcessing when feature_enabled(NewScheduler) && caller.role in ["operator", "admin"]
quota pending_orders: feature_value(MaxPendingOrders) >= count(order.pending_ids)

invariants {
  childIdsPartitioned: disjoint(pendingChildIds, creatingChildIds, succeededChildIds, failedChildIds)
}
```
