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
   - Generated OpenAPI, protobuf, server stubs, worker skeletons, migrations, diagrams, and tests are outputs of the spec.
   - Generators consume canonical IR, not raw AST.

---

## Recommended Repository Structure

```text
statespec/
├── README.md
├── ROADMAP.md
├── CONTRIBUTING.md
├── AGENTS.md
├── Makefile
├── Cargo.toml
├── pnpm-workspace.yaml
├── grammar/
│   └── statespec.ebnf
├── examples/
│   └── cluster-service/
│       └── cluster-service.sspec
├── crates/
│   ├── statespec-ast/
│   ├── statespec-parser/
│   ├── statespec-semantic/
│   ├── statespec-ir/
│   ├── statespec-formatter/
│   ├── statespec-cli/
│   ├── statespec-lsp/
│   ├── statespec-gen-openapi/
│   ├── statespec-gen-protobuf/
│   └── statespec-gen-go/
├── packages/
│   └── vscode-statespec/
├── docs/
│   ├── language/
│   └── tooling/
├── schemas/
└── testdata/
```

---

## Architecture

The intended pipeline is:

```text
.sspec text
    ↓
parser
    ↓
AST
    ↓
semantic analyzer
    ↓
canonical IR
    ↓
formatter / CLI / LSP / generators / VS Code extension
```

### Key rule

Generators, diagrams, and editor features should consume the semantic model or canonical IR wherever possible. Avoid building features directly against raw text unless the feature is purely syntactic.

---

## Language Concepts

StateSpec v0 includes these top-level concepts:

- `system`
- `state`
- `value`
- `external_system`
- `entity`
- `event`
- `api`
- `workflow`

Entities are durable system objects. Workflows describe asynchronous execution. APIs expose external contracts. Events connect APIs, workflows, and workers.

---

## Entity Rules

Entities represent durable objects with identity, ownership, lifecycle, fields, relationships, and invariants.

Canonical entity shape:

```sspec
entity Name {
  key id: uuid

  ownership {
    authority: system
    system_of_record: self
    lifecycle: authoritative
  }

  version: int
  state: NameState

  relations { }
  children { }
  fields { }
  transitions { }
  invariants { }
  indexes { }
  annotations { }
}
```

### Entity invariants

- Every entity must have exactly one `key` in v0.
- Entity names use PascalCase.
- Field names use lowerCamelCase.
- State fields must reference declared `state` types.
- State transitions must reference valid state members.
- Workflows may not introduce undeclared state transitions.

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

## Parent/Child Relationships

Parent-child relationships are first-class.

The child entity declares the relationship in `relations`:

```sspec
relations {
  parent clusterId: ref<Cluster> {
    kind: composition
    on_parent_delete: cascade
    unique_within_parent: [ordinal]
  }
}
```

The parent may optionally declare an inverse view:

```sspec
children {
  nodes: Node by clusterId
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
pendingChildIds: list<uuid>
creatingChildIds: list<uuid>
succeededChildIds: list<uuid>
failedChildIds: list<uuid>
```

Prefer `succeededChildIds` over `activeChildIds`, because not every child success terminal state is named `Active`.

### Required invariants

Child ID buckets must be disjoint and conserved:

```sspec
invariants {
  childIdsPartitioned:
    disjoint(pendingChildIds, creatingChildIds, succeededChildIds, failedChildIds)

  childIdsConserved:
    count(pendingChildIds) +
    count(creatingChildIds) +
    count(succeededChildIds) +
    count(failedChildIds) == desiredChildCount
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

---

## Formatter Guidelines

Formatter output must be deterministic.

Canonical ordering inside `entity`:

1. `key`
2. `ownership`
3. `version`
4. `state`
5. `relations`
6. `children`
7. `fields`
8. `transitions`
9. `invariants`
10. `indexes`
11. `annotations`

Canonical ordering inside `workflow`:

1. `on`
2. `load`
3. `require`
4. `child_set`
5. `step`

Do not introduce formatting options until there is a strong reason. One canonical style is preferred.

---

## CLI Guidelines

The CLI should expose simple commands:

```bash
statespec validate
statespec fmt
statespec generate
statespec graph
statespec diff
statespec doctor
statespec lsp
```

CLI behavior should be suitable for CI.

- `validate` exits non-zero on syntax or semantic errors.
- `fmt --check` exits non-zero if files are not formatted.
- `generate --check` exits non-zero if generated output is stale.

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
- protobuf
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

Generated output should include a header indicating it is generated from StateSpec.

---

## Testing Guidelines

Use golden tests for:

- parser output
- formatter output
- semantic diagnostics
- generated artifacts
- graph output

Suggested testdata layout:

```text
testdata/
├── parser/
├── semantic/
├── formatter/
├── generators/
└── lsp/
```

Every language feature should have:

- at least one valid example
- at least one invalid example
- formatter coverage
- semantic validation coverage

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
docs/language/overview.md
docs/language/grammar.md
docs/language/entities.md
docs/language/workflows.md
docs/language/ownership.md
docs/tooling/cli.md
docs/tooling/vscode-extension.md
```

Any grammar or semantic change should update docs in the same PR.

---

## Naming Conventions

- Entities: `PascalCase`
- States: `PascalCase`
- State members: `PascalCase`
- APIs: `PascalCase`
- Workflows: `PascalCase`
- Events: `PascalCase`
- Fields: `lowerCamelCase`
- Steps: `snake_case`
- Child sets: `lowerCamelCase` or plural lowerCamelCase

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
- adding dynamic typing
- adding untyped `any` as an escape hatch
- putting business semantics in annotations
- letting generators define semantics
- letting the VS Code extension diverge from CLI validation
- creating standalone diagram state separate from `.sspec`
- modeling external resources as locally authoritative
- launching child workflows without durable child entity state

---

## Current Design Biases

StateSpec is optimized for:

- distributed systems
- REST API + worker architectures
- control planes
- lifecycle-managed resources
- parent-child orchestration
- idempotent async workflows
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
