# Collection Schema Upgrades

StateSpec collection descriptors are durable backend contracts. Generated runtimes call
`ensure_collection` or `ensure_collections` during startup to register the collections
required by entities, feature flags, queues, leases, workflows, logs, metrics, and other
runtime components.

Backend implementations must allow only backwards-compatible updates to an already
registered collection descriptor. Incompatible updates must fail with a schema conflict
instead of silently replacing the stored descriptor.

## Enforcement Point

Schema compatibility is enforced by the generic backend collection registration API:

```text
ensure_collection(descriptor)
ensure_collections(descriptors)
```

Typed runtime stores define the descriptors for their own collections, but they do not
own compatibility enforcement. The backend owns descriptor registration because
collection descriptors are generic OCC storage contracts, not queue, lease, workflow,
feature flag, log, or metric semantics.

Backend adapters should use the shared schema compatibility helpers instead of
hand-rolling descriptor comparison:

| Language | Helper file | Compare API | Validate API |
|---|---|---|---|
| C++ | `common/schema_compatibility.hpp` | `compare_collection_descriptors` | `validate_collection_descriptor_upgrade` |
| Go | `common/backend/schema_compatibility.go` | `CompareCollectionDescriptors` | `ValidateCollectionDescriptorUpgrade` |
| Java | `common/com/statespec/backend/SchemaCompatibility.java` | `compareCollectionDescriptors` | `validateCollectionDescriptorUpgrade` |
| Rust | `common/schema_compatibility.rs` | `compare_collection_descriptors` | `validate_collection_descriptor_upgrade` |

The expected backend behavior is:

```text
if collection is not registered:
  store descriptor
else if descriptor is identical:
  no-op
else if requested descriptor is backwards compatible with existing descriptor:
  store requested descriptor
else:
  fail with SchemaConflict
```

Production backends should persist registered descriptors in durable backend-owned
metadata. In-memory backends may keep descriptors in memory, but must apply the same
compatibility rules so tests and local generated applications catch unsafe upgrades.

## Backwards Compatible

A collection descriptor update is backwards compatible only when all existing records
remain valid, all existing keys remain addressable, existing queries keep the same
meaning, and old and new application versions can safely overlap during a rolling
upgrade.

Allowed changes:

- Re-registering an identical descriptor.
- Increasing `schema_version` with only compatible changes.
- Adding an optional field.
- Relaxing a field from required to optional.
- Adding a non-unique secondary index.
- Adding fields that are valid when absent from existing JSON records.

Disallowed changes:

- Changing the collection name.
- Changing `key_fields`, including field order.
- Removing a key field.
- Renaming a field.
- Removing a field from the descriptor.
- Changing a field type.
- Changing a field's original StateSpec `type_name`.
- Tightening a field from optional to required.
- Adding a required field without an explicit migration and backfill mechanism.
- Removing an index.
- Renaming an index.
- Changing index fields or index field order.
- Changing an index from non-unique to unique.
- Changing an index from unique to non-unique.
- Decreasing `schema_version`.
- Changing descriptor shape without increasing `schema_version`, except exact
  idempotent re-registration.

Adding a unique index is intentionally disallowed as a normal compatible upgrade. It can
fail against existing data and changes write-conflict behavior. If StateSpec later adds
explicit migration support, unique index creation should be modeled as a migration step
that validates existing records before the descriptor is accepted.

## Rolling Upgrade Doctrine

Backwards compatibility is defined for rolling service upgrades. A new generated
application version may start while old application instances are still running against
the same backend.

This means a compatible descriptor update must not require old instances to understand a
new required field, new key shape, renamed field, changed index meaning, or changed field
type. New code may write additional optional fields, but old code must be able to read or
ignore those records according to the existing JSON document contract.

## Schema Version Rule

`schema_version` is a monotonic collection descriptor version.

- Identical descriptor re-registration may use the same `schema_version`.
- Any non-identical compatible descriptor update must increase `schema_version`.
- A lower `schema_version` is invalid.
- A changed descriptor with the same `schema_version` is invalid unless the descriptor is
  exactly identical.

`schema_version` is not a migration engine. It records descriptor evolution and gives
backends a deterministic compatibility boundary.

## Relationship To Runtime Stores

Runtime stores are typed clients of generic backend collections. They may register
collections for their own records, such as queue messages, lease records, workflow
executions, feature flag overrides, log events, or metric samples.

The compatibility rule applies to all of those collections equally. Runtime stores may
choose collection names, keys, indexes, and JSON record shapes, but backend registration
must reject incompatible descriptor changes regardless of which runtime store requested
the registration.

## Test Expectations

Backend conformance tests should cover:

- identical descriptor registration is idempotent
- adding an optional field is accepted with an increased `schema_version`
- adding a non-unique index is accepted with an increased `schema_version`
- changing key fields fails with `SchemaConflict`
- changing field type or `type_name` fails with `SchemaConflict`
- adding a required field fails without an explicit migration mechanism
- removing or changing an index fails with `SchemaConflict`
- decreasing `schema_version` fails with `SchemaConflict`
- changing descriptor shape without increasing `schema_version` fails with
  `SchemaConflict`
