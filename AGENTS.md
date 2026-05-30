# AGENTS.md

## Project: StateSpec

StateSpec is a canonical, text-first software design language for distributed systems. It defines entities, state machines, APIs, workflows, ownership boundaries, parent-child relationships, and orchestration semantics in a deterministic specification format.

The project goal is to make software design explicit, verifiable, generatable, and editable through both text and IDE-assisted visual tooling.

StateSpec source files use the `.sspec` extension.

---

## Core Product Principles

1. **Text is the source of truth**
   - `.sspec` files are canonical.
   - Visual editors, diagrams, and generated artifacts must derive from text.
   - No secondary visual model may become authoritative.

2. **Low ambiguity by design**
   - Prefer explicit syntax over inference.
   - Avoid aliases for language keywords.
   - Avoid optional semantics for critical behavior.
   - Prefer one canonical way to express each concept.

3. **Formatter defines the written language**
   - The formatter must produce stable, deterministic output.
   - Field ordering, indentation, spacing, and block structure should be canonical.
   - Generated examples should always be formatted.

4. **Compiler core owns semantics**
   - Parser, semantic analyzer, IR, formatter, CLI, LSP, and generators must agree on one semantic model.
   - Do not duplicate semantic rules independently across CLI, VS Code extension, or generators.

5. **Generated code is downstream**
   - Generated OpenAPI, API app scaffolds, worker app scaffolds, language bindings, diagrams, and tests are outputs of the spec.
   - Generators consume canonical IR, not raw AST.

---

## Current Repository Structure

```text
statespec/
├── README.md
├── AGENTS.md
├── Makefile
├── grammar/
│   └── statespec.ebnf
├── include/statespec/
│   ├── ast.hpp
│   ├── parser.hpp
│   ├── validator.hpp
│   ├── ir.hpp
│   ├── formatter.hpp
│   ├── generator*.hpp
│   └── template_renderer.hpp
├── src/
│   ├── parser_*.cpp
│   ├── validator_*.cpp
│   ├── ir.cpp
│   ├── formatter.cpp
│   ├── generator_cpp.cpp
│   ├── generator_go.cpp
│   ├── generator_java.cpp
│   ├── generator_rust.cpp
│   ├── generator_openapi.cpp
│   └── template_renderer.cpp
├── cmd/
│   └── statespec.cpp
├── bindings/
│   ├── cpp/
│   ├── go/
│   ├── java/
│   └── rust/
├── examples/
│   └── *.sspec
├── docs/
├── diagrams/
│   └── *.puml
├── tests/
│   ├── cli/
│   ├── bindings/
│   │   ├── cpp/
│   │   ├── go/
│   │   ├── java/
│   │   ├── rust/
│   │   └── e2e/
│   └── *.cpp
└── testdata/
    ├── generators/
    └── parity/
```

---

## Architecture

The current implementation is a C++20 compiler and generator toolchain.

The active pipeline is:

```text
.sspec text
    ↓
lexer
    ↓
parser
    ↓
AST
    ↓
include composition
    ↓
validator / semantic resolver
    ↓
canonical IR
    ↓
formatter / CLI / OpenAPI generator / language binding generators
```

### Key rule

Generators, diagrams, and editor features should consume the semantic model or canonical IR wherever possible. Avoid building features directly against raw text unless the feature is purely syntactic.

### Source layout rule

Parser and validator implementation is intentionally decomposed by language area:

- `parser_top_level.cpp`, `parser_entities.cpp`, `parser_apis.cpp`, `parser_workflows.cpp`, `parser_runtime.cpp`, `parser_observability.cpp`, `parser_values.cpp`, and related helpers own AST construction.
- `validator_declarations.cpp`, `validator_entities.cpp`, `validator_runtime.cpp`, `validator_workflows.cpp`, and related helpers own semantic checks.

Do not put new feature-specific parsing or validation back into monolithic catch-all files when a scoped module already exists.

### Include composition rule

StateSpec supports `include "path.sspec";` at file scope. The CLI composes included specs before validation and generation.

- Includes are path-based and resolved relative to the including file.
- Include cycles are errors.
- Duplicate loaded paths are ignored after first load.
- Included `system` members are appended into the root system.
- Included `tenant scoped_by` must match the root if both are present.
- Included `system_tenant configured` fills the root declaration only when the root omits it.
- `import` is intentionally unsupported.

---

## Binding Runtime Persistence Model

Binding interface code that reads or writes persisted StateSpec runtime state must use the optimistic-concurrency-control backend model.

This applies to all language bindings, including C++, Go, Java, and Rust, and to every runtime component that can observe persisted state:

- entity/document access
- leases
- queues
- workflows
- feature flags
- generated runtime helpers that inspect, evaluate, claim, enqueue, start, complete, fail, cancel, register, or otherwise depend on persisted records

### Backend boundary rule

Backends and transactions are generic OCC document storage primitives. They must not know
about queues, leases, workflows, feature flags, logs, metrics, or any other higher-level
runtime component as native backend concepts.

The backend layer owns only:

- collection registration and descriptors
- versioned records
- queries over registered collections
- transaction read sets, staged writes, staged deletes, commit, and abort
- backend capabilities and conflict reporting

Collection descriptor registration is the schema upgrade boundary. Backend adapters
must validate re-registration of an existing collection with the shared schema
compatibility helper for that language, not with hand-rolled per-backend comparison
logic. The helper names are:

- C++: `compare_collection_descriptors` and `validate_collection_descriptor_upgrade`
- Go: `CompareCollectionDescriptors` and `ValidateCollectionDescriptorUpgrade`
- Java: `SchemaCompatibility.compareCollectionDescriptors` and
  `SchemaCompatibility.validateCollectionDescriptorUpgrade`
- Rust: `compare_collection_descriptors` and `validate_collection_descriptor_upgrade`

Runtime stores define collection descriptors, but only the generic backend registration
path enforces whether a descriptor upgrade is identical, backwards compatible, or a
schema conflict. Bulk registration APIs such as `ensure_collections`,
`EnsureCollections`, and `ensureCollections` must validate the whole batch against
staged descriptor state before applying any descriptor, so a later schema conflict
cannot partially publish earlier descriptors from the same generated startup pass.
Schema conflict messages should include the collection name, existing and requested
schema versions, and the compatibility reason/path details from the shared helper.

Runtime components are users of the backend layer. Queue stores, lease stores, workflow
stores, feature flag stores, log sinks, and metric sinks must register the collections
they need and persist their records through `Backend`/`IBackend` and
`Transaction`/`ITransaction`.

### Runtime domain dependency rule

Runtime domain dependencies are directional, not forbidden outright. A higher-level
domain may use a lower-level domain when the dependency does not create a cycle.
Implementations and generated code must follow this table:

| Domain | May depend on |
|---|---|
| Feature flags | OCC only |
| Queues | OCC, queues for dead-letter routing |
| Leases | OCC only |
| Workflows | OCC, feature flags, queues, leases, workflows, logs, metrics |
| Logs | OCC only |
| Metrics | OCC only |
| Entity GC | OCC only |

This means workflows may enqueue messages, acquire leases, evaluate feature flags, emit
logs, record metrics, and start other workflows. The reverse dependencies are not
allowed: queue, lease, feature flag, log, metric, and GC implementations must not
require workflow functionality to work correctly. In particular, entity GC must not use
leases, queues, workflows, feature flags, logs, or metrics as part of its correctness
path, because those domains may themselves persist records that require GC. The
validator must reject declarations that introduce an edge outside this table.

Typed runtime stores and sinks must be backend-neutral clients. They should not depend
on `InMemoryBackend`, `InMemoryTransaction`, or equivalent memory-specific types. The
in-memory backend is one concrete adapter behind the same OCC interfaces used by
production backends.

Generated artifact layout must preserve that boundary:

- `memory/` contains only concrete in-memory backend and transaction adapters.
- `runtime/` contains backend-neutral codecs, feature flag stores, queue stores, lease
  stores, workflow stores, log sinks, and metric sinks.

Language packaging may add language-specific path prefixes, but it must not collapse
backend-neutral stores back into memory-specific packages.

Generated entity GC types, repositories, registration helpers, and workers are shared
common-tier runtime primitives. Per-entity GC descriptor functions belong with generated
entity modules as entity-local descriptor facts. API process and Worker runtime code own
GC lifecycle and descriptor binding. GC descriptor catalogs are deployment-tier
artifacts, not common global artifacts: API emits an API catalog, Worker emits a Worker
catalog, and each tier passes its chosen catalog into the shared registration helper.
Shared GC registration helpers must accept an explicit descriptor list from the caller;
they must not discover or call a common global descriptor function internally. This
keeps the binding point tier-owned today and allows API and Worker deployments to bind
different GC subsets later. Each tier must expose explicit config flags for enabling or
disabling GC. Defaults should keep standalone generated apps working, while production
deployments can disable GC on the tier that should not host background collection.

Do not add domain-specific transaction staging fields such as queue message maps,
workflow execution maps, lease maps, feature flag maps, log append buffers, or metric
append buffers to backend or transaction implementations. Those records belong in
component-owned collections.

### OCC rule

Persistent reads must be transaction-scoped.

Do not add binding APIs that read persisted state through standalone resolver/client interfaces that bypass `Backend`/`IBackend` and `Transaction`/`ITransaction`. A read that influences generated behavior, workflow branching, feature flag evaluation, queue claiming, lease acquisition, or entity mutation must participate in the same OCC transaction as the operation that consumes that read.

### Required binding shape

Runtime binding components should expose both method styles when they operate on persisted state:

1. Backend-managed methods that take a backend and own transaction lifecycle internally.
2. Caller-managed `Tx` methods that take an existing transaction so callers can compose multiple reads and writes atomically.

Backend-managed methods are expected to follow:

```text
begin transaction
perform persisted reads/writes through the transaction
commit on success
abort on failure
```

Caller-managed methods must not begin, commit, or abort the transaction they receive unless their contract explicitly says so.

### Feature flag rule

Feature flag usage is persisted-state access. Evaluation must be available as a transaction-scoped binding API, because generated workflows and services may branch on feature flag values while also reading or mutating entities, leases, queues, or workflow records.

### Generator rule

Generated binding code must consume these transaction-aware interfaces. Generators must not emit direct persisted-state reads, in-memory feature flag resolvers, or target-language helper APIs that bypass the canonical backend transaction model.

Generated descriptor values are spec-driven. Descriptor functions and records must report
what the `.sspec` file declares, and unused declaration categories should be absent or
empty according to the language's descriptor shape. Do not synthesize queue, lease,
workflow, feature flag, log, metric, API, worker, or app descriptor values merely
because a runtime helper exists.

Generated runtime artifacts are usage-pruned. Backend and transaction contracts remain
generic and always available in the common tier, but typed runtime stores, sinks,
codecs, worker views, app composition roots, module manifests, and compile-check
includes should be emitted only when the spec or generated API/Worker app needs that
runtime domain.

Runtime stores are typed backend clients. A generated queue, lease, workflow, feature
flag, log, or metric store is emitted because the spec or generated app references that
domain; it must still operate through the generic backend/transaction model and must not
be treated as a backend feature.

Generated entity garbage collection is a shared common-tier runtime feature, not an
API-only or Worker-only feature. API and Worker apps may both compose the same generated
entity GC workers. The baseline is one registered task per generated entity GC
descriptor, bounded batches, OCC revalidation, and generated lifecycle start/stop/join
wiring owned by `ApiProcess` or `WorkerProcess`.
Generated API process and Worker runtime configs must include explicit GC enablement
flags so mixed deployments can choose exactly one active GC host.

Do not introduce a scheduler abstraction for entity GC yet. GC registration must remain
a simple loop over generated GC descriptors, and GC implementations must use only the
generic OCC backend/transaction boundary. GC must not depend on leases, queues,
workflows, feature flags, logs, or metrics.

---

## Generated Application Architecture

Language bindings for C++, Go, Java, and Rust generate deployable artifact groups:

- `common` contains backend interfaces, usage-pruned typed runtime surfaces, shared descriptors, entity/workflow/queue/lease/log/metric/feature-flag definitions, external-system metadata helpers, and generated build files.
- `api` contains API tier descriptors, route descriptors, handler contracts, dispatchers, operator metadata APIs, and API server shells.
- `worker` contains worker descriptors, worker contexts, usage-pruned queue/lease/workflow views, workflow step handler contracts, worker registries, worker app shells, and workflow runners with keep-alive support.

### Generated facade dependency rule

Generated facade modules are for user imports, compatibility of top-level generated
composition, and broad catalog access. Examples include:

```text
common/descriptors.hpp
common/backend/descriptors.go
common/com/statespec/generated/Descriptors.java
common/descriptors.rs
api/api_descriptors.*
worker/worker_descriptors.*
```

Focused generated modules must not depend on those facades when a narrower dependency
exists. Per-API descriptor modules, per-worker descriptor modules, per-entity modules,
API codecs, API handlers, worker registries, and runtime clients should include or
import exact descriptor type modules, entity modules, shape modules, backend interfaces,
or runtime clients directly. This keeps generated dependency graphs scalable and keeps
language compilers from loading large aggregate files for small focused artifacts.

Common/API/Worker tier boundaries must remain explicit. API-only code belongs under the
API tree, Worker-only code belongs under the Worker tree, and common code is limited to
state and contracts genuinely shared by both tiers, such as entities, backend
interfaces, descriptor types, runtime contracts, shape descriptors when shared, and
schema/runtime registration helpers. Do not move tier-specific convenience code into
`common` merely to make imports easier.

The generator tier option controls emitted artifacts:

```bash
statespec generate bindings --lang cpp <file.sspec> --tier common
statespec generate bindings --lang go <file.sspec> --tier api
statespec generate bindings --lang java <file.sspec> --tier worker
statespec generate bindings --lang rust <file.sspec> --tier all
```

### API app rule

Generated API apps own structural wiring, not business behavior:

- API server shell lifecycle, process lifecycle, local composition, and request context shape.
- API process background execution, bootstrap, stop, and join semantics through generated
  lifecycle APIs.
- Local/no-op blocking transport behavior for generated app startup and shutdown tests,
  including unblocking when the generated process requests stop.
- Map-based route lookup and API-name-to-handler dispatch.
- Backend-owned handler invokers for generated entity CRUD operations.
- Business API handler interfaces for top-level non-CRUD APIs.
- Concrete generated default handlers for entity-owned CRUD operations identified by
  `api.entity` plus `api.repository_operation` in IR.
- Operator metadata API contracts.
- Descriptor views over APIs, routes, servers, shapes, and external-system metadata mappings.

User-owned code supplies real network transport selection, HTTP/RPC framework adapters,
authentication, authorization, tenancy policy, concrete handlers for top-level business
APIs, validation beyond the spec, concrete backend adapter, and outbound clients.
Generated local transports must not be treated as an opinionated HTTP backend; they own
only local lifecycle blocking/unblocking until StateSpec intentionally adopts a concrete
HTTP runtime.

Generated API dispatchers must use lookup maps for request-time dispatch:
`route_name -> ApiRouteDescriptor`, `api_name -> generated CRUD invoker`, and
`api_name -> business handler invoker`. Descriptor lists remain available for catalogs
and introspection, but generated request dispatch should not linearly scan route or API
descriptor lists. Top-level handler registries should be thin composers that register
per-entity CRUD invokers and separate business handler invokers; generated CRUD method
bodies belong in per-entity API modules, not in the top-level registry.

Generated `main` functions and tests should not wrap `ApiProcess.run` in ad hoc
threads. `ApiProcess` owns the background thread, goroutine, or task; callers start it,
request stop, and join it.

### Worker app rule

Generated worker apps own structural workflow and worker wiring:

- Worker application lifecycle shell.
- Worker registry and descriptor lookup.
- Workflow step handler interfaces.
- Queue worker interfaces.
- Workflow runner behavior for claim, keep alive, complete, fail, and retry-visible state.

User-owned code supplies concrete workflow step handlers, concrete queue workers, external API clients, retry policy integration, runtime configuration, and concrete backend adapters.

### Generated artifact naming rule

Generated artifacts must use meaningful filenames for the code they contain. Avoid catch-all files such as `api_artifacts.*` or `worker_artifacts.*`.

Current C++ generated common descriptors emit to:

```text
common/descriptors.hpp
```

Equivalent descriptor-heavy files in other languages are:

```text
common/backend/descriptors.go
common/com/statespec/generated/Descriptors.java
common/descriptors.rs
```

These common descriptor facades are for shared descriptor types and common system catalogs.
They must not expose API shape descriptor lists or worker catalogs. API shape catalogs
belong in the API tier, and worker catalogs belong in the Worker tier. A common shape
descriptor catalog should be emitted only for shapes that are truly shared and not API
contracts.

---

## Generator Template Architecture

Generators use source template files instead of large inline string blocks for runtime and app scaffolding.

Template roots live under each binding:

```text
bindings/cpp/
bindings/go/
bindings/java/
bindings/rust/
```

Generated-source templates use `.tmpl` suffixes, for example:

```text
bindings/cpp/generated/descriptors.hpp.tmpl
bindings/go/generated/descriptors.go.tmpl
bindings/java/generated/Descriptors.java.tmpl
bindings/rust/generated/descriptors.rs.tmpl
```

Use `TemplatePackage` and `TemplateRenderer` for template loading and placeholder replacement. Prefer moving large generated source bodies into templates when the content is mostly static app/runtime structure. Descriptor-heavy content that is generated from IR may still be assembled by generator code and injected into templates.

The CLI supports template override roots:

```bash
statespec generate bindings --lang cpp spec.sspec --template-root /path/to/templates
```

---

## Language Concepts

StateSpec v0.1 includes these file-level and system-level concepts:

- `statespec` file header
- `include`
- `system`
- `tenant scoped_by`
- `system_tenant configured`
- `value`
- `enum`
- `shape`
- `external_system`
- `feature_flag`
- `log`
- `metric`
- `entity`
- `event`
- `queue`
- `message`
- `lease`
- `worker`
- `api_server`
- `api`
- `workflow`
- `policy`
- `generate`
- `annotations`

`state`, `transition`, and `garbage_collection` declarations are not top-level objects; they belong inside an entity `state_machine`.

Entities are durable system objects. External systems model integration dependencies and operator-owned metadata mappings. Feature flags model declared rollout and configuration switches. Logs and metrics model declared observability signals. Queues and leases model durable runtime coordination. Workers bind execution to queues, leases, and workflows. API servers host one or more APIs. Workflows describe asynchronous execution. APIs expose external contracts. Events connect APIs, workflows, and workers. Policies express authorization, tenancy, quota, and audit intent.

---

## Entity Rules

Entities represent durable objects with identity, ownership, lifecycle, fields, relationships, and invariants.

Canonical entity shape:

```sspec
entity Name {
  key tenant_id, id

  ownership {
    authority: system
    system_of_record: self
    lifecycle: authoritative
  }

  version int

  fields {
    created_at timestamp
    updated_at timestamp
    status string
    id uuid
  }

  state_machine {
    state Requested;
    state Active;
    state Deleted {
      terminal: true
      garbage_collection {
        after: P30D
        mode: tombstone
      }
    }

    initial Requested
    terminal [Deleted]

    Requested -> Active;
    Active -> Deleted;
  }
}
```

### Entity invariants

- Every entity must declare a `key`; tenant-scoped systems should include the tenant field in durable entity keys.
- Entity names use PascalCase.
- Field and key names use snake_case.
- Every entity must declare `ownership`.
- Every entity must declare a `fields` block containing `created_at timestamp`, `updated_at timestamp`, and `status string`.
- Every entity must declare a `state_machine`.
- State machine transitions must reference valid state members.
- Terminal states must be marked `terminal: true`, listed in `terminal [...]`, and declare `garbage_collection`.
- Workflows may not introduce undeclared state transitions; workflow state mutation must use declared transitions.

### Entity-owned CRUD APIs

Canonical CRUD is entity-owned. Standard create, get, list, status update, and delete
operations must be declared inside the entity `api` block so they derive from the
entity key, fields, indexes, state machine, and garbage-collection semantics.

Do not hand-write duplicate top-level APIs for `Create<Entity>`, `Get<Entity>`,
`List<Entity>`, `Update<Entity>Status`, or `Delete<Entity>` lifecycle operations.
Top-level `api` declarations are for business actions that are not standard entity
lifecycle operations, such as starting a workflow, enqueueing a command, or exposing a
domain-specific operation.

Entity-owned list APIs may use only key prefixes, declared index names, or declared
index field prefixes in order. They must not imply arbitrary scan-by-field behavior.

---

## Ownership Model

Ownership is first-class.

Supported authority modes:

- `system` — this system is authoritative.
- `external` — another system is authoritative.

Supported lifecycle modes:

- `authoritative` — this system owns create/update/delete lifecycle.
- `managed` — this system requests lifecycle actions from another authority.
- `observed` — this system reads or caches externally-owned data.
- `projected` — this entity is a local projection or read model.

### Rule

Do not model externally-owned resources as if local state is authoritative. If this system creates a resource through another system’s REST API, prefer either:

1. a managed external entity, or
2. a pair of entities:
   - a local authoritative request/orchestration entity
   - an externally-owned managed resource entity

---

## External-System Metadata Model

External systems may require metadata that should not be exposed as end-user API fields. StateSpec models this through external-system metadata declarations and operator APIs rather than forcing all remote-call parameters into public APIs.

Metadata rules:

- `external_system ... metadata` names the metadata entity, profile field, required fields, and mappings.
- Metadata entities are durable StateSpec entities and use the same tenant and OCC rules as other entities.
- In tenant-scoped systems, operator metadata APIs and metadata entities must carry the service tenant field.
- Metadata mappings describe how values from `metadata`, `input`, `entity`, or `workflow` sources map to generated `client.*` or `request.*` fields.
- Operator APIs own create/update/disable/delete style metadata management.
- Generated bindings may provide mapping plans and applicators, but runtime code owns concrete remote clients and secret/config handling.

Do not expose backend-only remote metadata through user-facing API fields just because a remote service needs it.

---

## Parent/Child Relationships

Parent-child relationships are first-class.

The child entity declares the relationship in `relations`:

```sspec
relations {
  parent cluster_id: ref<Cluster> {
    kind: composition
    on_parent_delete: cascade
    unique_within_parent: [ordinal]
  }
}
```

The parent may optionally declare an inverse view:

```sspec
children {
  nodes: Node by cluster_id
}
```

### Rules

- The child owns the parent reference.
- Parent-side `children` declarations are optional readability and generation hints.
- Composition relationships must not form cycles.
- `unique_within_parent` fields must exist on the child.
- `parent_must_be_in` states must exist on the parent state type.
- `detach` delete behavior requires an optional parent reference.

---

## Workflow Model

Workflows are durable, restart-safe descriptions of asynchronous execution.

Workflows should:

- load one primary entity
- declare preconditions with `require`
- mutate entity state only through legal transitions
- emit events explicitly
- call external systems through named steps
- be idempotent across retries

### Workflow state doctrine

Entity state is the durable contract between workflows.

Parent workflows should not depend on private child workflow internals. Parent workflows should observe child entity state.

---

## Child Orchestration Pattern

StateSpec supports a canonical three-phase pattern for parent workflows that create child entities.

### Phase 1: `generate_child_ids`

Atomically generate child IDs and store them on the parent in a pending list.

### Phase 2: `creating_children`

For each pending child ID:

- create the child entity
- emit the child creation event
- move the child ID from pending to creating

### Phase 3: `waiting_children`

Monitor child entity state for each creating child ID:

- move successful children to succeeded list
- move failed children to failed list
- complete or fail the parent based on aggregate child state

Preferred bucket fields:

```sspec
pending_child_ids: list<uuid>
creating_child_ids: list<uuid>
succeeded_child_ids: list<uuid>
failed_child_ids: list<uuid>
```

Prefer `succeeded_child_ids` over `active_child_ids`, because not every child success terminal state is named `Active`.

### Required invariants

Child ID buckets must be disjoint and conserved:

```sspec
invariants {
  childIdsPartitioned:
    disjoint(pending_child_ids, creating_child_ids, succeeded_child_ids, failed_child_ids)

  childIdsConserved:
    count(pending_child_ids) +
    count(creating_child_ids) +
    count(succeeded_child_ids) +
    count(failed_child_ids) == desired_child_count
}
```

### Idempotency rule

Child creation must be retry-safe. A repeated `creating_children` step must not duplicate child entities or relaunch already-created child work incorrectly.

---

## Grammar Ownership

The grammar lives in:

```text
grammar/statespec.ebnf
```

Any grammar change must update, as applicable:

- parser tests
- semantic tests
- formatter tests
- examples
- language documentation
- VS Code syntax highlighting
- LSP completion and diagnostics

Do not change grammar without adding valid and invalid examples.

---

## Parser Guidelines

The parser should:

- preserve source spans for diagnostics
- recover from common syntax errors for editor use
- produce useful diagnostics rather than stopping at the first error
- avoid embedding semantic validation in syntax parsing

Parser output should be AST, not IR.

---

## Semantic Analyzer Guidelines

The semantic analyzer should own:

- name resolution
- type checking
- state validation
- transition validation
- relationship validation
- ownership validation
- workflow validation
- child-set validation
- invariant validation

Examples of semantic errors:

- unknown entity reference
- unknown state member
- invalid transition
- invalid parent relation
- child bucket type mismatch
- workflow state update not allowed by transition graph
- externally-owned entity claiming authoritative local transition behavior

---

## IR Guidelines

Canonical IR should be:

- fully resolved
- normalized
- stable for generators
- independent of source formatting
- explicit about defaults

Generators should consume IR, not AST.

IR should distinguish:

- syntax-level names
- resolved entity/type references
- authoritative vs external ownership
- parent-child relationships
- local lifecycle state vs observed external state
- workflow steps and child orchestration phases
- API server declarations and served APIs
- worker declarations and queue/workflow execution bindings
- external-system metadata entities and mapping plans

Field descriptors in generated bindings should use typed field-kind enums connected to grammar-supported entity types. Avoid treating descriptor field types as arbitrary strings when the grammar defines a closed set of scalar, collection, optional, reference, and named type categories.

---

## Formatter Guidelines

Formatter output must be deterministic.

Canonical ordering inside `entity`:

1. `key`
2. `ownership`
3. `version`
4. `fields`
5. `state_machine`
6. `relations`
7. `children`
8. `invariants`
9. `indexes`
10. `annotations`

Canonical ordering inside `workflow`:

1. `version`
2. `singleton`
3. `expected_execution_time`
4. `start`
5. `on`
6. `input`
7. `state`
8. `load`
9. `child_set`
10. `step`

Canonical ordering inside `log`:

1. `level`
2. `event_name`
3. `fields`

Canonical ordering inside `metric`:

1. `kind`
2. `name`
3. `unit`
4. `labels`

Canonical ordering inside `policy`:

1. `tenant`
2. `allow`
3. `deny`
4. `quota`
5. `audit`

Do not introduce formatting options until there is a strong reason. One canonical style is preferred.

---

## CLI Guidelines

The current CLI exposes:

```bash
statespec help
statespec validate
statespec fmt
statespec tokens
statespec ast
statespec generate bindings
statespec generate openapi
```

CLI behavior should be suitable for CI.

- `validate` exits non-zero on syntax or semantic errors.
- `fmt --check` exits non-zero if files are not formatted.
- `generate bindings` supports `--lang <cpp|go|java|rust>`, `--out DIR`, `--tier <all|common|api|worker>`, and `--template-root DIR`.
- `generate openapi` supports `--out DIR`.
- Generated file manifests in tests must be locale-stable; use `LC_ALL=C sort` when comparing generated paths.

---

## Makefile Guidelines

The top-level `Makefile` is the canonical developer entrypoint.

Important targets:

- `make cli-fast` builds the CLI with parallel jobs based on the host logical CPU count and is the preferred local first-build command on macOS.
- `make cli` builds the CLI with normal make job settings and remains the portable fallback.
- `make test` builds the compiler/tests and runs CLI tests.
- `make test-generated-apps` runs complete generated API/worker app E2E fixtures.
- `make test-bindings` runs C++, Go, Java, and Rust binding-local tests.
- `make format` formats core C++ sources plus binding implementations by invoking binding-local Makefiles.
- `make format-check` checks core and test formatting.
- `make diagrams-png` renders `diagrams/*.puml` to PNG with PlantUML.
- `make clean` removes build outputs, diagram PNGs, binding build outputs, `bindings/rust/target`, and `bindings/rust/Cargo.lock`.

Binding directories should keep their own `Makefile` for local language-native formatting, testing, and cleanup. The top-level Makefile should invoke those local targets rather than duplicating language-specific commands.

---

## VS Code Extension Guidelines

The VS Code extension is preferred over a standalone visual editor.

The extension should provide:

- syntax highlighting
- diagnostics
- autocomplete
- hover documentation
- outline view
- go to definition
- find references
- rename
- formatting
- state graph visualization
- workflow graph visualization
- command palette actions for validation and generation

### Source of truth rule

Visual editing must rewrite `.sspec` text. It must not maintain a separate authoritative model.

---

## Generator Guidelines

Generators should be separated by target:

- OpenAPI
- C++
- Go
- Java
- Rust
- diagrams
- tests

Each generator should:

- consume canonical IR
- be deterministic
- have golden tests
- avoid hidden runtime assumptions
- emit clear warnings for unsupported language features
- derive canonical CRUD API, shape, repository, OpenAPI, and default handler surfaces
  from entity-owned CRUD IR
- use `api.entity` plus `api.repository_operation` as the source of truth for generated
  entity CRUD handler behavior; do not infer CRUD behavior from operation names
- emit tiered app artifacts into `common`, `api`, and `worker` paths where applicable
- keep generated descriptor values spec-driven
- prune generated runtime artifacts, imports, module declarations, and compile-check
  manifests by actual runtime usage
- use template files for mostly static runtime/app scaffolding

Generated output should include a header indicating it is generated from StateSpec.

Generated bindings should include local build files that compile the emitted package with language-native tooling:

- C++ uses generated `Makefile` plus `clang++`/compatible C++20 compiler checks.
- Go uses generated `go.mod` and `Makefile`.
- Java uses generated `Makefile` and core Java compiler/runtime only.
- Rust uses generated `Cargo.toml`, `lib.rs`, and `Makefile`.

Generated build files must not reference pruned runtime files. If a spec only uses
workflows, the module manifest and compile-check surface should not import feature flag,
queue, log, or metric runtime stores. Backend and transaction interfaces are the
exception: they are generic contracts and remain available even when no typed runtime
domain is used.

---

## Testing Guidelines

Use focused tests for:

- parser output
- formatter output
- semantic diagnostics
- generated artifacts
- diagram output
- generated app manifests
- language binding compile checks

Current test layout:

```text
tests/
├── Makefile
├── cli/
│   ├── Makefile
│   └── *_tests.sh
├── bindings/
│   ├── Makefile
│   ├── cpp/
│   ├── go/
│   ├── java/
│   ├── rust/
│   └── e2e/
└── *_tests.cpp

testdata/
├── generators/
└── parity/
```

Every language feature should have:

- at least one valid example
- at least one invalid example
- formatter coverage
- semantic validation coverage

Top-level `make test` runs core and CLI tests. `make test-bindings` runs binding-local tests for C++, Go, Java, and Rust. `make test-generated-apps` runs complete generated API/worker app fixture checks.

### Cross-language binding fixture parity

Generated binding fixtures for C++, Go, Java, and Rust should cover the same runtime
contract. When adding or changing generated runtime behavior in one language, update the
other language fixtures in the same PR unless the difference is intentionally documented.

Each language fixture should verify:

- generated common, API, and worker artifacts compile with language-native tooling
- generated descriptor values reflect only the spec declarations
- unused runtime files, imports, members, and module declarations are pruned
- metadata resolver fixtures link and run
- one in-memory backend instance composes backend-neutral feature flag, queue, lease,
  workflow, log, and metric runtime clients
- transaction-scoped registration and runtime operations work through the public backend
  and transaction interfaces
- inspect APIs are usable for logs and metrics
- generic backend `put`/`query` primitives work independently of typed runtime stores

Generated app e2e fixtures should also remain aligned across languages for API app
linking, Worker app linking, generated manifest shape, and in-memory backend composition.

---

## Examples Policy

Examples are part of the language contract.

Keep examples realistic and aligned with intended use cases:

- control plane resources
- API + worker architectures
- parent-child orchestration
- externally-managed resources
- lifecycle-driven services

Examples should be small enough to understand but complete enough to validate.

---

## Documentation Policy

Docs should be kept close to the language design.

Recommended docs:

```text
docs/language-model.md
docs/entities-and-state.md
docs/feature-flags.md
docs/observability.md
docs/queues-leases-workers.md
docs/workflow-launch-control.md
docs/external-system-metadata.md
docs/backend-abstractions.md
docs/binding-template-strategy.md
docs/generators.md
docs/compiler-parity.md
docs/style-guide.md
```

Any grammar or semantic change should update docs in the same PR.

PlantUML architecture diagrams live in `diagrams/*.puml`. `make diagrams-png` renders PNGs, and `make clean` removes generated PNG files. Track diagram source unless the user explicitly asks for rendered images.

---

## Coding Style

Prefer declaring repeated static strings once as named constants and using those constants throughout the code. This applies especially to collection names, field names, descriptor keys, API names, state names, generated filenames, and diagnostic identifiers.

The goal is to let the compiler catch misspellings and stale references where the host language supports it, instead of scattering unchecked string literals across parser, validator, IR, generator, runtime, and test code.

Generated binding code must follow the same rule. Entity names, entity field names, entity state values, and entity index names should be emitted once as generated constants and then reused by descriptors, repositories, API handlers, GC metadata, and runtime helper code. In particular, generated index helper implementations must call the generic index query path with generated index-name constants, not repeated raw index-name literals.

Cross-language generated entity persistence is a compatibility contract. Generated C++, Go, Java, and Rust repositories for the same `.sspec` file must keep collection names, key-field ordering, key encoding, persisted field names, index names, index field order, and uniqueness flags aligned. Changes to those details require cross-language generated binding tests and documentation updates.

Use local string literals only when the value is genuinely one-off, clearer inline, or part of test data that is intentionally asserting exact generated output.

---

## Naming Conventions

- Entities: `PascalCase`
- States: `PascalCase`
- State members: `PascalCase`
- APIs: `PascalCase`
- API servers: `PascalCase`
- Workflows: `PascalCase`
- Events: `PascalCase`
- Queues: `PascalCase`, usually ending in `Queue`
- Leases: `PascalCase`, usually ending in `Lease`
- Workers: `PascalCase`
- Fields and keys: `snake_case`
- Steps: `snake_case`
- Child sets: `snake_case` or plural `snake_case`

Preferred workflow step names for child orchestration:

- `generate_child_ids`
- `creating_children`
- `waiting_children`

---

## Compatibility and Versioning

Language versions should be explicit once the grammar stabilizes.

Breaking changes include:

- keyword changes
- grammar changes that invalidate existing specs
- semantic reinterpretation of existing constructs
- IR changes that alter generator behavior

Non-breaking changes include:

- new optional blocks
- new generators
- new diagnostics
- stricter warnings that are not errors

---

## Pull Request Expectations

A PR that changes the language should include:

- grammar update
- parser update
- semantic validation update
- formatter update if syntax is affected
- docs update
- examples update
- valid and invalid tests

A PR that changes generators should include:

- IR compatibility check
- golden output updates
- documentation if user-facing behavior changes

A PR that changes VS Code behavior should include:

- extension tests where practical
- LSP tests for language behavior
- screenshots or notes for visual changes when useful

---

## Things to Avoid

Avoid:

- adding syntax aliases
- reintroducing `import` as an alias for `include`
- adding dynamic typing
- adding untyped `any` as an escape hatch
- putting business semantics in annotations
- letting generators define semantics
- letting the VS Code extension diverge from CLI validation
- creating standalone diagram state separate from `.sspec`
- modeling external resources as locally authoritative
- launching child workflows without durable child entity state
- adding binding APIs that read persisted state outside the OCC backend transaction model
- collapsing generated app files into catch-all artifact files

---

## Current Design Biases

StateSpec is optimized for:

- distributed systems
- REST API + worker architectures
- two-tier API server and worker deployments
- control planes
- lifecycle-managed resources
- parent-child orchestration
- idempotent async workflows
- backend-neutral OCC runtime bindings
- external-system metadata management
- declared observability
- code generation
- VS Code-first developer experience

It is not intended to be:

- a general-purpose programming language
- a replacement for Rust, Go, Java, or C++
- a diagram-only modeling tool
- a free-form requirements language

---

## One-Sentence Project Doctrine

StateSpec turns distributed system design into canonical, executable text: entities define durable state, APIs express intent, workflows drive lifecycle, and generated artifacts derive from one validated source of truth.
