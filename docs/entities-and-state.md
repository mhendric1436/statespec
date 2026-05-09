# Entities And State Machines

Entities are durable domain objects. They define identity, data fields, lifecycle state,
relationships, invariants, and indexes.

## Entity Skeleton

```statespec
entity Order {
  key tenant_id, order_id

  fields {
    tenant_id string
    order_id string
    status string
    created_at timestamp
  }
}
```

## Keys

Every entity should have one `key` declaration.

Single-field key:

```statespec
key order_id
```

Composite key:

```statespec
key tenant_id, order_id
```

Key fields should also appear in the `fields` block unless a future runtime explicitly
generates them.

## Fields

Fields use the form:

```statespec
field_name type
```

Examples:

```statespec
fields {
  id uuid
  display_name string
  active bool
  revision int64
  metadata json
  created_at timestamp
  deleted_at timestamp?
}
```

Use optional fields when the value may be absent:

```statespec
failure_reason string?
```

## State Machines

State machines define lifecycle states and legal transitions.

```statespec
state_machine {
  state Creating
  state Active
  state Updating
  state Deleting
  state Deleted { terminal: true }
  state Failed

  initial Creating
  terminal [Deleted]

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
```

A compact transition-only style is also commonly used in examples:

```statespec
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
```

Prefer explicit `state`, `initial`, and `terminal` declarations for production specs
because they make lifecycle intent easier to review.

## Relationships

The grammar reserves relationship constructs for parent-child and reference models.

```statespec
relations {
  parent account: ref<Account> {
    kind: composition
    on_parent_delete: block
    parent_must_be_in: [Active]
  }

  ref owner: ref<User>
}
```

Use relationships to model durable ownership and identity boundaries. Avoid encoding
ownership only in prose or workflow step names.

## Children

Parent entities can declare child collections:

```statespec
children {
  orders: Order by account_id
}
```

Child collection declarations should align with the child entity's parent relation.

## Invariants

Invariants name boolean expressions that must hold for entity state.

```statespec
invariants {
  valid_amount: amount >= 0
  has_status: status != ""
}
```

Keep invariants deterministic and side-effect free.

## Indexes

Indexes declare query intent.

```statespec
indexes {
  index by_status on status
  unique by_tenant_external_id on tenant_id, external_id
}
```

Use indexes to make expected access patterns explicit. Runtime generators decide how to
lower them into backend-specific metadata.

## Authoring Checklist

Before committing an entity:

- The entity has a stable key.
- All key fields are declared in `fields`.
- Lifecycle states and transitions are explicit.
- Parent-child relationships are durable, not implied.
- Invariants are named and deterministic.
- Index declarations match expected query patterns.
