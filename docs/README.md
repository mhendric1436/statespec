# StateSpec Programmer Guide

This directory contains practical guidance for programmers who author `.sspec` files.

StateSpec is a canonical language for describing distributed systems with explicit
entities, lifecycle state, APIs, workflows, queues, leases, workers, policies, and
generator targets. A `.sspec` file should be treated as the source of truth for system
behavior.

## Guide Structure

| File | Purpose |
|---|---|
| [getting-started.md](getting-started.md) | Minimal workflow for creating, validating, and generating from a `.sspec` file. |
| [language-model.md](language-model.md) | The core StateSpec mental model and declaration types. |
| [entities-and-state.md](entities-and-state.md) | Entity, field, key, state machine, relationship, invariant, and index authoring. |
| [queues-leases-workers.md](queues-leases-workers.md) | Durable queue, message, lease, and worker declarations. |
| [workflows-and-apis.md](workflows-and-apis.md) | Workflow, step, API, behavior, and orchestration declarations. |
| [policies.md](policies.md) | Policy authoring for tenant scoping, authorization rules, quotas, and audit points. |
| [generators.md](generators.md) | Generator targets and expected output layout. |
| [backend-abstractions.md](backend-abstractions.md) | OCC-centered backend abstraction source artifacts and runtime contracts. |
| [style-guide.md](style-guide.md) | Naming, ordering, and maintainability conventions for `.sspec` files. |

## Recommended Reading Order

1. Start with [getting-started.md](getting-started.md).
2. Read [language-model.md](language-model.md) to understand how declarations fit together.
3. Use the focused guides while authoring specific sections.
4. Read [backend-abstractions.md](backend-abstractions.md) when implementing runtimes or backend adapters.
5. Use [style-guide.md](style-guide.md) before submitting `.sspec` changes for review.

## Source Of Truth

The grammar reference lives in [`grammar/statespec.ebnf`](../grammar/statespec.ebnf).
These guides explain how to author useful specifications; the grammar remains the
canonical syntax reference.

## Current Runtime Mapping

The StateSpec runtime ecosystem uses these target names:

```text
mt       transactional entity/table/storage runtime
dl       distributed locks and leases runtime
qu       transactional queue/runtime messaging primitive
wf       workflow orchestration runtime
openapi  REST API contract generation
all      expand to the concrete generator targets
```

Use generator declarations or the CLI `generate` command to lower a `.sspec` file into
runtime artifacts.
