# StateSpec Quick Start

This guide shows how to build the CLI, validate the checked-in examples, generate
OpenAPI and language bindings, and understand the generated API app and Worker app
layout.

## Build The CLI

From the repository root:

```sh
make cli-fast
```

`make cli-fast` builds the CLI with parallel jobs based on the host logical CPU
count. Use `make cli` as the portable fallback if the fast target is unavailable
or if you want a single-job build.

The CLI is written to:

```text
build/bin/statespec
```

Use this command shape throughout the examples:

```sh
./build/bin/statespec <command>
```

## Validate The Examples

Validate the main examples:

```sh
./build/bin/statespec validate examples/order-system.sspec
./build/bin/statespec validate examples/feature-flags.sspec
./build/bin/statespec validate examples/external-system-metadata.sspec
./build/bin/statespec validate examples/workflow-launch-control.sspec
./build/bin/statespec validate examples/order-system-with-launch-control.sspec
```

Validate the generated app regression fixtures:

```sh
./build/bin/statespec validate testdata/generators/canonical-api-worker-app.sspec
./build/bin/statespec validate testdata/generators/external-system-metadata-e2e.sspec
```

Successful validation prints:

```text
valid
```

## Generate Bindings For One Example

Generate all supported language bindings for `examples/order-system.sspec`:

```sh
./build/bin/statespec generate bindings --lang cpp examples/order-system.sspec --out build/generated/order-system/cpp
./build/bin/statespec generate bindings --lang go examples/order-system.sspec --out build/generated/order-system/go
./build/bin/statespec generate bindings --lang java examples/order-system.sspec --out build/generated/order-system/java
./build/bin/statespec generate bindings --lang rust examples/order-system.sspec --out build/generated/order-system/rust
```

Generate OpenAPI:

```sh
./build/bin/statespec generate openapi examples/order-system.sspec --out build/generated/order-system/openapi
```

OpenAPI output is written to:

```text
build/generated/order-system/openapi/openapi.json
```

## Generate Entity-Owned CRUD APIs

Canonical CRUD APIs are declared on entities. The `examples/api-entities-only.sspec`
example shows `Account`, `Project`, and `Task` entities deriving create, get, list,
status update, and delete operations from entity `api` blocks. Top-level `api`
declarations should be used for business actions that are not standard entity lifecycle
operations.

Generate bindings and OpenAPI for the entity CRUD example:

```sh
./build/bin/statespec generate bindings --lang cpp examples/api-entities-only.sspec --out build/generated/api-entities-only/cpp
./build/bin/statespec generate bindings --lang go examples/api-entities-only.sspec --out build/generated/api-entities-only/go
./build/bin/statespec generate bindings --lang java examples/api-entities-only.sspec --out build/generated/api-entities-only/java
./build/bin/statespec generate bindings --lang rust examples/api-entities-only.sspec --out build/generated/api-entities-only/rust
./build/bin/statespec generate openapi examples/api-entities-only.sspec --out build/generated/api-entities-only/openapi
```

## Generate The Other Examples

Feature flag example:

```sh
./build/bin/statespec generate bindings --lang cpp examples/feature-flags.sspec --out build/generated/feature-flags/cpp
./build/bin/statespec generate bindings --lang go examples/feature-flags.sspec --out build/generated/feature-flags/go
./build/bin/statespec generate bindings --lang java examples/feature-flags.sspec --out build/generated/feature-flags/java
./build/bin/statespec generate bindings --lang rust examples/feature-flags.sspec --out build/generated/feature-flags/rust
```

External-system metadata example:

```sh
./build/bin/statespec generate bindings --lang cpp examples/external-system-metadata.sspec --out build/generated/external-system-metadata/cpp
./build/bin/statespec generate bindings --lang go examples/external-system-metadata.sspec --out build/generated/external-system-metadata/go
./build/bin/statespec generate bindings --lang java examples/external-system-metadata.sspec --out build/generated/external-system-metadata/java
./build/bin/statespec generate bindings --lang rust examples/external-system-metadata.sspec --out build/generated/external-system-metadata/rust
./build/bin/statespec generate openapi examples/external-system-metadata.sspec --out build/generated/external-system-metadata/openapi
```

Workflow launch control example:

```sh
./build/bin/statespec generate bindings --lang cpp examples/workflow-launch-control.sspec --out build/generated/workflow-launch-control/cpp
./build/bin/statespec generate bindings --lang go examples/workflow-launch-control.sspec --out build/generated/workflow-launch-control/go
./build/bin/statespec generate bindings --lang java examples/workflow-launch-control.sspec --out build/generated/workflow-launch-control/java
./build/bin/statespec generate bindings --lang rust examples/workflow-launch-control.sspec --out build/generated/workflow-launch-control/rust
```

Order system composed with workflow launch control:

```sh
./build/bin/statespec generate bindings --lang cpp examples/order-system-with-launch-control.sspec --out build/generated/order-system-with-launch-control/cpp
./build/bin/statespec generate bindings --lang go examples/order-system-with-launch-control.sspec --out build/generated/order-system-with-launch-control/go
./build/bin/statespec generate bindings --lang java examples/order-system-with-launch-control.sspec --out build/generated/order-system-with-launch-control/java
./build/bin/statespec generate bindings --lang rust examples/order-system-with-launch-control.sspec --out build/generated/order-system-with-launch-control/rust
./build/bin/statespec generate openapi examples/order-system-with-launch-control.sspec --out build/generated/order-system-with-launch-control/openapi
```

Complete generated API + Worker app fixture:

```sh
./build/bin/statespec generate bindings --lang cpp testdata/generators/canonical-api-worker-app.sspec --out build/generated/canonical-api-worker-app/cpp
./build/bin/statespec generate bindings --lang go testdata/generators/canonical-api-worker-app.sspec --out build/generated/canonical-api-worker-app/go
./build/bin/statespec generate bindings --lang java testdata/generators/canonical-api-worker-app.sspec --out build/generated/canonical-api-worker-app/java
./build/bin/statespec generate bindings --lang rust testdata/generators/canonical-api-worker-app.sspec --out build/generated/canonical-api-worker-app/rust
```

## Generate A Specific Tier

Binding generation emits all tiers by default. Use `--tier` when you want only the
shared runtime surface, the API app surface, or the Worker app surface:

```sh
./build/bin/statespec generate bindings --lang cpp examples/order-system.sspec --tier common --out build/generated/order-system/cpp-common
./build/bin/statespec generate bindings --lang cpp examples/order-system.sspec --tier api --out build/generated/order-system/cpp-api
./build/bin/statespec generate bindings --lang cpp examples/order-system.sspec --tier worker --out build/generated/order-system/cpp-worker
```

Tier meanings:

| Tier | Output |
|---|---|
| `common` | Shared descriptors, backend abstractions, runtime contracts, and generated build files. |
| `api` | `common` plus API server, route, dispatcher, handler, and operator metadata API files. |
| `worker` | `common` plus worker registry, worker contexts, worker handlers, workflow step handlers, and workflow runner files. |
| `all` | `common`, `api`, and `worker`. |

## Build Generated Output

Generated binding outputs include local build files. After generation, run:

```sh
make -C build/generated/order-system/cpp check
make -C build/generated/order-system/go check
make -C build/generated/order-system/java check
make -C build/generated/order-system/rust check
```

These checks compile the generated package surfaces. They do not implement business
logic or connect to a real backend.

## Link Generated Apps With The In-Memory Backend

Generated bindings include an in-memory backend in the `common` tier for C++, Go, Java,
and Rust. It implements the generated OCC backend, transaction, feature flag, queue,
lease, workflow, log, and metric interfaces so API and Worker app shells can link in
local tests without a production backend adapter.

Generate the complete API + Worker fixture and compile the generated surfaces:

```sh
./build/bin/statespec generate bindings --lang cpp testdata/generators/canonical-api-worker-app.sspec --out build/generated/canonical-api-worker-app/cpp
./build/bin/statespec generate bindings --lang go testdata/generators/canonical-api-worker-app.sspec --out build/generated/canonical-api-worker-app/go
./build/bin/statespec generate bindings --lang java testdata/generators/canonical-api-worker-app.sspec --out build/generated/canonical-api-worker-app/java
./build/bin/statespec generate bindings --lang rust testdata/generators/canonical-api-worker-app.sspec --out build/generated/canonical-api-worker-app/rust

make -C build/generated/canonical-api-worker-app/cpp check
make -C build/generated/canonical-api-worker-app/go check
make -C build/generated/canonical-api-worker-app/java check
make -C build/generated/canonical-api-worker-app/rust check
```

Use one in-memory backend instance in the application composition root when an API app
and Worker app need to observe the same local state. The in-memory backend is intended
for generated app linking, examples, and deterministic tests; production deployments
should replace it with a durable backend adapter.

The repository regression target for generated API and Worker app linking is:

```sh
make test-generated-apps
```

That target delegates to language-local E2E test directories:

```text
tests/bindings/e2e/
  cpp/
  go/
  java/
  rust/
```

Each language directory owns its generated app regression script and any linking fixtures
needed by that language. Run one language directly when working on a single binding:

```sh
make -C tests/bindings/e2e/cpp test-cli CLI="$(pwd)/build/bin/statespec"
make -C tests/bindings/e2e/go test-cli CLI="$(pwd)/build/bin/statespec"
make -C tests/bindings/e2e/java test-cli CLI="$(pwd)/build/bin/statespec"
make -C tests/bindings/e2e/rust test-cli CLI="$(pwd)/build/bin/statespec"
```

The aggregate E2E Makefile also supports formatting checks across the language fixtures:

```sh
make -C tests/bindings/e2e format-check
```

For the backend contract and per-language paths, see
[in-memory-backend.md](in-memory-backend.md).

## API App Design

The generated API app is framework-neutral scaffolding for serving declared `api`
blocks from declared `api_server` blocks.

Generated API code owns:

- API server descriptors and lifecycle shell.
- API process config, threading, lifecycle, bootstrap, stop, join, process entrypoint,
  and local composition wiring.
- A local/no-op blocking transport that starts successfully, blocks until shutdown, and
  unblocks when the generated process requests stop.
- Route descriptors derived from `api_server serves` and declared API method/path metadata.
- Map-based route lookup and API-name-to-handler dispatch.
- Generated backend-owned CRUD handler invokers for entity-owned APIs.
- Business API handler interfaces for top-level non-CRUD APIs.
- Typed request and response codecs for declared API input/output shapes.
- Default API action handlers that decode request shapes and encode accepted action responses.
- Operator metadata API handler interfaces for external-system metadata.
- Request and response context types shared by all API handlers.

User code owns:

- Real network transport selection.
- HTTP or RPC framework adapter.
- Authentication, authorization, and tenant resolution.
- Concrete business API handler implementations for top-level non-CRUD APIs.
- Transport-level request deserialization and response serialization for the selected framework.
- Concrete backend adapter implementing the generated OCC interfaces.
- External clients and runtime configuration.

The generated API process constructs the backend, handler, config, local blocking
transport, and API app composition roots. The generated `ApiProcess` owns the background
thread, goroutine, or task: generated `main` starts the process, installs stop handling,
then joins it for completion. This gives generated apps a production-shaped lifecycle
without choosing an HTTP library. The generated local transport is intentionally no-op:
it owns only lifecycle blocking and unblocking, and it does not bind a port or parse
network requests. The cross-language lifecycle contract is documented in
[api-process-lifecycle.md](api-process-lifecycle.md).

A production API adapter should implement the generated transport interface for the
chosen framework. It should translate a network request into a generated request
context, call the generated `ApiServer.handle` dispatch boundary, and translate the
generated response back to the transport. Until StateSpec adopts an opinionated HTTP
backend, real network transport selection remains user/runtime-owned. Local tests may
use the generated in-memory backend as the concrete backend adapter.

Entity-owned CRUD APIs do not require user handler code. Generated per-entity API
modules register CRUD invokers with the top-level handler registry, and those invokers
operate through the generated backend/OCC repositories. Manual top-level business APIs
remain user-owned: the dispatcher uses a separate business handler map and returns a
generated `501` response when no business handler is supplied.

When an entity declares terminal garbage collection metadata, generated common-tier GC
workers can be composed by API-only apps, Worker-only apps, or mixed API + Worker apps.
The generated API and Worker tiers each bind workers through their own GC descriptor
catalog; there is no common global catalog. GC is enabled by default for standalone
generated processes. In production mixed deployments, disable GC on one tier using the
generated API process or Worker runtime config so only one tier performs background
collection scans.

## Worker App Design

The generated Worker app is framework-neutral scaffolding for running declared workers,
queues, leases, and workflows.

Generated Worker code owns:

- Worker process lifecycle: `start`, background workflow loops, `request_stop`, and
  `join`.
- Worker descriptors derived from `worker` declarations.
- Worker registry lookup helpers.
- Worker context records.
- Queue, lease, and workflow descriptor views.
- Workflow step handler interfaces.
- Default workflow-specific step handlers so generated apps link until user code
  supplies production handlers.
- Workflow runner behavior for claim, claim-token validation, heartbeat-backed keep
  alive, handler-result dispatch,
  complete/fail/cancel, and retry-visible state.
- Entity GC worker startup and shutdown when Worker-hosted GC is enabled.

User code owns:

- Concrete workflow step handlers.
- External API clients, retries, timeouts, and idempotency behavior.
- Concrete backend adapter implementing the generated OCC interfaces.
- Concrete deployment and supervisor integration around generated `main`.

Workflow step handlers should be idempotent. Any persisted state read that affects a
worker decision must happen through the generated backend transaction model. Generated
workflow runners claim a step in a separate committed transaction, then execute the
handler and apply complete/fail/cancel in a second transaction. Handler-owned persisted
writes and workflow advancement should commit together through that second transaction.
Remote work performed by a handler must be idempotent because it may be retried after a
crash, lease expiry, or OCC conflict.
Handlers also own step advancement decisions. A generated handler method returns a
workflow step result: complete with an optional next step, fail with a reason, or cancel
with a reason. The runner applies that result directly and does not use a separate
generated next-step calculator.
Local Worker app tests may use the generated in-memory workflow, queue, lease, log, and
metric stores.

The generated Worker process lifecycle is documented in
[worker-process-lifecycle.md](worker-process-lifecycle.md).

## Generated Layout By Language

Typical generated output roots:

```text
build/generated/<example>/cpp/
  common/
    memory/
    runtime/
  api/
  worker/
  Makefile

build/generated/<example>/go/
  common/backend/
    memory/
    runtime/
  api/backend/
  worker/backend/
  go.mod
  Makefile

build/generated/<example>/java/
  common/com/statespec/backend/
    memory/
    runtime/
  common/com/statespec/generated/
  api/com/statespec/generated/
  worker/com/statespec/generated/
  Makefile

build/generated/<example>/rust/
  common/
    memory/
    runtime/
  api/
  worker/
  Cargo.toml
  lib.rs
  Makefile
```

The `memory/` paths contain concrete in-memory backend and transaction adapters. The
`runtime/` paths contain backend-neutral stores, sinks, and codecs for feature flags,
queues, leases, workflows, logs, and metrics.

Facade files such as `common/descriptors.*`, `api/api_descriptors.*`, and
`worker/worker_descriptors.*` are convenience surfaces for users and generated
composition roots. Focused generated files use direct imports/includes for the specific
descriptor types, entity modules, shape modules, backend contracts, and runtime clients
they need. This keeps common/API/Worker tier boundaries explicit and avoids making
small generated modules depend on large aggregate facades.

Generated files are disposable. Keep user-owned application code outside the generated
output tree and import or include the generated contracts.
