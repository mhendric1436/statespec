# Binding Generation

StateSpec generation is selected by CLI options. `.sspec` files do not contain
`generate` declarations; text remains the canonical system model, while tooling chooses
which binding package to emit.

## Command

```sh
statespec generate bindings --lang <cpp|go|java|rust> <file.sspec> [--out DIR]
```

When `--out` is omitted, the CLI writes to `generated/<language>`.

## Supported Languages

| Language | Default output root |
|---|---|
| `cpp` | `generated/cpp/` |
| `go` | `generated/go/` |
| `java` | `generated/java/` |
| `rust` | `generated/rust/` |

## Examples

```sh
./build/bin/statespec generate bindings --lang cpp system.sspec --out build/generated/cpp
./build/bin/statespec generate bindings --lang go system.sspec --out build/generated/go
./build/bin/statespec generate bindings --lang java system.sspec --out build/generated/java
./build/bin/statespec generate bindings --lang rust system.sspec --out build/generated/rust
```

## Generated Metadata

Binding generators consume the lowered StateSpec IR, not raw AST declarations.
Namespaces, values, enums, events, external systems, shapes, feature flags, logs,
metrics, entities, queues, leases, API servers, workflows, and policies are emitted as descriptor
metadata where supported so downstream tools can inspect runtime contracts
consistently.

Generated namespace metadata includes:

- name
- qualified member names

Generated value metadata includes:

- name
- type
- optional constraint expression

Generated enum metadata includes:

- name
- members
- optional member values

Generated event metadata includes:

- name
- typed fields

Generated event helpers include a generic event envelope and one event-specific builder
per declared event. Builders accept each payload field as the target binding's typed
JSON value and return an envelope that preserves the declared event name and field map.
Backend event transport is intentionally not generated yet.

Generated worker scaffolding includes passive worker descriptors, runtime context
records, and a language-specific handler interface. Queue polling, lease acquisition,
and workflow dispatch loops are intentionally not generated yet.

Generated external system metadata includes:

- name
- named string properties

Generated API metadata includes:

- name
- method
- path
- input type
- output type
- error type
- workflow start target
- queue enqueue target

Generated API server metadata includes:

- name
- served APIs
- concurrency

Generated shape metadata includes:

- name
- typed fields

Generated bindings also include conservative target-language DTOs for shapes:

- C++ emits a `struct` per shape in `system_descriptors.hpp`.
- Go emits an exported `struct` per shape in `backend/descriptors.go`.
- Java emits a `record` per shape in `Descriptors.java`.
- Rust emits a `struct` per shape in `descriptors.rs`.

Initial DTO type mapping covers scalar primitives directly and represents custom,
temporal, UUID, JSON, collection, and reference types as strings. Descriptor metadata
continues to preserve the exact StateSpec type for downstream tools.

Generated feature flag metadata includes:

- name
- type
- default value
- scope
- owner
- description
- expires

Runtime feature flag evaluation is not generated yet. Generated descriptors can be used
to register feature flag definitions into a backend-provided feature flag store.

The generated domain descriptor APIs are:

| Language | Namespaces | Values | Enums | Events | External systems | Shapes | APIs | API servers | Workers | Policies |
|---|---|---|---|---|---|---|---|---|---|
| C++ | `namespace_descriptors()` | `value_descriptors()` | `enum_descriptors()` | `event_descriptors()` | `external_system_descriptors()` | `shape_descriptors()` | `api_descriptors()` | `api_server_descriptors()` | `worker_descriptors()` | `policy_descriptors()` |
| Go | `NamespaceDescriptors()` | `ValueDescriptors()` | `EnumDescriptors()` | `EventDescriptors()` | `ExternalSystemDescriptors()` | `ShapeDescriptors()` | `ApiDescriptors()` | `ApiServerDescriptors()` | `WorkerDescriptors()` | `PolicyDescriptors()` |
| Java | `namespaceDescriptors()` | `valueDescriptors()` | `enumDescriptors()` | `eventDescriptors()` | `externalSystemDescriptors()` | `shapeDescriptors()` | `apiDescriptors()` | `apiServerDescriptors()` | `workerDescriptors()` | `policyDescriptors()` |
| Rust | `namespace_descriptors()` | `value_descriptors()` | `enum_descriptors()` | `event_descriptors()` | `external_system_descriptors()` | `shape_descriptors()` | `api_descriptors()` | `api_server_descriptors()` | `worker_descriptors()` | `policy_descriptors()` |

Generated log metadata includes:

- name
- level
- event name
- typed fields

Generated metric metadata includes:

- name
- kind
- backend metric name
- unit
- typed labels

The generated observability descriptor APIs are:

| Language | Log descriptors | Metric descriptors |
|---|---|---|
| C++ | `log_definitions()` | `metric_definitions()` |
| Go | `LogDefinitions()` | `MetricDefinitions()` |
| Java | `logDefinitions()` | `metricDefinitions()` |
| Rust | `log_definitions()` | `metric_definitions()` |

The generated transaction-scoped runtime bootstrap APIs are:

| Language | Feature flags | Queues | Leases | Full catalog |
|---|---|---|---|---|
| C++ | `register_feature_flag_definitionsTx` | `create_queue_definitionsTx` | `register_lease_definitionsTx` | `register_runtime_catalogTx` |
| Go | `RegisterFeatureFlagDefinitionsTx` | `CreateQueueDefinitionsTx` | `RegisterLeaseDefinitionsTx` | `RegisterRuntimeCatalogTx` |
| Java | `registerFeatureFlagDefinitionsTx` | `createQueueDefinitionsTx` | `registerLeaseDefinitionsTx` | `registerRuntimeCatalogTx` |
| Rust | `register_feature_flag_definitions_tx` | `create_queue_definitions_tx` | `register_lease_definitions_tx` | `register_runtime_catalog_tx` |

These helpers accept caller-managed OCC transaction objects and backend catalog stores,
matching the binding model used for leases, queues, workflows, logs, and metrics.

Generated binding packages also include separate runtime log and metric request and sink
types:

| Language | Log runtime file | Metric runtime file |
|---|---|---|
| C++ | `log.hpp` | `metric.hpp` |
| Go | `backend/log.go` | `backend/metric.go` |
| Java | `com/statespec/backend/Log.java` | `com/statespec/backend/Metric.java` |
| Rust | `log.rs` | `metric.rs` |

## Authoring Guidelines

- Keep generation choices in build scripts, CI, or developer commands.
- Do not add implementation-specific generation declarations to `.sspec` files.
- Generated bindings should consume the same validated model across languages.
