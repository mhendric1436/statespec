# StateSpec Quick Start

This guide shows how to build the CLI, validate the checked-in examples, generate
OpenAPI and language bindings, and understand the generated API app and Worker app
layout.

## Build The CLI

From the repository root:

```sh
make cli
```

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
- Route-to-handler dispatch.
- API handler interfaces.
- Typed request and response codecs for declared API input/output shapes.
- Default API action handlers that decode request shapes and encode accepted action responses.
- Operator metadata API handler interfaces for external-system metadata.
- Request and response context types shared by all API handlers.

User code owns:

- Real network transport selection.
- HTTP or RPC framework adapter.
- Authentication, authorization, and tenant resolution.
- Concrete API handler implementations.
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

When an entity declares terminal garbage collection metadata, generated common-tier GC
workers can be composed by API-only apps, Worker-only apps, or mixed API + Worker apps.
GC is enabled by default for standalone generated processes. In production mixed
deployments, disable GC on one tier using the generated API process or Worker runtime
config so only one tier performs background collection scans.

## Worker App Design

The generated Worker app is framework-neutral scaffolding for running declared workers,
queues, leases, and workflows.

Generated Worker code owns:

- Worker descriptors derived from `worker` declarations.
- Worker registry lookup helpers.
- Worker context records.
- Queue, lease, and workflow descriptor views.
- Workflow step handler interfaces.
- Worker handler interfaces.
- Workflow runner behavior for claim, keep alive, complete, fail, and retry-visible state.

User code owns:

- Queue polling or scheduler integration when needed.
- Concrete worker handlers.
- Concrete workflow step handlers.
- External API clients, retries, timeouts, and idempotency behavior.
- Concrete backend adapter implementing the generated OCC interfaces.
- Application composition root that connects descriptors, handlers, and backend stores.

Workflow step handlers should be idempotent. Any persisted state read that affects a
worker decision must happen through the generated backend transaction model, in the same
transaction as the write or workflow/queue operation that consumes the read.
Local Worker app tests may use the generated in-memory workflow, queue, lease, log, and
metric stores.

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

Generated files are disposable. Keep user-owned application code outside the generated
output tree and import or include the generated contracts.
