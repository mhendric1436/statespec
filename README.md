![Project Logo](images/statespec4.png)

# 📦 StateSpec

> **Design distributed systems with zero ambiguity.**

StateSpec is a **canonical language for distributed system design**. It unifies
**entities, APIs, workflows, queues, leases, state machines, and policies** into a
single deterministic specification.

A StateSpec file is intended to be the **source of truth** for a system. From that
source, tooling can generate schemas, APIs, workflow definitions, queue bindings,
lease declarations, validators, test scaffolding, diagrams, and runtime-specific
integration code.

---

# 🚀 Why StateSpec?

Modern distributed systems are difficult to reason about because critical behavior is
usually spread across many disconnected artifacts:

- OpenAPI documents describe APIs, but not lifecycle state.
- Database schemas describe storage, but not workflow behavior.
- Workflow definitions describe execution, but not entity invariants.
- Queue messages describe async work, but not ownership, retry, or policy.
- Lock and lease logic is often reimplemented differently in every subsystem.
- Prose design documents are ambiguous, inconsistent, and quickly outdated.

**StateSpec solves this by making system behavior explicit, structured, canonical, and
generator-friendly.**

---

# 🧠 Core Idea

> A system is a set of **entities with lifecycles**, manipulated by **APIs**,
> coordinated by **leases**, connected by **queues**, and executed by **workflows**.

StateSpec captures those concepts in one place:

| Concept | Purpose |
|---|---|
| **Entities** | Durable domain objects, state, relationships, indexes, invariants |
| **State machines** | Allowed lifecycle transitions and transition guards |
| **APIs** | External contracts and intent-oriented operations |
| **Workflows** | Long-running asynchronous orchestration |
| **Queues** | Durable async commands, events, and worker dispatch |
| **Leases** | Exclusive ownership, singleton workers, leader election, fencing |
| **Policies** | Tenant, authorization, quota, and operational constraints |
| **Generators** | Runtime-specific output from the canonical model |

---

# 🧩 StateSpec Ecosystem

StateSpec is designed to sit above a small set of focused C++20 runtime libraries.

```text
StateSpec  → canonical design language and generator toolchain
mt         → transactional entity/table/storage runtime
dl         → distributed locks and leases runtime
qu         → transactional queue/runtime messaging primitive
wf         → workflow orchestration runtime
```

A concise mental model:

```text
StateSpec defines the system.
mt stores the system state.
dl coordinates exclusive ownership.
qu dispatches durable work.
wf orchestrates long-running workflows.
```

---

# 🏗️ Runtime Stack

```text
                     ┌────────────────────────────┐
                     │         StateSpec           │
                     │ .sspec canonical language   │
                     └──────────────┬─────────────┘
                                    │
        ┌───────────────────────────┼───────────────────────────┐
        │                           │                           │
        ▼                           ▼                           ▼
┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
│ Entity Generator│       │Workflow Generator│       │ API Generator   │
└────────┬────────┘       └────────┬────────┘       └────────┬────────┘
         │                         │                         │
         ▼                         ▼                         ▼
┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
│ mt schemas      │       │ wf definitions  │       │ OpenAPI 3.1     │
│ mt mappings     │       │ WorkflowLogic   │       │ clients/servers │
└────────┬────────┘       └────────┬────────┘       └────────┬────────┘
         │                         │                         │
         └──────────────┬──────────┴──────────────┬──────────┘
                        │                         │
                        ▼                         ▼
              ┌─────────────────┐       ┌─────────────────┐
              │       mt        │       │       wf        │
              │ transactions    │       │ orchestration   │
              │ typed tables    │       │ workers/steps   │
              └───────┬─────────┘       └───────┬─────────┘
                      │                         │
        ┌─────────────┼─────────────┐           │
        ▼                           ▼           ▼
┌─────────────────┐       ┌─────────────────┐   │
│       dl        │       │       qu        │◄──┘
│ locks/leases    │       │ queues/messages │
└─────────────────┘       └─────────────────┘
```

---

# 🧱 Runtime Roles

## `mt` — Transactional Entity Runtime

`mt` is the storage and transaction substrate for StateSpec-generated entities.

StateSpec entities can generate:

```text
src/tables/schemas/<entity>.mt.json
src/tables/generated/<entity>_row.hpp
src/tables/generated/<entity>_mapping.hpp
```

`mt` provides:

- typed table access
- snapshot-style reads
- optimistic concurrency control
- predicate read validation
- backend-neutral storage
- generated schema metadata
- compatible schema evolution
- memory, SQLite, PostgreSQL, and future backend support

StateSpec should use `mt` for:

- entity rows
- lifecycle state persistence
- indexes
- invariant checks
- state transition guards
- atomic composition across workflows, queues, and leases

---

## `dl` — Distributed Locks and Leases Runtime

`dl` is the reusable coordination primitive for StateSpec-generated systems.

StateSpec leases can generate:

```text
generated/dl/<lease_name>_lease.hpp
generated/dl/<worker_name>_leader.hpp
```

`dl` provides:

- named resource leases
- exclusive ownership
- acquire, renew, release, and inspect operations
- expiry semantics
- monotonically increasing fencing tokens
- conflict detection through `mt`
- backend-neutral lease management

StateSpec should use `dl` for:

- singleton workers
- leader election
- exclusive reconcilers
- resource ownership
- service registration
- stale-owner protection
- common lease semantics shared by queues and workflows

---

## `qu` — Transactional Queue Runtime

`qu` is the durable queue and async messaging primitive for StateSpec-generated systems.

StateSpec queues and messages can generate:

```text
generated/qu/<queue_name>_queue.hpp
generated/qu/<message_name>_message.hpp
generated/qu/<queue_name>_worker.hpp
```

`qu` provides:

- enqueue with duplicate message protection
- claim with lease-style visibility timeout
- acknowledge / processed status
- fail and retry
- expired claim reaping
- namespace and channel scoping
- backend-neutral queue operations over `mt`
- caller-owned transaction overloads

StateSpec should use `qu` for:

- async command dispatch
- event handoff
- worker queues
- retryable background work
- transactional outbox-like flows
- workflow-to-worker communication
- queue operations committed atomically with entity and workflow state

---

## `wf` — Workflow Orchestration Runtime

`wf` is the workflow runtime for StateSpec-generated workflows.

StateSpec workflows can generate:

```text
generated/wf/<workflow_name>_workflow.json
generated/wf/<workflow_name>_logic.hpp
generated/wf/<workflow_name>_logic.cpp
```

`wf` provides:

- workflow definition parsing and validation
- workflow execution orchestration
- worker-oriented step execution
- atomic poll-and-claim
- lease-based step ownership
- keep-alive support
- cancellation
- background lease sweep
- `mt`-backed persistence
- HTTP REST API
- OpenAPI 3.1 specification
- CLI commands

StateSpec should use `wf` for:

- long-running workflows
- parent-child orchestration
- restart-safe step execution
- retries and failure handling
- worker pools
- workflow state tracking
- generated workflow definitions and `WorkflowLogic` scaffolding

---

# 🧬 Canonical Language Model

StateSpec should model these first-class concepts:

```text
system
entity
state_machine
api
workflow
step
queue
message
lease
worker
policy
```

The language should remain backend-neutral. It should not require users to write
runtime-specific concepts directly unless they intentionally select a runtime target.

Good:

```statespec
entity Order { ... }
workflow OrderProcessing { ... }
queue EmailQueue { ... }
lease OrderReconcilerLease { ... }
```

Avoid making the language depend directly on implementation names such as `mt::Table`,
`wf::WorkflowOrchestrator`, or `qu::Queue`.

Runtime-specific output belongs in generators.

---

# 📄 Example `.sspec`

```statespec
system OrderSystem {
  entity Order {
    key tenant_id, order_id

    fields {
      tenant_id string
      order_id string
      status string
      created_at timestamp
    }

    state_machine {
      Creating -> Active
      Creating -> Failed
      Active -> Updating
      Active -> Deleting
      Updating -> Active
      Updating -> Failed
      Deleting -> Deleted
      Deleting -> Failed
      Failed -> Deleting
    }
  }

  queue EmailQueue {
    namespace workflow
    channel email
    visibility_timeout PT30S

    message SendConfirmation {
      idempotency_key message_id

      payload {
        workflowExecutionId string
        orderId string
        email string
      }
    }
  }

  lease OrderReconcilerLease {
    resource "reconciler:orders"
    ttl PT30S
    renew_every PT10S
    fencing_token true
  }

  workflow OrderProcessing {
    version 1
    singleton false
    expected_execution_time PT5M
    start validate_order

    step validate_order {
      expected_execution_time PT10S
      max_retries 2
    }

    step charge_payment {
      expected_execution_time PT30S
      max_retries 3
    }

    step send_confirmation {
      expected_execution_time PT10S
      max_retries 3
      enqueue EmailQueue.SendConfirmation
    }
  }

  api StartOrderProcessing {
    method POST
    path "/v1/orders/{orderId}/start"
    starts workflow OrderProcessing
  }
}
```

---

# 🧩 What StateSpec Can Generate

From a single `.sspec` file, StateSpec can generate:

| Target | Generated artifacts |
|---|---|
| `mt` | entity schemas, row structs, table mappings, index metadata |
| `dl` | lease declarations, singleton worker helpers, fencing-token checks |
| `qu` | queue bindings, message payload types, enqueue helpers, worker helpers |
| `wf` | workflow definition JSON, workflow logic scaffolding, worker integration |
| OpenAPI | REST contracts, request/response schemas, error models |
| Protobuf / gRPC | RPC schemas and service definitions |
| Server stubs | Go, Java, Rust, C++, or other target languages |
| Tests | state transition tests, workflow tests, queue tests, lease tests |
| Documentation | architecture docs, diagrams, state machine graphs |
| VS Code | syntax highlighting, validation, autocomplete, symbol navigation |

---

# 💻 Planned CLI

```sh
statespec validate <file>
statespec fmt <file>
statespec generate <target> <file>
statespec graph <file>
statespec diff <old-file> <new-file>
```

Planned generator targets:

```sh
statespec generate mt       system.sspec
statespec generate dl       system.sspec
statespec generate qu       system.sspec
statespec generate wf       system.sspec
statespec generate openapi  system.sspec
statespec generate proto    system.sspec
statespec generate docs     system.sspec
statespec generate all      system.sspec
```

Example:

```sh
statespec generate all examples/order-system/order-system.sspec
```

Possible output layout:

```text
examples/order-system/
├── order-system.sspec
└── generated/
    ├── mt/
    │   ├── order.mt.json
    │   └── order_row.hpp
    ├── dl/
    │   └── order_reconciler_lease.hpp
    ├── qu/
    │   ├── email_queue.hpp
    │   └── send_confirmation_message.hpp
    ├── wf/
    │   ├── order-processing-workflow.json
    │   └── order_processing_logic.hpp
    ├── openapi/
    │   └── openapi.yaml
    └── docs/
        └── architecture.md
```

---

# ⚙️ Design Principles

StateSpec is built around **low-entropy system design**.

## 1. Canonical Structure

There should be one preferred way to express each concept.

## 2. Explicit State

All important lifecycle states and transitions should be defined explicitly.

## 3. Deterministic Behavior

Specifications should generate deterministic artifacts. Hidden side effects and implicit
transitions should be avoided.

## 4. Runtime Independence

The language should describe the system. Generators should decide how that system maps
to `mt`, `dl`, `qu`, `wf`, OpenAPI, Protobuf, or other targets.

## 5. Durable Coordination

Workflow communication should happen through durable entity state, durable queues, or
explicit leases, not transient in-memory execution state.

## 6. Separation of Concerns

StateSpec separates:

- domain model
- lifecycle model
- API surface
- workflow execution
- queue dispatch
- lease coordination
- storage realization
- security and policy

## 7. Text Is The Source Of Truth

Visual tools, diagrams, generated code, and runtime configuration should be derived from
the canonical text specification.

---

# 🔗 Parent-Child Model

StateSpec natively supports hierarchical systems:

- parent entities own child entities
- parent workflows create and orchestrate child workflows
- child workflows execute independently
- parent progress is derived from durable child entity state

> Workflows communicate through durable entity state, durable messages, and explicit
> coordination primitives.

---

# 🔄 Orchestration Model

StateSpec standardizes parent-child orchestration as a three-phase protocol:

```text
generate_child_ids
creating_children
waiting_children
```

This makes orchestration:

- idempotent
- observable
- restart-safe
- deterministic

---

# 🔐 Policy Model

StateSpec should eventually support policy declarations for hosted or multi-tenant
systems.

Examples:

```statespec
policy WorkflowAccess {
  tenant scoped_by tenant_id

  allow startWorkflowExecution when caller.role in ["operator", "control-plane"]
  allow pollAndClaimWorkflowSteps when caller.role == "worker"
}
```

Policies can generate:

- authorization hooks
- ownership checks
- tenant scoping rules
- quota checks
- audit event definitions
- service-layer validation scaffolding

Runtime libraries such as `mt`, `dl`, `qu`, and `wf` are intentionally focused on
backend-neutral primitives. Multi-tenant identity and authorization should be enforced
by the service layer generated or guided by StateSpec.

---

# 🧠 VS Code Extension

StateSpec is designed to be used with a Visual Studio Code extension.

Planned features:

- syntax highlighting
- real-time validation
- autocomplete
- symbol navigation
- entity graph visualization
- workflow graph visualization
- state machine visualization
- code generation commands
- jump-to-generated-artifact support

---

# 📂 File Extension

```text
.sspec
```

---

# 🎯 Use Cases

StateSpec is intended for systems where behavior must be precise, durable, and
reviewable:

- cloud control planes
- infrastructure services
- event-driven systems
- API + worker architectures
- distributed workflow systems
- systems with complex lifecycle management
- systems requiring explicit state transition rules
- systems using durable queues and leases
- internal platforms and service orchestration layers

---

# 🔍 Comparison

| Tool | Scope | Limitation |
|---|---|---|
| OpenAPI | APIs | No workflows, leases, queues, or lifecycle state |
| Protobuf | RPC schemas | No lifecycle or orchestration model |
| Terraform | Infrastructure | No runtime behavior |
| BPMN | Business workflows | Not ideal as a system implementation spec |
| UML | Diagrams | Not usually executable or canonical |
| Prose docs | Everything | Ambiguous, inconsistent, difficult to validate |
| StateSpec | System behavior | Intended to be canonical, structured, and generator-friendly |

---

# ✅ Milestone History

| Milestone | Status | Summary |
|---:|---|---|
| 1 | Complete | C++ skeleton |
| 2 | Complete | Lexer |
| 3 | Complete | Core parser |
| 4 | Complete | Ecosystem parser for entities, queues, leases, workers, workflows, APIs, policies, and generators |
| 5 | Complete | Symbol table and reference validation |
| 6 | Complete | Deeper semantic validation |
| 7 | Complete | AST JSON CLI dump and regression test |
| 8 | Complete | Code generation scaffold |
| 9 | Complete | CLI `generate` command |
| 10 | Complete | Real `mt` generator with entity structs and metadata |
| 11 | Complete | Real `qu` generator with queue/message payload structs and metadata |
| 12 | Complete | Real `dl` generator with lease and worker binding metadata |
| 13 | Complete | Real `wf` generator with workflow and step metadata |
| 14 | Complete | Consolidated generator library |
| 15 | Complete | Retired old monolithic generator from the active build |
| 16 | Planned | Real OpenAPI generator with full paths, schemas, request/response models, and problem details |

The current generator implementation is decomposed by target:

```text
src/generator.cpp          facade and target dispatch
src/generator_backend.hpp  shared generator backend helpers and declarations
src/generator_mt.cpp       mt backend
src/generator_dl.cpp       dl backend
src/generator_qu.cpp       qu backend
src/generator_wf.cpp       wf backend
src/generator_openapi.cpp  OpenAPI and generic scaffold backend
```

---

# 🛣️ Roadmap

## Language

- language spec v0.1
- parser
- validator
- formatter
- grammar documentation
- canonical examples

## Generators

- `mt` entity schema and mapping generator
- `dl` lease helper generator
- `qu` queue and message binding generator
- `wf` workflow definition and logic generator
- OpenAPI generator
- Protobuf / gRPC generator
- documentation generator
- diagram generator

## Runtime Integration

- integrated `mt` entity example
- integrated `dl` lease example
- integrated `qu` queue example
- integrated `wf` workflow example
- full order-system end-to-end example
- generated test scaffolding
- generated policy hooks

## Tooling

- CLI
- VS Code extension
- graph visualization
- generated artifact diffing
- compatibility checks for spec evolution

---

# 🤝 Contributing

See `CONTRIBUTING.md`.

---

# 📜 License

Apache 2.0 is recommended.

---

# 🧭 Positioning

StateSpec is a **canonical design language for distributed systems** that unifies APIs,
entities, state machines, workflows, queues, leases, and policies into a single
deterministic specification.

It is designed to generate and coordinate runtime artifacts for:

- `mt` transactional entity storage
- `dl` distributed locks and leases
- `qu` transactional queues
- `wf` workflow orchestration

---

# 🧠 Philosophy

> Systems fail when design is implicit.

StateSpec makes design explicit, structured, reviewable, and executable.

---

# 🚀 One-Line Summary

> From state machines to running systems.
