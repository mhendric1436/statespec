# Generation

StateSpec generation is selected by CLI options. `.sspec` files do not contain
`generate` declarations; text remains the canonical system model, while tooling chooses
which downstream artifact to emit.

## Commands

```sh
statespec generate bindings --lang <cpp|go|java|rust> <file.sspec> [--out DIR] [--tier <all|common|api|worker>] [--template-root DIR]
statespec generate openapi <file.sspec> [--out DIR]
```

When `--out` is omitted, the CLI writes to `generated/<language>`.
OpenAPI generation writes `generated/openapi/openapi.json` by default.

Protocol buffer generation is not implemented yet.

Binding generation writes all artifacts by default. Use `--tier common` to emit only
shared descriptors and runtime support, `--tier api` to emit shared plus API-server
artifacts, or `--tier worker` to emit shared plus worker artifacts.

Binding generation resolves source-backed template packages in this order:

1. `--template-root DIR`, scoped to `DIR/<language>`
2. `STATESPEC_TEMPLATE_ROOT`, scoped to `$STATESPEC_TEMPLATE_ROOT/<language>`
3. the development default `bindings/<language>`

The template root is intentionally language-scoped so future checked-in packages can move
to `templates/bindings/<language>` without changing generator internals. Generated
dynamic descriptor blocks are still rendered by the compiler until each artifact is
converted to a source template.

Generated bindings also include minimal language-native packaging metadata:

| Language | Packaging files |
|---|---|
| `cpp` | `Makefile` with clang++-based common, API, worker build, check, and package targets |
| `go` | `go.mod` plus `Makefile` with common, API, worker build, check, and package targets |
| `java` | `Makefile` with javac-based build, check, and package targets for generated sources |
| `rust` | `Cargo.toml`, tier-aware `lib.rs`, and `Makefile` with build, check, and package targets |

Common-tier runtime support and descriptor source is physically emitted under a
`common/` directory in each language output root. API-tier code is emitted under `api/`
as focused files for descriptors, handlers, routes, and external-system operator
metadata APIs. Worker-tier code is emitted under `worker/` as focused files for
worker descriptors, contexts, handlers, queues, leases, and workflows.

Generated application shells are intentionally separated from user-owned implementation
code. See [generated-extension-points.md](generated-extension-points.md) for the API
handler, worker handler, workflow step handler, and operator metadata handler contracts
that application code implements outside the generated output tree.

## Application Artifact Model

The binding generators use a stable application artifact model for the planned complete
API-server and worker applications plus a common-tier in-memory backend for local
linking. The API server shell, API dispatcher, worker registry, worker application
shell, workflow step handler interfaces, and workflow runner are emitted as the first
generated application artifacts. The remaining application and memory backend files are
still modeled as generated-owned artifacts but are not emitted yet. The current
generators also emit the lower-level common, API contract, and worker contract files
described above.

The common in-memory backend artifact responsibilities are:

| Kind | Responsibility |
|---|---|
| `memory_backend` | In-memory backend composition root for local API and worker linking |
| `memory_transaction` | Optimistic-concurrency transaction implementation |
| `memory_feature_flag_store` | Feature flag definition, override, and evaluation store |
| `memory_queue_store` | Queue definition, enqueue, claim, ack, and fail store |
| `memory_lease_store` | Lease definition, acquire, renew, release, and fencing store |
| `memory_workflow_store` | Workflow definition, execution, step claim, keep-alive, complete, and fail store |
| `memory_log_sink` | Transaction-scoped structured log sink with inspect support |
| `memory_metric_sink` | Transaction-scoped metric sink with inspect support |

The API application artifact responsibilities are:

| Kind | Responsibility |
|---|---|
| `api_application` | API application composition root |
| `api_server` | API server lifecycle and request loop |
| `api_dispatcher` | Route-to-handler dispatch |
| `api_handler_registry` | User implementation registry for API handlers |
| `api_main` | API process entrypoint |

The worker application artifact responsibilities are:

| Kind | Responsibility |
|---|---|
| `worker_application` | Worker application composition root |
| `worker_runtime` | Queue polling, lease management, and execution runtime |
| `worker_registry` | Registry derived from `worker` declarations |
| `workflow_runner` | Workflow claim, keep-alive, complete, fail, and retry loop |
| `workflow_step_handlers` | User implementation registry for workflow steps |
| `worker_main` | Worker process entrypoint |

Planned API application filenames:

| Language | Files |
|---|---|
| `cpp` | `api/api_application.hpp`, `api/api_server.hpp`, `api/api_dispatcher.hpp`, `api/api_handler_registry.hpp`, `api/main.cpp` |
| `go` | `api/backend/api_application.go`, `api/backend/api_server.go`, `api/backend/api_dispatcher.go`, `api/backend/api_handler_registry.go`, `api/cmd/api/main.go` |
| `java` | `api/com/statespec/generated/ApiApplication.java`, `api/com/statespec/generated/ApiServer.java`, `api/com/statespec/generated/ApiDispatcher.java`, `api/com/statespec/generated/ApiHandlerRegistry.java`, `api/com/statespec/generated/ApiMain.java` |
| `rust` | `api/api_application.rs`, `api/api_server.rs`, `api/api_dispatcher.rs`, `api/api_handler_registry.rs`, `api/main.rs` |

Planned worker application filenames:

| Language | Files |
|---|---|
| `cpp` | `worker/worker_application.hpp`, `worker/worker_runtime.hpp`, `worker/worker_registry.hpp`, `worker/workflow_runner.hpp`, `worker/workflow_step_handlers.hpp`, `worker/main.cpp` |
| `go` | `worker/backend/worker_application.go`, `worker/backend/worker_runtime.go`, `worker/backend/worker_registry.go`, `worker/backend/workflow_runner.go`, `worker/backend/workflow_step_handlers.go`, `worker/cmd/worker/main.go` |
| `java` | `worker/com/statespec/generated/WorkerApplication.java`, `worker/com/statespec/generated/WorkerRuntime.java`, `worker/com/statespec/generated/WorkerRegistry.java`, `worker/com/statespec/generated/WorkflowRunner.java`, `worker/com/statespec/generated/WorkflowStepHandlers.java`, `worker/com/statespec/generated/WorkerMain.java` |
| `rust` | `worker/worker_application.rs`, `worker/worker_runtime.rs`, `worker/worker_registry.rs`, `worker/workflow_runner.rs`, `worker/workflow_step_handlers.rs`, `worker/main.rs` |

Planned in-memory backend filenames:

| Language | Files |
|---|---|
| `cpp` | `common/memory/backend.hpp`, `common/memory/transaction.hpp`, `common/memory/feature_flag_store.hpp`, `common/memory/queue_store.hpp`, `common/memory/lease_store.hpp`, `common/memory/workflow_store.hpp`, `common/memory/log_sink.hpp`, `common/memory/metric_sink.hpp` |
| `go` | `common/backend/memory/backend.go`, `common/backend/memory/transaction.go`, `common/backend/memory/feature_flags.go`, `common/backend/memory/queues.go`, `common/backend/memory/leases.go`, `common/backend/memory/workflows.go`, `common/backend/memory/logs.go`, `common/backend/memory/metrics.go` |
| `java` | `common/com/statespec/backend/memory/InMemoryBackend.java`, `common/com/statespec/backend/memory/InMemoryTransaction.java`, `common/com/statespec/backend/memory/InMemoryFeatureFlagStore.java`, `common/com/statespec/backend/memory/InMemoryQueueStore.java`, `common/com/statespec/backend/memory/InMemoryLeaseStore.java`, `common/com/statespec/backend/memory/InMemoryWorkflowStore.java`, `common/com/statespec/backend/memory/InMemoryLogSink.java`, `common/com/statespec/backend/memory/InMemoryMetricSink.java` |
| `rust` | `common/memory/backend.rs`, `common/memory/transaction.rs`, `common/memory/feature_flags.rs`, `common/memory/queues.rs`, `common/memory/leases.rs`, `common/memory/workflows.rs`, `common/memory/logs.rs`, `common/memory/metrics.rs` |

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
./build/bin/statespec generate openapi system.sspec --out build/generated/openapi
```

## OpenAPI Generation

The OpenAPI generator emits an OpenAPI 3.0.3 document from the same validated IR used
by the binding generators. Declared `api` blocks become HTTP operations with path
parameters inferred from `{name}` path segments, JSON request bodies from `input`
shapes, JSON success responses from `output` shapes, and default error responses from
`error` shapes when present.

Shape declarations become reusable `components.schemas` entries. Primitive StateSpec
types are mapped to conservative JSON Schema types, while references to declared
shapes become OpenAPI `$ref` schemas.

External-system operator metadata declarations also contribute operator-facing OpenAPI
operations for the generated metadata management surface:

- `Upsert<Entity>` as `PUT`
- `Get<Entity>` as `GET`
- `Disable<Entity>` as `POST .../disable`
- `Delete<Entity>` as `DELETE`

For tenant-scoped metadata, the generated path uses the service tenant field:

```text
/v1/tenants/{tenant_id}/operators/external-systems/{external_system_id}/profiles/{profile}
```

The generated request schema contains non-key, non-foundational metadata fields. Fields
listed in `required_fields` remain required; additional metadata fields are optional.
The generated response schema contains the full metadata entity field set.

## Generated Metadata

Binding generators consume the lowered StateSpec IR, not raw AST declarations.
Values, enums, events, external systems, shapes, feature flags, logs,
metrics, entities, queues, leases, API servers, workflows, and policies are emitted as descriptor
metadata where supported so downstream tools can inspect runtime contracts
consistently.

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
- optional metadata entity descriptor
- metadata tenant field
- metadata entity key fields
- metadata profile field
- required execution metadata fields
- descriptor-level source-to-target metadata field mappings
- generated mapping-plan helpers with ordered, client, and request assignment views
- generated mapping applicator contracts for turning mapping inputs into client
  configuration and request payloads
- generated root-aware source-value lookup helpers for mapping applicators
- generated missing-source diagnostics for incomplete mapping inputs
- generated transaction-scoped operator metadata repository contracts for OCC-backed
  upsert, get, disable, and delete operations
- generated lookup helpers that build runtime metadata lookup requests from descriptors
- generated transaction-scoped helpers that call external system metadata resolvers
- generated resolve helpers validate lookup key completeness before resolver calls

The generator smoke tests compile small in-memory metadata resolver fixtures for C++, Go,
Java, and Rust. Those fixtures verify the generated descriptor-to-lookup-to-transaction
resolver path and the incomplete-key short-circuit behavior.

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

Generated API server scaffolding also includes framework-neutral route descriptors,
request/response context records, and handler interfaces. Route descriptors join each
`api_server serves` target to the corresponding API method, path, input, output, and
error metadata. Concrete HTTP server loops and framework adapters remain downstream
runtime work.

Generated external-system operator metadata scaffolding includes API handler contracts
that bridge operator metadata API implementations to the transaction-scoped repository
contracts. These handlers return the same framework-neutral API response shape as other
generated API handlers.

Generated shape metadata includes:

- name
- typed fields

Generated bindings also include conservative target-language DTOs for shapes:

- C++ emits a `struct` per shape in `descriptors.hpp`.
- Go emits an exported `struct` per shape in `backend/descriptors.go`.
- Java emits a `record` per shape in `Descriptors.java`.
- Rust emits a `struct` per shape in `descriptors.rs`.

Initial DTO type mapping covers scalar primitives directly and represents custom,
temporal, UUID, JSON, collection, and reference types as strings. Descriptor metadata
continues to preserve the exact StateSpec type for downstream tools.

Descriptor field metadata is typed independently from generated DTO language types.
Generated `FieldDescriptor` values include:

- field name
- language-native `FieldType` enum value
- original StateSpec type name
- required flag

The shared compiler classifier maps grammar-level field types to `string`, `bool`,
`int`, `int32`, `int64`, `long`, `double`, `decimal`, `json`, `timestamp`, `duration`,
`uuid`, `named`, `list`, `set`, `map`, `optional`, and `reference`. Optional fields are
classified as `optional` and retain their exact source type, such as `int?`, in the type
name.

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

| Language | Values | Enums | Events | External systems | Shapes | APIs | API servers | Workers | Policies |
|---|---|---|---|---|---|---|---|---|---|
| C++ | `value_descriptors()` | `enum_descriptors()` | `event_descriptors()` | `external_system_descriptors()` | `shape_descriptors()` | `api_descriptors()` | `api_server_descriptors()`, `api_route_descriptors()` | `worker_descriptors()` | `policy_descriptors()` |
| Go | `ValueDescriptors()` | `EnumDescriptors()` | `EventDescriptors()` | `ExternalSystemDescriptors()` | `ShapeDescriptors()` | `ApiDescriptors()` | `ApiServerDescriptors()`, `ApiRouteDescriptors()` | `WorkerDescriptors()` | `PolicyDescriptors()` |
| Java | `valueDescriptors()` | `enumDescriptors()` | `eventDescriptors()` | `externalSystemDescriptors()` | `shapeDescriptors()` | `apiDescriptors()` | `apiServerDescriptors()`, `apiRouteDescriptors()` | `workerDescriptors()` | `policyDescriptors()` |
| Rust | `value_descriptors()` | `enum_descriptors()` | `event_descriptors()` | `external_system_descriptors()` | `shape_descriptors()` | `api_descriptors()` | `api_server_descriptors()`, `api_route_descriptors()` | `worker_descriptors()` | `policy_descriptors()` |

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
| C++ | `register_feature_flag_definitionsTx` | `register_queue_definitionsTx` | `register_lease_definitionsTx` | `register_runtime_catalogTx` |
| Go | `RegisterFeatureFlagDefinitionsTx` | `RegisterQueueDefinitionsTx` | `RegisterLeaseDefinitionsTx` | `RegisterRuntimeCatalogTx` |
| Java | `registerFeatureFlagDefinitionsTx` | `registerQueueDefinitionsTx` | `registerLeaseDefinitionsTx` | `registerRuntimeCatalogTx` |
| Rust | `register_feature_flag_definitions_tx` | `register_queue_definitions_tx` | `register_lease_definitions_tx` | `register_runtime_catalog_tx` |

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
