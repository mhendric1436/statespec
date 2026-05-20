# StateSpec Design History

This document captures the initial design exploration and architectural decisions for StateSpec.

## Project Vision

StateSpec is a canonical design language for distributed systems that unifies:

- entities
- state machines
- APIs
- workflows
- orchestration
- parent/child relationships
- ownership semantics

The primary design goal is to minimize ambiguity and reduce cross-entropy in software design specifications.

---

## Foundational Concepts

### Information-Theoretic Basis

The language design was inspired by Shannon Information Theory and the relationship between:

- entropy
- predictability
- cross-entropy reduction
- probabilistic modeling
- canonical structure

A major guiding principle became:

> Every feature should reduce ambiguity rather than introduce multiple equivalent representations.

---

## Core Language Principles

StateSpec was designed around:

- explicit state machines
- deterministic workflows
- canonical formatting
- strongly typed entities
- parent/child orchestration
- durable workflow state
- restart-safe orchestration
- text-first design
- VS Code extension-first tooling

The language intentionally avoids:

- hidden defaults
- implicit transitions
- multiple equivalent syntaxes
- runtime-only semantics
- ambiguous orchestration patterns

---

## Entity Model

Entities were defined as:

- durable objects with identity
- explicit lifecycle state machines
- invariant-constrained
- ownership-aware
- relationship-aware

Canonical entity sections:

- key
- ownership
- version
- state
- relations
- children
- fields
- transitions
- invariants
- indexes
- annotations

---

## Ownership Semantics

The language introduced first-class ownership semantics.

Supported ownership modes:

- authoritative
- managed
- observed
- projected

This distinction enables modeling:

- fully system-owned resources
- externally-owned resources
- resources created through external REST APIs
- projection/read-model entities

The design emphasized separating:

- local orchestration entities
- externally authoritative resources

when appropriate.

---

## Parent / Child Relationships

Parent-child relationships became a first-class language construct.

Children declare:

- typed parent references
- ownership semantics
- delete behavior
- parent state constraints
- uniqueness within parent

Parents may declare child collections for readability and generation.

This model supports:

- orchestration
- aggregate state management
- nested API generation
- lifecycle validation

---

## Orchestration Model

A major design milestone was defining the standardized orchestration pattern.

The workflow model is:

- parent workflow creates child entities
- child workflows execute independently
- parent workflow monitors child entity state
- orchestration progress is durable and restart-safe

The finalized orchestration protocol contains three phases:

1. generate_child_ids
2. creating_children
3. waiting_children

### generate_child_ids

- atomically reserves child IDs
- initializes orchestration buckets
- ensures idempotent fan-out

### creating_children

- materializes child entities
- launches child workflows
- moves child IDs from pending to creating

### waiting_children

- observes child entity state
- reconciles child completion/failure
- aggregates parent success/failure

The parent entity stores durable orchestration buckets:

- pendingChildIds
- creatingChildIds
- succeededChildIds
- failedChildIds

This became one of the defining architectural patterns of StateSpec.

---

## Workflow Philosophy

The workflow model intentionally avoids coupling workflows directly together.

Instead:

> Workflows communicate through durable entity state.

This design improves:

- observability
- restart safety
- reconciliation
- deterministic recovery
- distributed execution

---

## Tooling Direction

The project intentionally adopted:

- text-first specifications
- canonical formatting
- VS Code extension-first UX

Instead of a standalone visual designer.

The VS Code extension direction includes:

- syntax highlighting
- validation
- autocomplete
- graph visualization
- synchronized text + graph editing

The text specification remains the source of truth.

---

## Repository Architecture

The recommended repository structure includes:

- grammar/
- crates/
- packages/
- examples/
- docs/
- testdata/

The architecture separates:

- parser
- AST
- semantic analyzer
- canonical IR
- generators
- LSP
- VS Code extension

The implementation direction favors:

- Rust for compiler/runtime/tooling core
- TypeScript for VS Code extension

---

## Licensing Discussion

MIT and Apache 2.0 were compared.

MIT was selected initially for:

- simplicity
- permissiveness
- low overhead

Apache 2.0 was recognized as stronger for patent protection and enterprise ecosystems.

---

## Generated Project Files

The initial project bootstrap included generation of:

- README.md
- CONTRIBUTING.md
- AGENTS.md
- Makefile
- statespec.ebnf

These established:

- repo structure
- contribution philosophy
- workflow semantics
- grammar foundation
- tooling direction

---

## Key Doctrine

Several key architectural doctrines emerged.

### Canonical Representation

There must be one preferred way to express each concept.

### Explicit Lifecycle

All orchestrated entities must have explicit lifecycle transitions.

### Durable Orchestration

Workflow progress must be recoverable from entity state.

### Parent-Driven Orchestration

Parents orchestrate children through durable child entities.

### Text-First Design

Visual tooling augments the language but does not replace the canonical text representation.

---

## Project Tagline

> From state machines to running systems.
