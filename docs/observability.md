# Logs And Metrics

StateSpec models observability as declared log events and metric instruments. These
declarations are part of the canonical system model, so generated runtimes can expose one
stable catalog across implementation languages.

Observability declarations describe the shape and identity of signals. They do not choose
a concrete logging library, metrics backend, exporter, sampling policy, or storage
provider.

## Log Declarations

A `log` declaration names one structured event.

```statespec
log WorkflowLaunchDecision {
  level info
  event_name "workflow.launch.decision"

  fields {
    tenant_id string
    workflow_name string
    decision string
  }
}
```

| Member | Required | Purpose |
|---|---:|---|
| `level` | yes | One of `debug`, `info`, `warn`, or `error`. |
| `event_name` | yes | Backend event name emitted to logging infrastructure. |
| `fields` | no | Typed structured fields carried by the event. |

Validation requires log declaration names to use PascalCase. `event_name` values must be
unique within the composed system.

Log fields use the normal StateSpec field type model. Optional fields may use the `?`
suffix.

In a tenant-scoped system, every log declaration must include the system tenant field in
its `fields` block. For example, `tenant scoped_by tenant_id` requires a `tenant_id`
field on each log.

## Metric Declarations

A `metric` declaration names one metric instrument.

```statespec
metric WorkflowLaunchAttempts {
  kind counter
  name "workflow_launch_attempts_total"
  unit count

  labels {
    tenant_id string
    workflow_name string
    decision string
  }
}
```

| Member | Required | Purpose |
|---|---:|---|
| `kind` | yes | One of `counter`, `gauge`, or `histogram`. |
| `name` | yes | Backend metric name emitted to metrics infrastructure. |
| `unit` | yes | Unit name, such as `count`, `seconds`, or `bytes`. |
| `labels` | no | Low-cardinality dimensions for aggregation. |

Validation requires metric declaration names to use PascalCase. Backend metric `name`
values must be unique within the composed system.

Metric labels are intentionally restricted to `string`, `bool`, and `int` so generated
runtimes can keep metric cardinality and encoding portable. Label names must also avoid
obvious high-cardinality dimensions such as IDs, UUIDs, GUIDs, timestamps, raw payloads,
and error messages. The system tenant field is the only identifier-style metric label
reserved by the language.

In a tenant-scoped system, every metric declaration must include the system tenant field
in its `labels` block. For example, `tenant scoped_by tenant_id` requires a `tenant_id`
label on each metric.

## Include Composition

Included `.sspec` files contribute their `log` and `metric` declarations to the composed
root system. Duplicate log `event_name` values and duplicate metric backend `name` values
are rejected after composition, regardless of which file declared them.

Use this to share standard operational signals across systems:

```statespec
statespec 0.1;
include "./workflow-launch-control.sspec";

system OrderSystem {
  log OrderWorkflowStarted {
    level info
    event_name "order.workflow.started"
    fields {
      tenant_id string
      order_id string
    }
  }
}
```

## IR And Generated Descriptors

The semantic IR contains normalized log and metric declarations:

```text
IrLog
  name
  level
  event_name
  fields

IrMetric
  name
  kind
  backend_name
  unit
  labels
```

Binding generators consume this IR and emit descriptor catalogs in C++, Go, Java, and
Rust. Descriptor catalogs are intended for runtime registration, documentation,
configuration checks, and adapter setup. Generated descriptors also include bootstrap
helpers that register observability and workflow catalogs through the runtime binding
interfaces.

The generated descriptor APIs are:

| Language | Log descriptors | Metric descriptors |
|---|---|---|
| C++ | `log_definitions()` | `metric_definitions()` |
| Go | `LogDefinitions()` | `MetricDefinitions()` |
| Java | `logDefinitions()` | `metricDefinitions()` |
| Rust | `log_definitions()` | `metric_definitions()` |

Generated bootstrap helpers include:

| Language | Collections | Observability | Workflows |
|---|---|---|---|
| C++ | `ensure_system_collections(...)` | `register_observability_catalogTx(...)` | `register_workflow_definitionsTx(...)` |
| Go | `EnsureSystemCollections(...)` | `RegisterObservabilityCatalogTx(...)` | `RegisterWorkflowDefinitionsTx(...)` |
| Java | `ensureSystemCollections(...)` | `registerObservabilityCatalogTx(...)` | `registerWorkflowDefinitionsTx(...)` |
| Rust | `ensure_system_collections(...)` | `register_observability_catalog_tx(...)` | `register_workflow_definitions_tx(...)` |

## Runtime Binding Model

Generated bindings also include separate runtime surfaces for logs and metrics:

| Language | Log runtime file | Metric runtime file |
|---|---|---|
| C++ | `log.hpp` | `metric.hpp` |
| Go | `backend/log.go` | `backend/metric.go` |
| Java | `com/statespec/backend/Log.java` | `com/statespec/backend/Metric.java` |
| Rust | `log.rs` | `metric.rs` |

The runtime model is deliberately small:

```text
LogEvent
  name
  level
  event_name
  fields

MetricSample
  name
  kind
  backend_name
  value
  unit
  labels
```

Fields and labels use the typed JSON value already shared by the backend bindings.

Each language exposes separate log and metric sink interfaces with backend-managed and
caller-managed transaction variants. This lets generated workers emit logs and record
metrics either as standalone operations or as part of the same OCC transaction that
mutates entity, queue, lease, or workflow state.

### Transaction Semantics

Log and metric `Tx` methods participate in the caller-managed OCC transaction. A
transactional log event or metric sample is staged with the transaction and becomes
visible to exporters only after commit. If the transaction aborts or rolls back, staged
observability records are discarded.

Backend-managed log and metric methods may open and commit their own transaction, or use
an adapter-specific durable write path, but they must preserve the same validation and
idempotency rules as the `Tx` methods.

Log and metric definition registration is idempotent. Registering the same definition
again is a no-op. Registering the same logical name with an incompatible shape should
fail through the binding's backend error mechanism.

Runtime implementations should validate emitted log fields and metric labels against the
registered definitions. Required fields or labels must be present, unexpected fields or
labels should be rejected unless an adapter explicitly documents a compatibility mode,
and metric label values should remain low-cardinality.

## Authoring Guidelines

- Use stable `event_name` and metric backend `name` values. Treat them as external
  contracts once dashboards, alerts, or log queries depend on them.
- Keep metric labels low-cardinality. The validator rejects obvious IDs, timestamps, raw
  payloads, and error messages as labels.
- Include the tenant field in all log fields and metric labels for tenant-scoped systems.
- Prefer explicit log fields over unstructured message text.
- Do not encode business semantics in observability declarations that should belong in
  entities, workflows, policies, or APIs.
