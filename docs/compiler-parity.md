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
| `system` | complete | complete | complete | complete | complete | complete | complete | P0 | Root system is the implemented compilation unit. |
| `tenant scoped_by` | complete | complete | complete | complete | complete | complete | partial | P0 | Mandatory for every composed system and lowered into IR; runtime enforcement and generator usage remain limited. |
| `system_tenant configured` | complete | complete | complete | complete | complete | complete | partial | P0 | Mandatory for every composed system and lowered into IR; generated runtime config contract should be expanded. |
| `value` | complete | complete | complete | complete | complete | complete | partial | P2 | Reusable named value types flow through IR and binding descriptors; target-language type generation remains future work. |
| `enum` | complete | complete | complete | complete | complete | complete | partial | P2 | Enum declarations and members flow through IR and binding descriptors; target-language enum generation remains future work. |
| `shape` | complete | complete | complete | complete | complete | complete | partial | P1 | Shape descriptors and conservative target-language DTOs are generated; richer custom type mapping remains future work. |
| `external_system` | complete | complete | complete | complete | complete | complete | partial | P1 | External system properties, metadata entity references, tenant/key/profile metadata, required metadata fields, and source-to-target mapping descriptors flow through IR and generators. Operator metadata repository/API contracts and OpenAPI routes are generated; concrete persistence and remote clients are runtime-owned. |
| `feature_flag` | complete | complete | complete | complete | complete | complete | partial | P1 | Descriptors, bindings, and transaction-scoped generated registration helpers exist; runtime evaluator generation remains future work. |
| `log` | complete | complete | complete | complete | complete | complete | complete | P0 | Descriptor generation, bootstrap helpers, registration, emit, and inspect binding APIs exist. |
| `metric` | complete | complete | complete | complete | complete | complete | complete | P0 | Descriptor generation, bootstrap helpers, registration, record, and inspect binding APIs exist. |
| `entity` | complete | partial | partial | partial | partial | complete | partial | P0 | Core fields, keys, indexes, state machine, ownership, relations, children, and invariants exist; validator warnings flag noncanonical member ordering; expression syntax and built-ins are validated while type checking remains incomplete. |
| `event` | complete | complete | complete | complete | complete | complete | partial | P2 | Event payload fields are validated, emitted in descriptors, and have generated payload envelope builders; backend event transport remains future work. |
| `queue` | complete | complete | complete | complete | complete | complete | partial | P1 | Queue/message descriptors, bindings, and transaction-scoped generated creation helpers exist; worker scaffolding consumes queue references as metadata. |
| `lease` | complete | complete | complete | complete | complete | complete | partial | P1 | Lease descriptors, bindings, and transaction-scoped generated registration helpers exist; runtime lease enforcement is backend-owned. |
| `worker` | complete | complete | complete | complete | complete | complete | partial | P2 | References resolve and generated bindings expose worker descriptors, contexts, and handler skeleton interfaces; executable worker bodies remain future work. |
| `api` | complete | complete | partial | partial | partial | complete | partial | P1 | References resolve, passive API descriptor metadata is generated, and OpenAPI generation emits declared API operations. Request decoding, auth, framework routing, and concrete server adapters remain runtime-owned. |
| `api_server` | complete | complete | complete | complete | complete | complete | partial | P1 | API server declarations resolve served APIs and generated bindings expose descriptors, route metadata, request/response contexts, handler interfaces, process lifecycle types, and local blocking transports; real HTTP adapters remain runtime-owned. |
| `workflow` | complete | partial | complete | partial | partial | complete | partial | P1 | Step descriptors and registration helpers exist; workflow trigger/load metadata and linear step statements now lower into IR, but nested blocks and worker body generation remain future work. |
| `policy` | complete | complete | partial | complete | complete | complete | partial | P2 | Rules lower as strings/references and emit descriptor metadata; expression syntax and built-ins are validated while policy enforcement generation remains future work. |
| `annotations` | complete | grammar-only | not-started | not-started | not-started | not-started | not-started | P4 | Keep low priority; annotations must not become a semantic escape hatch. |

## External-System Metadata Pipeline

| Stage | Status | Notes |
|---|---|---|
| Grammar | complete | `external_system metadata` supports `entity`, `profile_field`, `required_fields`, and `mappings`. Operator APIs are ordinary `api` declarations served by ordinary `api_server` declarations. |
| Parser/AST | complete | Metadata declarations and mapping paths are parsed into AST nodes; operator API and API server declarations reuse the normal API AST. |
| Validator | complete | Metadata entities must resolve, include tenant/profile/key fields, declare a unique key index, use authoritative system ownership, and model the canonical `Active`/`Disabled`/`Deleted` lifecycle with `Deleted` as a terminal GC state. Mapping roots and target uniqueness are validated. `metadata.*` mapping sources must exist and be listed in `required_fields`. Operator API shapes follow normal tenant validation. |
| Semantic model | complete | External system metadata references, mappings, API declarations, and API server serves references resolve through the shared symbol model. |
| IR | complete | IR preserves metadata entity name, tenant field, profile field, metadata key fields, required fields, mapping source/target pairs, APIs, and API server route inputs. |
| Formatter | partial | Token-preserving formatting handles the syntax and canonical-order warnings cover the containing declarations. Full AST-owned reordering for all nested metadata/API forms remains future work. |
| Binding generators | complete | C++, Go, Java, and Rust emit external-system descriptors, mapping-plan helpers, default mapping applicators, missing-source diagnostics, generic external-system client interfaces and call adapters, metadata lookup helpers, transaction-scoped resolver helpers, OCC repository contracts plus default generic repository implementations, and operator metadata API handler contracts. |
| OpenAPI generator | partial | OpenAPI emits declared API operations and metadata-derived operator routes when explicit routes are absent. It emits schema contracts, not framework glue. Protobuf generation is intentionally not implemented. |
| Regression fixture | complete | `testdata/generators/external-system-metadata-e2e.sspec` validates the end-to-end pass across metadata, mappings, explicit operator APIs, API servers, OpenAPI, and generated bindings. |

Runtime-owned responsibilities are intentionally outside the compiler contract:

- durable backend implementations behind generic collection/document OCC primitives
- transaction begin/commit/retry orchestration
- Real network transport selection
- HTTP framework routing and request decoding
- authentication and authorization enforcement
- concrete remote clients, protocol serialization, retry, timeout, and circuit-breaker behavior
- secret resolution for credential references
- event transport and worker execution loops

## Entity Construct Parity

| Construct | Grammar | Parser/AST | Validator | Semantic | IR | Formatter | Generators | Priority | Notes |
|---|---|---|---|---|---|---|---|---|---|
| `key` | complete | complete | complete | complete | complete | complete | complete | P0 | Composite keys are supported. |
| `fields` | complete | complete | complete | complete | complete | complete | complete | P0 | Mandatory lifecycle fields are validated and must appear first in canonical order. |
| `state_machine` | complete | complete | complete | complete | complete | complete | complete | P0 | Initial, terminal, transitions, and GC metadata are represented. |
| `indexes` | complete | complete | complete | complete | complete | complete | complete | P1 | Index declarations populate generated collection `IndexDescriptor` metadata. |
| `ownership` | complete | complete | complete | complete | complete | not-started | complete | P0 | Mandatory for every entity. Represented in AST, validation, semantic model, IR, and binding descriptors; formatter support is still token-preserving. |
| `relations` | complete | complete | complete | complete | complete | not-started | complete | P1 | Parent/reference metadata is validated and emitted in descriptors; composition-cycle checks remain future work. |
| `children` | complete | complete | complete | complete | complete | not-started | complete | P2 | Parent-side declarations are validated against child-owned parent relations. |
| entity-level `transitions` block | not-started | not-started | not-started | not-started | not-started | not-started | not-started | n/a | Removed before implementation. State transitions are authored only inside `state_machine`. |
| `invariants` | complete | complete | partial | complete | complete | not-started | complete | P1 | Names and raw expressions flow through descriptors; expression syntax and built-ins are validated while type checking remains pending. |
| entity `annotations` | complete | not-started | not-started | not-started | not-started | not-started | not-started | P4 | Low priority by doctrine. |

## Workflow Construct Parity

| Construct | Grammar | Parser/AST | Validator | Semantic | IR | Formatter | Generators | Priority | Notes |
|---|---|---|---|---|---|---|---|---|---|
| workflow metadata | complete | complete | complete | complete | complete | complete | complete | P0 | Version, singleton, timing, start step, and retry metadata are supported; validator warnings flag noncanonical workflow member ordering. |
| `step` metadata | complete | complete | complete | complete | complete | complete | complete | P0 | Current step model is descriptor-oriented. |
| `on` trigger | complete | complete | partial | complete | complete | complete | not-started | P1 | Parsed, resolved, and validated against known trigger targets; trigger-specific execution semantics remain future work. |
| `load` | complete | complete | partial | complete | complete | complete | not-started | P1 | Parsed, resolved, and validated against entity key fields; loaded binding type propagation remains future work. |
| `require` | complete | complete | partial | complete | complete | complete | not-started | P1 | Parsed and lowered as a raw expression string; expression syntax, allowed built-ins, and feature flag references are validated while type checking remains future work. |
| `child_set` | complete | not-started | not-started | not-started | not-started | not-started | not-started | P2 | Needed for first-class child orchestration generation. |
| workflow operations | complete | partial | partial | partial | partial | not-started | not-started | P2 | Linear `set`, `emit`, `enqueue`, lease, `start workflow`, `transition_to`, `complete`, and `fail` statements lower into IR with target validation. Nested blocks and typed expression validation remain future work. |

## Generator Parity

| Area | Current Status | Priority | Next Work |
|---|---|---|---|
| Binding template emission | partial | P0 | Keep generated packages self-contained; add package manifests where practical. |
| Descriptor generation | partial | P0 | Collection, event, shape, log, and metric field descriptors now use the shared compiler field type classifier and emit language-native `FieldType` enums plus preserved StateSpec type names in C++, Go, Java, and Rust. Next work is expanding descriptor usage into richer endpoint, worker body, and policy generation. |
| Runtime bootstrap helpers | complete | P1 | Transaction-scoped helpers cover feature flags, queues, leases, workflows, logs, and metrics in all generated bindings. |
| Worker scaffolding | partial | P2 | Generated bindings expose worker descriptors, contexts, and language-specific handler interfaces. Next work is runnable queue polling, lease acquisition, and workflow dispatch loops. |
| API generation | partial | P1 | OpenAPI generation exists for declared APIs and generated operator metadata API surfaces. Next work is richer OpenAPI schemas, error model coverage, and optional framework adapters. |
| External-system metadata generation | partial | P1 | Descriptor, mapping, lookup, OCC repository, operator API handler, and OpenAPI contracts exist. Runtime persistence, remote clients, auth, retries, and HTTP framework adapters remain runtime-owned. |
| Policy generation | not-started | P3 | Wait for expression type checking and runtime evaluator design. |

## Implementation Order

1. **P0: Keep implemented constructs complete.**
   Preserve current entity, state machine, log, metric, queue, lease, workflow, include,
   and binding generation behavior while closing known descriptor gaps.

2. **P1: Generate target-language types from `shape`.**
   Implemented: generated bindings now include C++ structs, Go structs, Java records,
   and Rust structs for each shape using conservative scalar mappings. Next work is
   richer custom type mapping for value, enum, event, collection, and nested shape
   fields.

3. **P1: Expand relationship and invariant semantics for entities.**
   Ownership is mandatory and descriptor-oriented support for relations and invariants is
   in place. Remaining work is formatter ownership for canonical block ordering, typed
   invariant expressions, composition-cycle checks, and richer generator usage.

4. **P1: Expand runtime bootstrap helpers across all runtime catalogs.**
   Implemented: generated bindings now include transaction-scoped helpers for feature
   flags, queues, leases, workflows, logs, and metrics. Keep future runtime catalogs on
   the same caller-managed transaction model.

5. **P1: Add workflow behavior IR.**
   Initial workflow behavior IR is in place for `on`, `input`, `state`, `load`, `require`,
   `set`, `emit`, `enqueue`, lease operations, `start workflow`, `transition_to`,
   `complete`, and `fail`, with validation for loaded entity keys, resolved statement
   targets, workflow input/state types, expression syntax, allowed expression built-ins,
   and feature flag references in workflow expressions. The next increment is nested
   blocks and typed expression validation before generating worker bodies.

6. **P2: Add reusable `value`, `enum`, and `event` support.**
   Implemented: value, enum, and event declarations are parsed, validated, lowered
   through semantic/IR models, surfaced in AST JSON, and emitted as passive binding
   descriptor metadata. Next work is target-language type generation and event runtime
   helpers.

7. **P2/P3: Add external systems and policies.**
   Implemented: external systems are parsed and validated as named property catalogs,
   external-system metadata maps to operator-managed durable state, and policies now
   emit passive binding descriptor metadata. Metadata mapping and operator API
   contracts are generated; next work is external ownership semantics and generated
   policy enforcement hooks.

8. **P1: Generate API descriptors from typed shapes.**
   Implemented: generated bindings now include passive API descriptor metadata for
   method, path, input, output, error, workflow starts, and queue enqueue targets.
   OpenAPI generation now emits declared API operations and operator metadata API
   surfaces. Next work is richer schema lowering, error contract coverage, and optional
   endpoint adapter generation.

9. **P1: Add API server descriptors.**
   Implemented: `api_server` declarations parse, validate served API references, lower
   into IR, and emit binding descriptor metadata, route descriptors, request/response
   contexts, and handler interfaces. Next work is concrete HTTP framework adapters and
   route registration loops.

10. **P2: Add event runtime helpers.**
   Implemented: generated bindings now include event envelope types and event-specific
   payload builder helpers backed by each language binding's typed JSON value. Next
   work is backend event transport interfaces and generated publisher/subscriber
   integration.

11. **P2: Add worker scaffolding.**
    Implemented: generated bindings now expose worker descriptor lists, worker runtime
    contexts, and language-specific handler interfaces. Next work is generating runnable
    queue polling, lease acquisition, and workflow dispatch loops from worker, workflow,
    and queue IR.

## Pull Request Rule

Any PR that marks a matrix cell as more complete should update this document and include
tests for the affected pipeline stages. A construct should not be called `complete` until
it has parser/AST coverage, validation or semantic coverage where applicable, formatter
coverage, and at least one generator or explicit non-generator rationale.
