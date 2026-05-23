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
- one lightweight GC worker model per GC-enabled entity
- shared defaults for polling, jitter, batch size, and stack size

Generated API and Worker tiers own lifecycle wiring:

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

The baseline generated design is one low-resource background worker per GC-enabled
entity. Each worker:

- sleeps on a stop-aware timed wait
- polls no faster than one tenth of that entity's GC expiration duration
- applies a minimum polling floor
- adds randomized jitter to reduce synchronized work across replicas
- scans a bounded number of eligible rows through an indexed backend lookup
- revalidates terminal state and expiration inside the transaction that finalizes or
  removes the entity

Default runtime values:

| Setting | Default |
|---|---:|
| Minimum poll interval | 60 seconds |
| Jitter | 0-25% of poll interval |
| Batch size | 100 records |
| Java/Rust thread stack size | 256 KiB |

Go uses goroutines, whose stacks start small and grow. C++ keeps the stack-size setting
for configuration parity, but portable `std::thread` does not expose stack sizing; a
platform-specific thread builder can apply it later.

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

Lease coordination is intentionally not part of the baseline. It can be added later if
large deployments show measurable GC contention.

Entity GC must not depend on leases, queues, workflows, feature flags, logs, or metrics
for correctness. It is limited to the generic OCC backend and transaction primitives.
That constraint prevents circular runtime dependencies, such as GC requiring leases
while lease records themselves require GC.

## Artifact Model

The shared artifact model reserves common-tier generated files for all languages:

| Language | Descriptor artifact | Repository contract | Worker artifact |
|---|---|---|---|
| C++ | `common/runtime/entity_gc_descriptors.hpp` | `common/runtime/entity_gc_repository.hpp` | `common/runtime/entity_gc_workers.hpp` |
| Go | `common/backend/runtime/entity_gc_descriptors.go` | `common/backend/runtime/entity_gc_repository.go` | `common/backend/runtime/entity_gc_workers.go` |
| Java | `common/com/statespec/backend/runtime/EntityGcDescriptors.java` | `common/com/statespec/backend/runtime/EntityGcRepository.java` | `common/com/statespec/backend/runtime/EntityGcWorkers.java` |
| Rust | `common/runtime/entity_gc_descriptors.rs` | `common/runtime/entity_gc_repository.rs` | `common/runtime/entity_gc_workers.rs` |

These artifacts are shared by generated API and Worker apps. The common worker exposes
a small `runOnce`/`run_once` surface for one entity descriptor and leaves lifecycle
thread ownership to the generated API or Worker app composition layer.

Generated API processes expose an entity GC worker registration hook and own the
background lifecycle after `start()` is called. Registered GC workers run on generated
background threads or goroutines, receive stop requests through the same process shutdown
path as the API transport, and are joined before `join()` completes. The concrete
repository implementation that supplies eligible rows remains backend/runtime-owned.

The generated architecture diagrams show this composition:

- [Generated API app architecture](../diagrams/generated-api-app-architecture.puml)
- [Generated Worker app architecture](../diagrams/generated-worker-app-architecture.puml)
- [Generated system block diagram](../diagrams/generated-system-block-diagram.puml)
