![Project Logo](images/statespec4.png)

# StateSpec

> A canonical design language and C++20 compiler/generator toolchain for backend-neutral distributed system specifications.

StateSpec describes distributed systems as durable state, explicit lifecycle transitions, asynchronous workflows, queues, leases, workers, APIs, policies, observability contracts, feature flags, and generator targets.

A `.sspec` file is intended to be the source of truth for system behavior. Tooling can validate it, inspect tokens and AST output, and generate language binding metadata for runtime libraries.

---

## Current Implementation

This repository is a C++20 implementation of the StateSpec compiler and generator toolchain.

```text
include/statespec/   public compiler and generator headers
src/                 lexer, parser, validator, and generator implementation
cmd/                 statespec CLI
tests/               C++ and shell-based regression tests
grammar/             StateSpec grammar reference
examples/            example .sspec files
bindings/            generated/runtime binding surfaces for supported languages
diagrams/            PlantUML architecture diagrams
docs/                design notes and binding documentation
```

The build produces:

```text
build/libstatespec.a       compiler and generator library
build/bin/statespec        CLI
build/bin/statespec_tests  regression test binary
```

---

## Build and Test

```sh
make all
make test
make cli
make help
```

Useful maintenance targets:

```sh
make format
make format-check
make test-bindings
make diagrams-png
make clean
```

`make diagrams-png` renders `diagrams/*.puml` into PNG files with PlantUML. The target uses these variables:

```text
PLANTUML ?= plantuml
PLANTUML_FLAGS ?= -DPLANTUML_LIMIT_SIZE=16384 -Xmx512m
```

---

## CLI

```sh
statespec help
statespec validate <file.sspec>
statespec tokens <file.sspec>
statespec ast <file.sspec>
statespec generate bindings --lang <cpp|go|java|rust> <file.sspec> [--out DIR]
```

The CLI currently supports validation, token inspection, AST inspection, and binding generation for C++, Go, Java, and Rust.

---

## Language Model

StateSpec models these top-level and nested concepts:

```text
statespec version
include
import
system
tenant_scope
system_tenant
namespace
value
enum
shape
external_system
feature_flag
entity
state_machine
state
transition
garbage_collection
queue
message
lease
worker
api
api_server
workflow
step
policy
log
metric
generate
```

The language is backend-neutral. Specifications describe system semantics rather than binding themselves to a specific storage engine, queue implementation, workflow runtime, or transport.

---

## Core Concepts

| Concept | Purpose |
|---|---|
| Tenant scope | Default tenant field and system tenant configuration |
| Namespaces | Logical grouping for generated/runtime resources |
| Values, enums, shapes | Reusable typed model definitions |
| External systems | Named integration dependencies and properties |
| Feature flags | Persisted, scoped runtime behavior controls |
| Entities | Durable domain objects, keys, fields, indexes, relationships, invariants, and state machines |
| State machines | Lifecycle states, initial state, terminal states, transitions, and terminal cleanup policies |
| Queues and messages | Durable async dispatch with channels, visibility timeouts, retry limits, and typed payloads |
| Leases | Exclusive ownership, renewal, max TTL, holders, and fencing tokens |
| Workers | Queue polling, workflow execution, singleton mode, leases, and concurrency |
| APIs | External operations, request/response/error shapes, workflow starts, and message enqueue behavior |
| API servers | Runtime actors that serve one or more declared APIs with concurrency metadata |
| Workflows | Versioned long-running orchestration with steps, loads, statements, retry limits, and expected execution time |
| Policies | Tenant scoping, allow/deny rules, quotas, and audit declarations |
| Logs and metrics | Stable operational event and measurement contracts |
| Generators | Deterministic output targets from the canonical model |

---

## Example

See [`examples/order-system.sspec`](examples/order-system.sspec) for a complete example. A shortened shape is shown below.

```statespec
statespec 0.1;

system OrderSystem {
  tenant scoped_by tenant_id
  system_tenant configured

  entity Order {
    key tenant_id, order_id
    ownership {
      authority: system
      system_of_record: self
      lifecycle: authoritative
    }

    fields {
      created_at timestamp
      updated_at timestamp
      status string
      tenant_id string
      order_id string
      customer_id string
      total_cents int64
    }

    state_machine {
      state Created
      state Validated
      state PaymentPending
      state Paid
      state Fulfilled {
        terminal: true
        garbage_collection {
          after: P90D
          mode: archive
        }
      }
      state Cancelled {
        terminal: true
        garbage_collection {
          after: P30D
          mode: tombstone
        }
      }
      state Failed {
        terminal: true
        garbage_collection {
          after: P30D
          mode: tombstone
        }
      }

      initial Created
      terminal [Fulfilled, Cancelled, Failed]

      Created -> Validated
      Validated -> PaymentPending
      PaymentPending -> Paid
      Paid -> Fulfilled
      Created -> Cancelled
      PaymentPending -> Failed
    }
  }

  queue OrderEvents {
    namespace workflow_ns
    channel order-events
    visibility_timeout PT30S
    max_attempts 5

    message OrderValidated {
      idempotency_key message_id
      payload {
        message_id string
        tenant_id string
        order_id string
      }
    }
  }

  lease OrderWorkflowLease {
    resource "workflow:order-processing"
    ttl PT30S
    renew_every PT10S
    holder worker_id
    fencing_token true
    max_ttl PT5M
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
  }

  worker OrderWorkflowWorker {
    singleton true
    lease OrderWorkflowLease
    polls OrderEvents.OrderValidated
    executes OrderProcessing
    concurrency 4
  }

  api StartOrderProcessing {
    method POST
    path "/v1/tenants/{tenantId}/orders/{orderId}/start"
    input StartOrderProcessingRequest
    output StartOrderProcessingResponse
    error ProblemDetails
    starts workflow OrderProcessing
  }

  api_server OrderApi {
    serves StartOrderProcessing
    concurrency 16
  }

  policy OrderAccess {
    tenant scoped_by tenant_id
    allow StartOrderProcessing when caller.role == operator;
    deny StartOrderProcessing when caller.suspended == true;
    quota starts_per_minute: 60;
    audit StartOrderProcessing;
  }

  generate mt { out "generated/mt" }
  generate dl { out "generated/dl" }
  generate qu { out "generated/qu" }
  generate wf { out "generated/wf" }
  generate openapi { out "generated/openapi" }
}
```

---

## Feature Flags

Feature flags are first-class system declarations. They are modeled as persisted-state runtime controls so generated services and workflows can evaluate them inside the same optimistic-concurrency transaction as other behavior-affecting reads.

A feature flag can describe:

```text
name
type
default
scope
owner
description
expires
```

Feature flag runtime support is represented in the binding/OCC model through a `FeatureFlagStore` with transaction-aware evaluation APIs.

---

## Backend Abstraction and OCC Model

StateSpec defines an OCC-centered backend abstraction so generated runtimes can share one consistency doctrine across concrete implementations.

The core backend model includes:

```text
CollectionDescriptor
FieldDescriptor
IndexDescriptor
BackendCapabilities
ConflictKind
VersionedRecord
Query
Transaction
Backend
```

The common rule is:

```text
read versioned state
validate predicates and invariants
prepare deterministic changes
commit only if observed versions are still current
retry safely on conflict
```

Runtime component APIs support backend-managed calls and caller-managed transaction calls. Caller-managed variants use a `Tx` suffix, adapted to each language's naming conventions.

---

## Language Bindings

Reference backend and runtime component bindings are provided for:

| Language | Binding guide |
|---|---|
| C++ | [`bindings/cpp/README.md`](bindings/cpp/README.md) |
| Go | [`bindings/go/README.md`](bindings/go/README.md) |
| Java | [`bindings/java/README.md`](bindings/java/README.md) |
| Rust | [`bindings/rust/README.md`](bindings/rust/README.md) |

Shared binding documentation is in [`docs/backend-abstractions.md`](docs/backend-abstractions.md).

Generated binding packages include separate runtime surfaces for backend operations, queues, leases, workflows, logs, metrics, and feature flags where supported by the binding.

---

## Diagrams

PlantUML source diagrams are maintained in [`diagrams/`](diagrams/):

```text
diagrams/compiler-pipeline.puml
diagrams/language-model.puml
diagrams/runtime-occ-bindings.puml
```

Generate PNG diagrams with:

```sh
make diagrams-png
```

---

## Design Principles

### Canonical structure

There should be one preferred way to express each concept.

### Explicit state

Important lifecycle states, transitions, ownership rules, retry behavior, feature flag decisions, and invariants should be represented directly in the specification.

### Deterministic behavior

Generated artifacts should be deterministic. Hidden side effects and ambiguous transitions should be avoided.

### Backend independence

The language should describe the system without depending on a concrete runtime, storage engine, database, or transport.

### Durable coordination

Workflow communication, queue dispatch, and exclusive ownership should be expressed in terms of durable state, durable messages, or explicit leases rather than transient in-memory coordination.

### Declared observability

Logs and metrics should be declared as stable contracts with typed fields and labels rather than hidden in runtime implementation code. In tenant-scoped systems, log fields and metric labels must include the system tenant field. Metric labels must stay low-cardinality.

### Text is the source of truth

Visual tools, diagrams, generated code, runtime configuration, and tests should be derived from the canonical text specification.

---

## What StateSpec Can Generate

From a single `.sspec` file, StateSpec tooling can derive:

| Artifact type | Examples |
|---|---|
| Schemas | entity metadata, collection descriptors, indexes, validation metadata |
| APIs | OpenAPI, RPC contracts, request/response models, error models |
| Runtime metadata | workflow, queue, lease, worker, API server, log, metric, and feature flag definitions |
| Code scaffolding | server stubs, client stubs, integration helpers, worker skeletons |
| Tests | state transition tests, invariant tests, runtime behavior tests |
| Documentation | architecture docs, diagrams, state machine graphs |
| Tooling | syntax highlighting, validation, autocomplete, symbol navigation |

---

## File Extension

```text
.sspec
```

---

## Use Cases

StateSpec is intended for systems where behavior must be precise, durable, and reviewable:

```text
cloud control planes
infrastructure services
event-driven systems
API and worker architectures
distributed workflow systems
systems with complex lifecycle management
systems requiring explicit state transition rules
internal platforms and service orchestration layers
```

---

## Comparison

| Tool | Scope | Limitation |
|---|---|---|
| OpenAPI | APIs | Does not model lifecycle state, queues, leases, workers, policies, feature flags, logs, metrics, or workflows |
| Protobuf | RPC schemas | Does not model lifecycle or orchestration semantics |
| Terraform | Infrastructure | Does not model runtime behavior |
| BPMN | Business workflows | Not usually a system implementation specification |
| UML | Diagrams | Not usually executable or canonical |
| Prose docs | Everything | Ambiguous, inconsistent, difficult to validate |
| StateSpec | System behavior | Structured, canonical, generator-friendly |

---

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md).

---

## License

See [`LICENSE.md`](LICENSE.md).

---

## Philosophy

> Systems fail when design is implicit.

StateSpec makes design explicit, structured, reviewable, and executable.

---

## One-Line Summary

> From state machines to backend-neutral distributed systems.
