# StateSpec Programmer Guide

This directory contains practical guidance for programmers who author `.sspec` files.

StateSpec is a canonical language for describing distributed systems with explicit
entities, lifecycle state, APIs, API servers, workflows, queues, leases, workers, feature
flags, logs, metrics, and policies. A `.sspec` file should be treated as the source of
truth for system behavior.

## Guide Structure

| File | Purpose |
|---|---|
| [getting-started.md](getting-started.md) | Minimal workflow for creating, validating, and generating from a `.sspec` file. |
| [language-model.md](language-model.md) | The core StateSpec mental model and declaration types. |
| [compiler-parity.md](compiler-parity.md) | Grammar-to-compiler implementation matrix and priority order. |
| [roadmap.md](roadmap.md) | Current implementation roadmap and runtime-owned boundaries. |
| [includes.md](includes.md) | Include directives, path resolution, system composition, and file-local tooling behavior. |
| [entities-and-state.md](entities-and-state.md) | Entity, field, key, state machine, relationship, invariant, and index authoring. |
| [queues-leases-workers.md](queues-leases-workers.md) | Durable queue, message, lease, and worker declarations. |
| [workflows-and-apis.md](workflows-and-apis.md) | Workflow, step, API, API server, behavior, and orchestration declarations. |
| [workflow-launch-control.md](workflow-launch-control.md) | Durable launch queue, capacity, reservation, and singleton scheduler pattern for rate-controlled workflow starts. |
| [external-system-metadata.md](external-system-metadata.md) | Tenant-scoped operator metadata model for external system runtime configuration. |
| [expressions.md](expressions.md) | Expression operators, syntax shape, and allowed built-in functions. |
| [feature-flags.md](feature-flags.md) | Feature flag declarations, rollout scopes, and expression usage. |
| [observability.md](observability.md) | Log and metric declarations, generated descriptors, and runtime binding contracts. |
| [policies.md](policies.md) | Policy authoring for tenant scoping, authorization rules, quotas, and audit points. |
| [generators.md](generators.md) | CLI-selected binding generation and expected output layout. |
| [backend-abstractions.md](backend-abstractions.md) | OCC-centered backend abstraction source artifacts and runtime contracts. |
| [style-guide.md](style-guide.md) | Naming, ordering, and maintainability conventions for `.sspec` files. |

## Recommended Reading Order

1. Start with [getting-started.md](getting-started.md).
2. Read [language-model.md](language-model.md) to understand how declarations fit together.
3. Check [compiler-parity.md](compiler-parity.md) before relying on newer grammar constructs.
4. Use the focused guides while authoring specific sections.
5. Read [backend-abstractions.md](backend-abstractions.md) when implementing runtimes or backend adapters.
6. Use [style-guide.md](style-guide.md) before submitting `.sspec` changes for review.

## Source Of Truth

The grammar reference lives in [`grammar/statespec.ebnf`](../grammar/statespec.ebnf).
These guides explain how to author useful specifications; the grammar remains the
canonical syntax reference.

## Current Runtime Mapping

The binding generator supports these language names:

```text
cpp
go
java
rust
```

Use `statespec generate bindings --lang <language>` to lower a validated `.sspec` file
into language bindings. Use `statespec generate openapi` to emit the OpenAPI contract
for declared APIs and generated operator metadata APIs.
