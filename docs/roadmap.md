# Roadmap

This roadmap tracks implementation state for the current compiler and generator pass.
The compiler owns syntax, validation, semantic lowering, IR, descriptors, and generated
contracts. Runtime adapters own concrete storage, transport, authentication, and remote
execution.

## Current Pass: Generated API And Worker Runtime Baseline

The current pass has moved beyond descriptor-only generation. StateSpec now generates
production-shaped API and Worker application skeletons while keeping concrete adapters,
business behavior, and external side effects runtime-owned.

Complete:

- Generate C++, Go, Java, and Rust API composition roots, process lifecycle types,
  process entrypoints, local/no-op blocking transports, request/response contexts, and
  map-based route plus handler dispatch.
- Lower entity-owned CRUD intent into deterministic API contracts, request/response
  shapes, route metadata, generated codecs, backend-owned CRUD handlers, and OpenAPI
  operations.
- Generate Worker composition roots, WorkerProcess lifecycle types, process
  entrypoints, workflow runners, per-workflow handler contracts, per-workflow registries,
  and composite workflow-step dispatch keys.
- Execute workflow steps through a two-boundary OCC pattern: claim in a short
  transaction, run the handler and finalization in a second transaction, and perform
  periodic claim-token keepalive while the handler is active.
- Apply handler-returned workflow results directly through generated complete, fail, and
  cancel paths; runner-side next-step inference chains have been removed.
- Generate entity GC descriptors, per-tier GC catalogs, and API/Worker lifecycle wiring
  so GC can run in API-only, Worker-only, or mixed deployments.
- Parse `external_system metadata` declarations, including `entity`, `profile_field`,
  `required_fields`, and `mappings`.
- Validate metadata entity references, tenant/key/profile fields, required metadata
  fields, unique key indexes, authoritative ownership, and the canonical
  `Active`/`Disabled`/`Deleted` lifecycle.
- Validate mapping roots, mapping path shape, unique targets, and required
  `metadata.*` sources.
- Lower metadata contracts, mappings, APIs, API servers, and route metadata into IR.
- Generate C++, Go, Java, and Rust descriptors for external systems, mappings, APIs,
  API servers, and route metadata.
- Generate mapping-plan helpers, mapping applicator contracts, missing-source
  diagnostics, metadata lookup helpers, and transaction-scoped resolver helpers.
- Generate OCC repository contracts and operator metadata API handler contracts for
  metadata upsert, get, disable, and delete flows.
- Generate OpenAPI for declared APIs and operator metadata API surfaces.
- Maintain an end-to-end regression fixture at
  `testdata/generators/external-system-metadata-e2e.sspec`.
- Maintain cross-language generated-app fixtures for API-only, workflow/worker, GC, CRUD,
  in-memory backend registration, schema compatibility, and long-running workflow
  keepalive behavior.

Partial:

- OpenAPI schema lowering emits primitives, values, enums, shape references, recursive
  list/set/map collections, and default error responses. Remaining OpenAPI work is auth
  metadata, richer constraints, examples, and optional framework adapter integration.
- Formatter support is token-preserving. Canonical-order warnings exist, but formatter
  ownership is now the next architectural pass and must move canonical ordering into
  AST-owned rewriting for high-value nested forms.
- Policies are emitted as descriptors, but enforcement hooks are not generated.
- Workflow statements, child sets, and nested workflow blocks lower into IR, but
  generated business workflow bodies remain user-owned until statement execution
  semantics, idempotency contracts, and expression typing are stronger.
- Queue descriptors and runtime stores exist, but generated queue-worker business
  execution remains intentionally thin.

Runtime-owned by design:

- Metadata persistence implementations behind generated repository contracts.
- OCC transaction lifecycle, retry policy, and commit handling.
- Real network transport selection.
- HTTP framework route registration, request decoding, response encoding, and middleware
  integration.
- Authentication and authorization enforcement for operator APIs.
- Secret lookup for credential references.
- Concrete external-system clients, wire serialization, timeout behavior, retry logic,
  circuit breakers, and service mesh integration.
- Worker event transport, queue-worker business bodies, remote-call execution, and
  idempotent external side effects.
- Production process supervisors and deployment-specific health/readiness integration.

Generated-owned today:

- API and Worker composition roots and process entrypoints.
- API process lifecycle contract, bootstrap, owned background execution, stop handling,
  join semantics, and local/no-op blocking transport.
- Worker process lifecycle contract, owned background execution, stop handling, join
  semantics, and generated workflow polling/runner wiring.
- Map-based API route lookup and API-name-to-handler dispatch.
- Entity CRUD handlers for generated entity-owned lifecycle APIs.
- Workflow step dispatch maps, claim-token validation, periodic keepalive, and
  result-driven complete/fail/cancel finalization.

StateSpec intentionally stops short of choosing an HTTP library. The local API transport
exists so generated apps start and block like production services, but it does not bind a
socket. A real HTTP transport remains user/runtime-owned until StateSpec adopts an
opinionated HTTP backend.

## Next Priorities

1. Make formatter ownership the next architectural pass. Move formatter behavior from
   token-preserving output toward AST-owned canonical declaration ordering for entity
   `ownership`, `relations`, `children`, and `invariants`, then key nested API and
   workflow forms.
2. Keep parity and roadmap docs synchronized with generated API/Worker runtime changes.
3. Split remaining generator artifact orchestration files so path selection, manifests,
   template data, and source rendering stay modular.
4. Expand OpenAPI output for `value`, `enum`, nested collection, and error schemas.
5. Add generated policy enforcement hooks once expression type checking is stronger.
6. Add optional HTTP adapter generation after route, auth, lifecycle, and error contracts
   stabilize.
7. Expand generated queue-worker execution and workflow body generation after workflow
   statement semantics are fully typed and idempotency guidance is enforceable.
8. Add backend conformance tests for metadata repository implementations.

## Next Architectural Pass: Formatter Ownership

The formatter should become the owner of the written `.sspec` shape for constructs whose
canonical order is already enforced by validation warnings. The next pass should keep
semantic behavior unchanged and replace token-preserving best effort formatting with
AST-owned reconstruction for the most important nested forms.

Scope:

- Entity member order: `key`, `ownership`, `version`, `fields`, `state_machine`,
  `relations`, `children`, `invariants`, `indexes`, and `annotations`.
- Entity `ownership` property order: `authority`, `system_of_record`, and `lifecycle`.
- Entity `relations`, `children`, and `invariants` block ordering and indentation.
- Entity-owned API blocks: `resource`, `create`, `get`, `list`, `update_status`, and
  `delete`, with stable nested `fields`, `path`, and `by` ordering.
- Workflow blocks: metadata first, then `on`, `input`, `state`, `load`, `child_set`,
  and `step`; step bodies should format `require` before behavior statements.

Out of scope for this pass:

- Semantic rewrites, inferred declarations, migration tooling, or user-configurable
  style options.
- Typed expression formatting beyond preserving deterministic whitespace and indentation.
- New syntax for APIs, workflows, relations, children, or invariants.

Acceptance criteria:

- `statespec fmt` produces a stable fixed point for the targeted forms.
- Formatter tests include intentionally scrambled entity/API/workflow examples.
- Validator canonical-order warnings agree with formatter output.
- Examples and generator fixtures remain formatted by the canonical formatter.
