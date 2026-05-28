# Entity Garbage Collection Runtime

Entity garbage collection is a shared generated runtime feature. It is not API-only and
it is not Worker-only. The common tier owns the descriptors and reusable GC worker model,
while API and Worker generated applications may both compose the same shared workers.

## Runtime Model

StateSpec generates entity GC runtime artifacts only when at least one entity declares a
terminal state with `garbage_collection` metadata.

Generated common-tier runtime artifacts own:

- entity GC descriptors derived from terminal state metadata
- backend-neutral repository helper contracts for eligibility scans and finalization
- one reusable GC worker model that is registered once per generated entity GC descriptor
- shared defaults for polling and batch size

Generated API and Worker tiers own lifecycle wiring:

- register generated GC tasks before process start
- start GC workers when the process/runtime starts and GC is enabled
- request stop on GC workers during shutdown
- join GC workers before process/runtime shutdown completes

GC is enabled by default in generated process/runtime config, but it is an explicit
deployment choice:

| Tier | Config |
|---|---|
| C++ API | `ApiProcessConfig::entity_gc_enabled` |
| C++ Worker | `WorkerRuntimeConfig::entity_gc_enabled` |
| Go API | `APIProcessConfig.EntityGCEnabled` |
| Go Worker | `WorkerTierRuntimeConfig.EntityGCEnabled` |
| Java API | `ApiProcess.Config.entityGcEnabled()` |
| Java Worker | `WorkerRuntime.Config.entityGcEnabled()` |
| Rust API | `ApiProcessConfig::entity_gc_enabled` |
| Rust Worker | `WorkerRuntimeConfig::entity_gc_enabled` |

Set the flag to `false` on the tier that should not host entity GC. This is the
recommended way to run API and Worker deployments side by side without duplicate
background GC scans.

## Worker Shape

The baseline generated design is one low-resource registered task per generated entity
GC descriptor. Each registered task:

- scans a bounded number of eligible rows through the generic OCC query path
- revalidates terminal state inside the transaction that finalizes or removes the entity
- uses only generic `Backend` / `Transaction` APIs

The generated `ApiProcess` or `WorkerProcess` owns the stop-aware timed wait, polling
interval, start, stop, and join behavior for those tasks.

Default runtime values:

| Setting | Default |
|---|---:|
| Minimum poll interval | 60 seconds |
| Batch size | 100 records |

No scheduler abstraction is generated for v0. The generated API process and Worker
runtime own the stop-aware loop, and the common GC registration helper only registers
plain tasks with those lifecycle owners.

## API And Worker Composition

API-only applications can run GC without requiring a Worker deployment. Worker-only
applications can also run GC. Deployments that run both API and Worker processes should
configure which tier hosts GC to avoid unnecessary duplicate work.

Recommended deployment shapes:

| Deployment | Recommended GC host | Config shape |
|---|---|---|
| API-only app | API tier | leave API GC enabled |
| Worker-only app | Worker tier | leave Worker GC enabled |
| Mixed API + Worker app | one tier only | disable GC on the non-host tier |

For mixed deployments, choose the tier that is already closest to lifecycle maintenance
work. If the API tier handles most entity lifecycle changes and must work without
workers, host GC in API. If workers already perform background reconciliation, host GC in
Worker. The generated workers are OCC-safe, so duplicate attempts are tolerated, but
running both tiers as GC hosts wastes backend scans and should not be the default
production posture.

Duplicate GC attempts must still be harmless:

- candidate records are revalidated transactionally
- missing or already-collected records are treated as no-op outcomes
- OCC conflicts cause the current batch item to be skipped or retried later
- jitter reduces lockstep scans across replicas

Lease coordination and a dedicated scheduler abstraction are intentionally not part of
the baseline. They can be added later if large deployments show measurable GC
contention.

Entity GC must not depend on leases, queues, workflows, feature flags, logs, or metrics
for correctness. It is limited to the generic OCC backend and transaction primitives.
That constraint prevents circular runtime dependencies, such as GC requiring leases
while lease records themselves require GC.

## Artifact Model

The shared artifact model reserves common-tier generated files for all languages:

| Language | Descriptor artifact | Repository contract | Worker artifact | Registration artifact |
|---|---|---|---|---|
| C++ | `common/runtime/entity_gc_descriptors.hpp` | `common/runtime/entity_gc_repository.hpp` | `common/runtime/entity_gc_workers.hpp` | `common/runtime/entity_gc_registration.hpp` |
| Go | `common/backend/runtime/entity_gc_descriptors.go` | `common/backend/runtime/entity_gc_repository.go` | `common/backend/runtime/entity_gc_workers.go` | `common/backend/runtime/entity_gc_registration.go` |
| Java | `common/com/statespec/backend/runtime/EntityGcDescriptors.java` | `common/com/statespec/backend/runtime/EntityGcRepository.java` | `common/com/statespec/backend/runtime/EntityGcWorkers.java` | `common/com/statespec/backend/runtime/EntityGcRegistration.java` |
| Rust | `common/runtime/entity_gc_descriptors.rs` | `common/runtime/entity_gc_repository.rs` | `common/runtime/entity_gc_workers.rs` | `common/runtime/entity_gc_registration.rs` |

These artifacts are shared by generated API and Worker apps. The registration artifact
iterates the generated descriptor list, creates an OCC-backed repository and worker for
each descriptor, and registers one plain task per descriptor. The common worker exposes
a small `runOnce`/`run_once` surface for one entity descriptor and leaves lifecycle
thread ownership to the generated API or Worker app composition layer.

Generated API processes expose an entity GC worker registration hook and own the
background lifecycle after `start()` is called. Registered GC workers run on generated
background threads or goroutines, receive stop requests through the same process shutdown
path as the API transport, and are joined before `join()` completes. The concrete
repository implementation that supplies eligible rows remains backend/runtime-owned.

Generated Worker processes use the same lifecycle shape. `WorkerProcess.start()` starts
Worker-hosted entity GC when enabled, starts generated workflow polling loops, and
`WorkerProcess.join()` waits for both GC workers and workflow loops to stop. This keeps
GC usable in API-only, Worker-only, and mixed deployments without moving GC into
workflow code.

The generated architecture diagrams show this composition:

- [Generated API app architecture](../diagrams/generated-api-app-architecture.puml)
- [Generated Worker app architecture](../diagrams/generated-worker-app-architecture.puml)
- [Generated system block diagram](../diagrams/generated-system-block-diagram.puml)
