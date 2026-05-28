# Generated Extension Points

StateSpec generated bindings separate generated-owned application scaffolding from
user-owned implementation code. Generated files are safe to overwrite on every
`statespec generate bindings` run. User code should live outside the generated output
tree, or in application-owned packages that import the generated API, worker, and common
modules.

The `.sspec` file owns contracts, topology, descriptors, route metadata, worker metadata,
workflow step names, and backend catalog definitions. Generated code owns the local
composition and lifecycle scaffolding needed to run those contracts. Runtime
implementations own real network transport selection, HTTP/RPC framework adapters,
authentication, authorization integration, concrete backend adapters, remote API clients,
and business logic inside API and worker handlers.

Generated descriptor values are spec-driven. They report declarations from the
validated `.sspec` input, while unused descriptor domains are empty or absent according
to the target language's descriptor surface. Runtime artifacts are usage-pruned:
backend and transaction contracts remain available as generic OCC primitives, but typed
runtime stores, sinks, codecs, imports, module declarations, and app wiring are emitted
only when the spec or generated app needs them.

Generated facade modules are intended for users and top-level composition code that need
broad catalog access. Focused generated files should not depend on those facades when a
direct dependency exists. Per-API descriptor modules, per-worker descriptor modules,
entity repositories, API codecs, and handler implementations should import or include
the exact descriptor type, entity, shape, backend, or runtime module they use.

The `common`, `api`, and `worker` directory trees are deployment boundaries as well as
source layout boundaries. Common artifacts must be genuinely shared by API and Worker
tiers; API-only code stays in the API tree, and Worker-only code stays in the Worker
tree. Production application code should treat those boundaries as stable import
surfaces instead of reaching through a facade to avoid declaring the dependency it
actually needs.

For local generated app linking and handler tests, the generated common tier includes an
in-memory backend that implements the same OCC backend interfaces. See
[in-memory-backend.md](in-memory-backend.md).

## Ownership Boundary

Generated code owns:

- descriptor records for entities, shapes, APIs, API servers, workers, queues, leases,
  workflows, feature flags, logs, metrics, policies, and external systems
- DTOs for declared shapes
- API route descriptors and framework-neutral request/response context types
- API server shells, route dispatch helpers, handler registries, local composition roots,
  process lifecycle types, local blocking transports, and process entrypoints
- worker descriptors, worker contexts, worker application shells, worker runtimes, worker
  registry helpers, and process entrypoints
- workflow step handler context types, typed step-specific handler methods, and
  deterministic workflow step handler keys
- workflow runners that claim workflow steps, call user handlers, keep claimed steps
  alive, advance declared `transition_to` targets, and complete or fail steps through
  backend workflow stores
- transaction-scoped catalog registration helpers for queues, leases, workflows, feature
  flags, logs, and metrics when those runtime domains are used
- operator metadata lookup, default mapping applicators, external-system client call
  adapters, default generic repositories, and API handler contracts for external systems

User-owned code owns:

- concrete API handlers
- concrete workflow step handlers
- real network transports, HTTP/RPC server framework adapters, and request/response
  serialization
- worker and queue polling runtime adapters when the generated runtime is not sufficient
- backend implementations for the OCC interfaces
- external client construction, protocol selection, auth, retry policy, timeout policy,
  and remote API calls
- production composition adapters that replace generated default handlers or the
  generated in-memory backend

The generated composition root may use the generated in-memory backend and local
blocking API transport for local tests and examples. Production applications should
provide a durable backend adapter behind the same generated interfaces and choose a real
network transport adapter. StateSpec intentionally does not select an HTTP library yet;
that remains user/runtime-owned until the project adopts an opinionated HTTP backend.

## API Startup And Transport Ownership

Generated API apps own startup shape:

- construct backend, handler, process config, process runtime, and local transport
- bootstrap runtime catalogs through the generated app composition roots
- start the API process on an owned background thread, goroutine, or task
- install process stop handling where the language runtime supports it
- request stop and join the owned background execution during shutdown
- use the generated local/no-op transport to block and unblock lifecycle tests
- expose framework-neutral request dispatch through the generated `ApiServer.handle`
  boundary

The generated local transport exists to make generated applications runnable and
testable with production-shaped lifecycle. It owns only local blocking and unblocking:
`run()` waits until its stop signal is received, and the generated process stop method
delegates to the transport stop method so `join()` can complete. It does not open
sockets, bind ports, perform TLS, parse HTTP, or serialize framework responses. A
production adapter should implement the generated transport interface, translate the
chosen network protocol into the generated request context, call `ApiServer.handle`, and
translate the generated response back to the framework.

StateSpec will keep this boundary until it explicitly adopts an opinionated HTTP
backend. At that point the HTTP adapter can become another generated/runtime-owned
artifact without changing API handler contracts.

The generated `ApiProcess` lifecycle contract is documented in
[api-process-lifecycle.md](api-process-lifecycle.md). In short: `start()` owns
background execution, bootstrap runs inside the generated process execution path when
bootstrap-on-start is enabled, `request_stop()` is idempotent and safe before start,
`join()` waits for completion, and starting the same process twice must be rejected
deterministically.

If entity garbage collection metadata is present, generated API process config also
controls whether API-hosted GC workers are started. API-only deployments can leave this
enabled; mixed API + Worker deployments should disable it when Worker is the selected GC
host.

## External System Client Extension Point

External-system call adapters are generated as framework-neutral helpers. They apply the
generated metadata mapping plan, fail before invoking a remote system when required
mapping sources are missing, and pass a generic call request to an injected client.

| Language | Generated external-system client surface |
|---|---|
| C++ | `statespec_generated::IExternalSystemClient` |
| Go | `common.ExternalSystemClient` |
| Java | `Descriptors.ExternalSystemClient` |
| Rust | `descriptors::ExternalSystemClient` |

The injected client owns protocol-specific behavior: HTTP or RPC transport, URL and
header construction, authentication, retries, timeouts, response decoding, and
circuit-breaker behavior.

## API Handler Extension Point

API handlers receive a framework-neutral request context and return a generated API
response. The generated dispatcher maps an API server route name such as
`ProvisionApi.StartProvision` to the declared operation-specific handler method.
Framework adapters should translate HTTP requests into the generated request context,
call the generated API server dispatch boundary, and translate the response back to the
selected HTTP framework. The generated local blocking transport is only a lifecycle
placeholder; real network transport selection is user/runtime-owned.

| Language | Generated API handler surface |
|---|---|
| C++ | `statespec_generated::api::IApiOperationHandler` in `api/api_handlers.hpp` |
| Go | `api/backend.APITierHandler` |
| Java | `ApiHandlers.Handler` |
| Rust | `api_handlers::ApiHandler` |

API handlers may start workflows, enqueue messages, resolve operator metadata, or read
entities, but any persisted state access must use the generated backend/OCC transaction
model. Handler code should not bypass `IBackend` and `ITransaction` or the equivalent
language bindings for direct store access.

Generated default entity API handlers use generated entity repositories for create,
read, list, status update, and soft-delete operations. Repositories own collection names,
descriptor registration, key encoding, declared index names, and index-value ordering.
List handlers call named repository helpers derived from declared indexes, such as
`listByAccountStatusTx`, `ListByAccountStatusTx`, or `list_by_account_status_tx`,
rather than embedding raw index names in API code. The repository then translates the
typed generated surface into generic OCC backend queries within the caller-managed
transaction.

Entity-owned CRUD APIs always generate default handlers. The source of truth is IR
metadata, not method-name conventions: an API with both `api.entity` and
`api.repository_operation` set is a generated entity lifecycle operation. Generators
must emit concrete create, get, list, status update, and delete behavior for those APIs.
Manual user handler implementations remain the extension point for top-level business
APIs that do not have entity repository metadata.

The generated handler registry composes these two paths. Entity CRUD operations
delegate to generated repository-backed handler modules. Top-level business APIs
delegate to a user-supplied business handler interface:

| Language | Business API handler surface |
|---|---|
| C++ | `statespec_generated::api::IBusinessApiOperationHandler` |
| Go | `api/backend.BusinessAPITierHandler` |
| Java | `ApiHandlers.BusinessHandler` |
| Rust | `api_handlers::BusinessApiHandler` |

If no business handler is supplied, generated default handlers return `501` for manual
business APIs. This keeps generated application shells linkable without pretending to
implement domain-specific behavior.

Generated constants are the canonical spelling source for entity metadata in binding
output. Entity names, field names, state values, and index names are emitted once and
then reused by descriptors, repository helpers, generated API handlers, GC metadata, and
runtime helpers. User extension code should prefer those constants over repeating raw
strings when it interacts with generated repositories or descriptor values.

User extension code should not create alternate entity key encodings, collection names,
or document field layouts. Cross-language API app persistence compatibility depends on
generated repositories owning those details consistently for C++, Go, Java, and Rust
bindings produced from the same `.sspec` file.

Typed operation handlers are the canonical API extension point. StateSpec does not
generate a parallel generic API handler path; each declared API operation maps to one
generated handler method and one dispatcher branch.

Generated API codec files provide the canonical conversion between the framework-neutral
`ApiRequestContext`/`ApiResponse` JSON bodies and declared API input/output shape types:

| Language | Generated API codec artifact |
|---|---|
| C++ | `api/api_codecs.hpp` |
| Go | `api/backend/api_codecs.go` |
| Java | `ApiCodecs.java` |
| Rust | `api_codecs.rs` |

The generated default API handler uses these codecs for entity-owned CRUD operations.
For top-level business APIs with a primary action model such as `starts workflow` or
`enqueues`, users provide the business handler implementation that performs
domain-specific persistence, authorization, and runtime side effects.

When testing API handlers locally, inject the generated in-memory backend through the
same application-owned dependency path that production code uses for its durable
backend.

## Operator Metadata API Handlers

External-system operator metadata APIs use a separate handler contract because they bridge
operator requests to transaction-scoped metadata repository calls. These handlers receive
the caller-managed transaction, the generated repository contract, and the generated
request object for upsert, get, disable, or delete.

| Language | Generated operator metadata handler surface |
|---|---|
| C++ | `statespec_generated::IExternalSystemOperatorMetadataApiHandler` |
| Go | `common.ExternalSystemOperatorMetadataAPIHandler` |
| Java | `Descriptors.ExternalSystemOperatorMetadataApiHandler` |
| Rust | `descriptors::ExternalSystemOperatorMetadataApiHandler` |

The handler should enforce operator-specific policy and response shaping. The generated
default repository persists metadata through generic OCC collection/document operations
on the transaction passed into the handler, including expected-version checks for
operator updates. Production adapters may wrap or replace the generated repository for
extra policy or credential-manager integration, but backend implementations should remain
unaware of external-system metadata concepts.

## Worker Runtime Extension Point

Worker runtimes are generated for declared `worker` metadata. They own local runtime
composition for backend stores, definition registration, GC worker registration, and
one-step workflow execution through generated workflow runners. `WorkerProcess` owns the
background lifecycle: bootstrap, Worker-hosted GC startup, generated workflow polling
loops, stop, and join. StateSpec no longer generates a parallel coarse worker handler
interface.

| Language | Generated worker runtime surface |
|---|---|
| C++ | `statespec_generated::worker::WorkerRuntime` |
| Go | `worker/backend.WorkerTierRuntime` |
| Java | `WorkerRuntime` |
| Rust | `worker_runtime::WorkerRuntime` |

Use workflow step handlers for business logic. Production adapters may wrap or replace
the generated process entrypoint, but persisted StateSpec resources must still be
accessed through generated backend transaction interfaces.

Worker runtime config controls whether Worker-hosted entity GC workers are started.
Worker-only deployments can leave this enabled; mixed deployments should enable GC on
only one tier.

See [worker-process-lifecycle.md](worker-process-lifecycle.md) for the cross-language
Worker process contract.

## Workflow Step Handler Extension Point

Workflow step handlers are the primary business-logic extension point for generated
worker applications. The generated workflow runner owns the workflow store interaction:
claiming a step, keeping the claim alive, invoking the generated step-specific user
handler method, advancing to a declared `transition_to` target when the step succeeds,
completing terminal steps, and failing the step when the handler returns an error.

| Language | Generated workflow step handler surface |
|---|---|
| C++ | `statespec_generated::worker::IWorkflowStepHandler` in `worker/workflow_step_handlers.hpp` |
| Go | `worker/backend.WorkflowStepHandler` |
| Java | `WorkflowStepHandlers.Handler` |
| Rust | `worker::workflow_step_handlers::WorkflowStepHandler` |

Each declared workflow step maps to one generated handler method. For example,
`workflow ProvisionService` step `validate_request` maps to:

- C++ `handle_provision_service_validate_request`
- Go `HandleProvisionServiceValidateRequest`
- Java `handleProvisionServiceValidateRequest`
- Rust `handle_provision_service_validate_request`

StateSpec does not generate a parallel generic workflow step handler path. The generated
context includes the workflow name, workflow version, step name, optional execution ID,
and input payload. Generated `workflow_step_handler_keys` helpers list the valid handler
keys in deterministic `WorkflowName.step_name` form. Application code can use those keys
to register concrete step handlers and fail fast when a declared workflow step has no
implementation.

The generated runner derives workflow advancement from the `.sspec` workflow body. A
step with `transition_to next_step` completes the claimed step with `next_step` set to
that target, leaving the workflow execution `Running`. A step with no transition target
completes the workflow execution.

Workflow step handlers should be idempotent. A handler may be retried after process
restart, lease expiry, or backend failover. External calls should use idempotency keys
from entity or workflow state, and entity mutations should happen through OCC-backed
transactions.

Generated workflow runners can be linked with the in-memory workflow, queue, lease, log,
and metric stores for deterministic local tests. This verifies the generated wiring and
handler contracts, not production backend durability.

## Recommended Application Layout

Generated output should be treated as a dependency, not as the place where application
code is edited:

```text
generated/
  cpp|go|java|rust/
    common/
    api/
    worker/

app/
  api/
    handlers.*
    http_adapter.*
    main.*
  worker/
    workflow_step_handlers.*
    worker_runtime.*
    main.*
  backend/
    backend_adapter.*
```

The exact application directory names are repo-owned. The important rule is that
generated files remain disposable and user-owned implementations import or include the
generated surfaces.

## Regeneration Contract

After a `.sspec` change, regenerate bindings and rebuild application code. Handler
implementations should fail compilation when a required generated contract changes. For
workflow steps, compare the generated workflow step handler key list against the
application registry so missing implementations are caught during startup or tests.

Specs remain the source of truth. User-owned extension code implements behavior behind
the generated contracts; it should not reinterpret `.sspec` semantics independently.
