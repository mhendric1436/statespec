# Generated Extension Points

StateSpec generated bindings separate generated-owned application scaffolding from
user-owned implementation code. Generated files are safe to overwrite on every
`statespec generate bindings` run. User code should live outside the generated output
tree, or in application-owned packages that import the generated API, worker, and common
modules.

The `.sspec` file owns contracts, topology, descriptors, route metadata, worker metadata,
workflow step names, and backend catalog definitions. Runtime implementations own HTTP
framework adapters, authentication, authorization integration, concrete backend
adapters, remote API clients, and business logic inside API and worker handlers.

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
  and process entrypoints
- worker descriptors, worker contexts, worker application shells, worker runtimes, worker
  registry helpers, and process entrypoints
- workflow step handler context types, typed step-specific handler methods, and
  deterministic workflow step handler keys
- workflow runners that claim workflow steps, call user handlers, keep claimed steps
  alive, advance declared `transition_to` targets, and complete or fail steps through
  backend workflow stores
- transaction-scoped catalog registration helpers for queues, leases, workflows, feature
  flags, logs, and metrics
- operator metadata lookup, mapping, repository, and API handler contracts for external
  systems

User-owned code owns:

- concrete API handlers
- concrete workflow step handlers
- HTTP server framework adapters and request/response serialization
- worker and queue polling runtime adapters when the generated runtime is not sufficient
- backend implementations for the OCC interfaces
- external client construction, auth, retry policy, timeout policy, and remote API calls
- production composition adapters that replace generated default handlers or the
  generated in-memory backend

The composition root may use the generated in-memory backend for local tests and
examples. Production applications should provide a durable backend adapter behind the
same generated interfaces.

## API Handler Extension Point

API handlers receive a framework-neutral request context and return a generated API
response. The generated dispatcher maps an API server route name such as
`ProvisionApi.StartProvision` to the declared operation-specific handler method.
Framework adapters should translate HTTP requests into the generated request context,
call the typed handler, and translate the response back to the selected HTTP framework.

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

The handler should enforce operator-specific policy and response shaping. The repository
implementation remains backend-owned and must enforce optimistic concurrency through the
transaction passed into the handler.

## Worker Runtime Extension Point

Worker runtimes are generated for declared `worker` metadata. They own local runtime
composition for backend stores, definition registration, and one-step workflow execution
through generated workflow runners. StateSpec no longer generates a parallel coarse
worker handler interface.

| Language | Generated worker runtime surface |
|---|---|
| C++ | `statespec_generated::worker::WorkerRuntime` |
| Go | `worker/backend.WorkerTierRuntime` |
| Java | `WorkerRuntime` |
| Rust | `worker_runtime::WorkerRuntime` |

Use workflow step handlers for business logic. Production adapters may wrap or replace
the generated runtime loop, but persisted StateSpec resources must still be accessed
through generated backend transaction interfaces.

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
