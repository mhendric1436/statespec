# Includes

Use `include` to reuse declarations from another `.sspec` file.

```statespec
statespec 0.1;
include "./workflow-launch-control.sspec";

system OrderSystem {
  // service-specific declarations
}
```

Include directives must appear after the optional `statespec` header and before imports
or the root `system` declaration.

## Resolution

`statespec validate` and `statespec generate bindings` resolve includes before semantic
validation.

Resolution rules:

- Include paths are resolved relative to the file that declares the `include`.
- Include cycles are rejected.
- Repeated includes of the same normalized path are de-duplicated.
- Missing include files are reported as diagnostics on the include directive.
- `statespec ast` remains file-local and reports parsed include directives without
  expanding them.
- `statespec fmt` remains file-local and does not inline or recursively format included
  files.

## System Composition

Included files are treated as declaration providers. The root file keeps the final system
name.

For example, this root file:

```statespec
include "./workflow-launch-control.sspec";

system OrderSystem {
  entity Order { ... }
}
```

and this included file:

```statespec
system WorkflowLaunchControl {
  entity WorkflowCapacity { ... }
  queue WorkflowLaunchQueue { ... }
}
```

compose as if validation and generation saw:

```statespec
system OrderSystem {
  entity WorkflowCapacity { ... }
  queue WorkflowLaunchQueue { ... }
  entity Order { ... }
}
```

The included system name is not the final system name. It acts as a source boundary for
the declarations in that file.

## Merged Declarations

The following system members are appended from included systems into the root system:

- `feature_flag`
- `entity`
- `queue`
- `lease`
- `worker`
- `api`
- `workflow`
- `policy`

Included members are added before root members. Duplicate declarations after composition
are reported by semantic validation.

## System-Level Declarations

System-level declarations are handled specially:

- If an included file declares `tenant scoped_by ...` and the root does not, the composed
  root system inherits it.
- If both included and root files declare tenant scopes and they differ, validation
  reports a conflict.
- If an included file declares `system_tenant configured` and the root does not, the root
  inherits it.

## Example

See
[`examples/order-system-with-launch-control.sspec`](../examples/order-system-with-launch-control.sspec)
for a root spec that includes the reusable workflow launch-control baseline.
