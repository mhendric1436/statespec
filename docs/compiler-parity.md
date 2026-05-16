# Compiler Parity Matrix

StateSpec intentionally keeps the grammar ahead of some implementation details, but
compiler work should make that gap explicit. This matrix tracks each language construct
through the compiler pipeline:

```text
grammar -> lexer/parser -> AST -> validator -> semantic model -> IR -> formatter -> generators
```

Status meanings:

| Status | Meaning |
|---|---|
| `complete` | Implemented for the current compiler scope and covered by tests. |
| `partial` | Some syntax or semantics exist, but the construct is not fully represented end to end. |
| `grammar-only` | Reserved in `grammar/statespec.ebnf` and lexer keywords, but not meaningfully parsed into AST. |
| `not-started` | Not represented beyond broad documentation or future intent. |

## Top-Level Constructs

| Construct | Grammar | Parser/AST | Validator | Semantic | IR | Formatter | Generators | Priority | Notes |
|---|---|---|---|---|---|---|---|---|---|
| `statespec` header | complete | complete | partial | n/a | n/a | complete | n/a | P2 | Version is parsed; formal language-version compatibility policy is still minimal. |
| `include` | complete | complete | complete | complete | complete | partial | complete | P1 | Composition is supported for validation/generation; formatter keeps file-local includes. |
| `import` | complete | complete | partial | not-started | not-started | partial | not-started | P3 | Parsed as metadata only; no module/import resolution semantics yet. |
| `system` | complete | complete | complete | complete | complete | complete | complete | P0 | Root system is the implemented compilation unit. |
| `namespace` | complete | grammar-only | not-started | not-started | not-started | not-started | not-started | P2 | Queue `namespace` property is supported; declaration block namespaces are not. |
| `tenant scoped_by` | complete | complete | complete | complete | complete | complete | partial | P1 | Lowered into IR; runtime enforcement and generator usage remain limited. |
| `system_tenant configured` | complete | complete | complete | complete | complete | complete | partial | P1 | Lowered into IR; generated runtime config contract should be expanded. |
| `value` | complete | grammar-only | not-started | not-started | not-started | not-started | not-started | P2 | Needed for reusable constants and policy expressions. |
| `enum` | complete | grammar-only | not-started | not-started | not-started | not-started | not-started | P2 | Needed before named type generation is complete. |
| `shape` | complete | complete | complete | complete | complete | complete | partial | P1 | Shape descriptors are generated; target-language type generation is still pending. |
| `external_system` | complete | grammar-only | not-started | not-started | not-started | not-started | not-started | P3 | Required for explicit managed/observed ownership modeling. |
| `feature_flag` | complete | complete | complete | complete | complete | complete | partial | P1 | Descriptors and bindings exist; generated registration helpers are not complete across all languages/catalogs. |
| `log` | complete | complete | complete | complete | complete | complete | complete | P0 | Descriptor generation, bootstrap helpers, registration, emit, and inspect binding APIs exist. |
| `metric` | complete | complete | complete | complete | complete | complete | complete | P0 | Descriptor generation, bootstrap helpers, registration, record, and inspect binding APIs exist. |
| `entity` | complete | partial | partial | partial | partial | complete | partial | P0 | Core fields, keys, indexes, state machine, ownership, relations, children, and invariants exist; expression typing and index descriptors remain incomplete. |
| `event` | complete | grammar-only | not-started | not-started | not-started | not-started | not-started | P2 | Needed to connect APIs, workflows, queues, and workers explicitly. |
| `queue` | complete | complete | complete | complete | complete | complete | partial | P1 | Queue/message descriptors and bindings exist; generated runtime registration helper is still missing. |
| `lease` | complete | complete | complete | complete | complete | complete | partial | P1 | Lease descriptors and bindings exist; generated runtime registration helper is still missing. |
| `worker` | complete | complete | complete | complete | complete | complete | partial | P2 | References resolve; generator output currently does not produce worker scaffolding. |
| `api` | complete | complete | partial | partial | partial | complete | not-started | P1 | References resolve, but OpenAPI/server generation and shaped request/response support are not implemented. |
| `workflow` | complete | complete | complete | complete | complete | complete | partial | P1 | Step descriptors and registration helpers exist; workflow behavior syntax is not represented. |
| `policy` | complete | complete | partial | partial | partial | complete | not-started | P2 | Rules lower as strings/references; expression parsing and policy generation are not implemented. |
| `annotations` | complete | grammar-only | not-started | not-started | not-started | not-started | not-started | P4 | Keep low priority; annotations must not become a semantic escape hatch. |

## Entity Construct Parity

| Construct | Grammar | Parser/AST | Validator | Semantic | IR | Formatter | Generators | Priority | Notes |
|---|---|---|---|---|---|---|---|---|---|
| `key` | complete | complete | complete | complete | complete | complete | complete | P0 | Composite keys are supported. |
| `fields` | complete | complete | complete | complete | complete | complete | complete | P0 | Mandatory lifecycle fields are validated. |
| `state_machine` | complete | complete | complete | complete | complete | complete | complete | P0 | Initial, terminal, transitions, and GC metadata are represented. |
| `indexes` | complete | complete | complete | complete | complete | complete | partial | P1 | IR has indexes; collection descriptor generation should populate `IndexDescriptor`. |
| `ownership` | complete | complete | complete | complete | complete | not-started | complete | P1 | Represented in AST, validation, semantic model, IR, and binding descriptors; formatter support is still token-preserving. |
| `relations` | complete | complete | complete | complete | complete | not-started | complete | P1 | Parent/reference metadata is validated and emitted in descriptors; composition-cycle checks remain future work. |
| `children` | complete | complete | complete | complete | complete | not-started | complete | P2 | Parent-side declarations are validated against child-owned parent relations. |
| `transitions` block | complete | not-started | not-started | not-started | not-started | not-started | not-started | P2 | Separate from `state_machine` transitions; clarify or merge syntax before implementation. |
| `invariants` | complete | complete | partial | complete | complete | not-started | complete | P1 | Names and raw expressions flow through descriptors; expression parsing/type checking is still pending. |
| entity `annotations` | complete | not-started | not-started | not-started | not-started | not-started | not-started | P4 | Low priority by doctrine. |

## Workflow Construct Parity

| Construct | Grammar | Parser/AST | Validator | Semantic | IR | Formatter | Generators | Priority | Notes |
|---|---|---|---|---|---|---|---|---|---|
| workflow metadata | complete | complete | complete | complete | complete | complete | complete | P0 | Version, singleton, timing, start step, and retry metadata are supported. |
| `step` metadata | complete | complete | complete | complete | complete | complete | complete | P0 | Current step model is descriptor-oriented. |
| `on` trigger | complete | not-started | not-started | not-started | not-started | not-started | not-started | P1 | Needed to connect workflows to events and APIs. |
| `load` | complete | not-started | not-started | not-started | not-started | not-started | not-started | P1 | Needed for entity-centric workflow validation. |
| `require` | complete | not-started | not-started | not-started | not-started | not-started | not-started | P1 | Requires expression parsing and type checking. |
| `child_set` | complete | not-started | not-started | not-started | not-started | not-started | not-started | P2 | Needed for first-class child orchestration generation. |
| workflow operations | complete | not-started | not-started | not-started | not-started | not-started | not-started | P2 | `emit`, `enqueue`, `call`, `transition_to`, and similar operations need a typed model. |

## Generator Parity

| Area | Current Status | Priority | Next Work |
|---|---|---|---|
| Binding template emission | partial | P0 | Keep generated packages self-contained; add package manifests where practical. |
| Descriptor generation | partial | P0 | Fill missing entity indexes and runtime catalog bootstrap helpers. |
| Runtime bootstrap helpers | partial | P1 | Add feature flag, queue, and lease registration/provisioning helpers. |
| Worker scaffolding | not-started | P2 | Generate language-specific worker skeletons from worker/workflow/queue IR. |
| API generation | not-started | P1 | Generate OpenAPI or endpoint descriptors once `shape` support exists. |
| Policy generation | not-started | P3 | Wait for expression parsing/type checking. |

## Implementation Order

1. **P0: Keep implemented constructs complete.**
   Preserve current entity, state machine, log, metric, queue, lease, workflow, include,
   and binding generation behavior while closing known descriptor gaps.

2. **P1: Generate target-language types from `shape`.**
   Shapes now flow through the compiler and descriptor generators. The next increment is
   emitting typed request/response structs or classes for each binding language.

3. **P1: Add ownership, relations, and invariants for entities.**
   Initial descriptor-oriented support is in place. Remaining work is formatter ownership
   for canonical block ordering, typed invariant expressions, composition-cycle checks,
   and richer generator usage.

4. **P1: Expand runtime bootstrap helpers across all runtime catalogs.**
   Feature flags, queues, and leases should have generated transaction-scoped bootstrap
   helpers like workflows and observability.

5. **P1: Add workflow behavior IR.**
   Implement `on`, `load`, `require`, and typed step operations before generating worker
   bodies.

6. **P2: Add reusable `value`, `enum`, and `event` support.**
   These make specs more expressive and reduce repeated string literals.

7. **P2/P3: Add namespace, external systems, and policies.**
   Implement these once the core type/reference system is strong enough to support them
   without duplicating semantic rules in generators.

## Pull Request Rule

Any PR that marks a matrix cell as more complete should update this document and include
tests for the affected pipeline stages. A construct should not be called `complete` until
it has parser/AST coverage, validation or semantic coverage where applicable, formatter
coverage, and at least one generator or explicit non-generator rationale.
