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
runtimes can keep metric cardinality and encoding portable.

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

Binding generators consume this IR and emit passive descriptor catalogs in C++, Go, Java,
and Rust. Descriptor catalogs are intended for runtime registration, documentation,
configuration checks, and adapter setup.

The generated descriptor APIs are:

| Language | Log descriptors | Metric descriptors |
|---|---|---|
| C++ | `log_definitions()` | `metric_definitions()` |
| Go | `LogDefinitions()` | `MetricDefinitions()` |
| Java | `logDefinitions()` | `metricDefinitions()` |
| Rust | `log_definitions()` | `metric_definitions()` |

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

## Authoring Guidelines

- Use stable `event_name` and metric backend `name` values. Treat them as external
  contracts once dashboards, alerts, or log queries depend on them.
- Keep metric labels low-cardinality. Avoid IDs, raw user input, timestamps, request
  payloads, and error messages as labels.
- Prefer explicit log fields over unstructured message text.
- Do not encode business semantics in observability declarations that should belong in
  entities, workflows, policies, or APIs.
