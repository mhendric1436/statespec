# StateSpec Programmer Guide

This directory contains practical guidance for programmers who author `.sspec` files.

StateSpec is a canonical language for describing distributed systems with explicit
entities, lifecycle state, APIs, API servers, workflows, queues, leases, workers, feature
flags, logs, metrics, and policies. A `.sspec` file should be treated as the source of
truth for system behavior.

## Guide Structure

| File | Purpose |
|---|---|
| [quick-start.md](quick-start.md) | Command-oriented walkthrough for validating examples, generating bindings/OpenAPI, linking generated apps with the in-memory backend, and understanding generated API and Worker apps. |
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
| [generated-extension-points.md](generated-extension-points.md) | User-owned API, worker, workflow step, and operator metadata implementation boundaries. |
| [api-process-lifecycle.md](api-process-lifecycle.md) | Cross-language generated API process threading, startup, shutdown, and join contract. |
| [backend-abstractions.md](backend-abstractions.md) | OCC-centered backend abstraction source artifacts and runtime contracts. |
| [schema-upgrades.md](schema-upgrades.md) | Backwards-compatible collection descriptor upgrade rules for backend adapters. |
| [runtime-store-contract.md](runtime-store-contract.md) | Backend-neutral contract for feature flag, queue, lease, workflow, log, and metric stores. |
| [in-memory-backend.md](in-memory-backend.md) | Cross-language in-memory backend contract, generated paths, and local API plus Worker app linking guidance. |
| [style-guide.md](style-guide.md) | Naming, ordering, and maintainability conventions for `.sspec` files. |

## Recommended Reading Order

1. Start with [quick-start.md](quick-start.md) to run the CLI against checked-in examples.
2. Read [getting-started.md](getting-started.md) for the smallest authoring loop.
3. Read [language-model.md](language-model.md) to understand how declarations fit together.
4. Check [compiler-parity.md](compiler-parity.md) before relying on newer grammar constructs.
5. Use the focused guides while authoring specific sections.
6. Read [generated-extension-points.md](generated-extension-points.md) before wiring generated code into runtime applications.
7. Read [api-process-lifecycle.md](api-process-lifecycle.md) before changing generated API startup behavior.
8. Read [backend-abstractions.md](backend-abstractions.md) when implementing runtimes or backend adapters.
9. Read [schema-upgrades.md](schema-upgrades.md) before implementing collection registration in a backend adapter.
10. Read [in-memory-backend.md](in-memory-backend.md) before using generated local/test backend adapters.
11. Use [style-guide.md](style-guide.md) before submitting `.sspec` changes for review.

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
