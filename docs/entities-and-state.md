# Entities And State Machines

Entities are durable domain objects. They define identity, data fields, lifecycle state,
relationships, invariants, and indexes.

## Entity Skeleton

```statespec
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
  }

  state_machine {
    state Creating
    state Active
    state Deleted {
      terminal: true
      garbage_collection {
        after: P30D
        mode: tombstone
      }
    }

    initial Creating
    terminal [Deleted]

    Creating -> Active
    Active -> Deleted
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

Tenant-scoped systems must include the scoped tenant field in every entity key. For the
canonical `tenant scoped_by tenant_id` declaration, durable tenant-owned entities use a
composite key such as `key tenant_id, order_id`.

## Ownership

Every entity must declare explicit ownership. The ownership block states whether this
system or an external system is authoritative, identifies the system of record, and
declares the lifecycle mode.

```statespec
ownership {
  authority: system
  system_of_record: self
  lifecycle: authoritative
}
```

Do not encode ownership only in prose, annotations, or workflow names.

## Fields

Fields use the form:

```statespec
field_name type
```

The first fields in every entity must be the canonical management fields, in this
exact order:

```statespec
created_at timestamp
updated_at timestamp
status string
```

`status` is the durable lifecycle field associated with the entity `state_machine`.
Domain fields, key fields, relation fields, and optional timestamps follow these
management fields.

Examples:

```statespec
fields {
  created_at timestamp
  updated_at timestamp
  status string
  id uuid
  display_name string
  active bool
  revision int64
  metadata json
  deleted_at timestamp?
}
```

Use optional fields when the value may be absent:

```statespec
failure_reason string?
```

## State Machines

State machines define lifecycle states and legal transitions. State transitions are
authored only inside the `state_machine` block.

```statespec
state_machine {
  state Creating
  state Active
  state Updating
  state Deleting
  state Deleted {
    terminal: true
    garbage_collection {
      after: P30D
      mode: tombstone
    }
  }
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

Every entity must declare a `state_machine`. Entity state is durable lifecycle metadata,
not a transient implementation detail.

Terminal states must be declared in both places: the state itself uses `terminal: true`,
and the state machine lists the same state in `terminal [...]`. Each terminal state must
also declare `garbage_collection`, which makes retention explicit.

## Terminal Garbage Collection

Terminal state garbage collection is modeled on the terminal state declaration. The
canonical syntax shape is:

```statespec
state_machine {
  state Creating
  state Active
  state Deleted {
    terminal: true
    garbage_collection {
      after: P30D
      mode: tombstone
    }
  }

  initial Creating
  terminal [Deleted]

  Creating -> Active
  Active -> Deleted
}
```

Garbage collection policy belongs to the terminal state because cleanup eligibility is
part of the entity lifecycle contract. It is not an annotation, workflow step, or
backend-specific generator option.

The policy declares when an entity becomes eligible for cleanup after entering the
terminal state. It does not mean the entity is deleted immediately on transition.

Initial policy fields:

| Field | Meaning |
|---|---|
| `after` | Duration after entering the terminal state before the entity is eligible for collection. |
| `mode` | Collection behavior. Initial modes are `delete`, `tombstone`, and `archive`. |

For v0.1, every terminal state declares a cleanup policy. Advanced GC behavior such as
cascade cleanup, archive targets, retained indexes, and backend-specific retention tuning
is intentionally deferred.

## Relationships

Relationships are supported as entity metadata and flow through parsing, validation,
semantic resolution, IR lowering, and binding descriptors.

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
The validator checks that the referenced child entity exists and owns the named parent
relation.

## Invariants

Invariants name boolean expressions that must hold for entity state.

```statespec
invariants {
  valid_amount: amount >= 0
  has_status: status != ""
}
```

Keep invariants deterministic and side-effect free.
Current compiler support preserves invariant names and raw expressions for descriptors.
Expression syntax and allowed built-ins are validated. Type checking and generated
runtime enforcement are future work.

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

## Entity-Owned API Intent

Entities may declare canonical CRUD API intent inside an `api` block. This makes the
entity the source of truth for routine create, get, list, status update, and delete
contracts instead of requiring authors to hand-write repetitive API declarations.

```statespec
api {
  resource "/v1/tenants/{tenant_id}/accounts/{account_id}"

  create CreateAccount {
    fields [display_name]
  }

  get GetAccount

  list {
    path "/v1/tenants/{tenant_id}/accounts"
    by tenant_id
  }

  list AccountProjects {
    path "/v1/tenants/{tenant_id}/accounts/{account_id}/projects"
    by by_account_status
  }

  update_status UpdateAccountStatus
  delete DeleteAccount
}
```

Operation names are optional overrides. If omitted, generators can derive canonical
operation names from the entity name and operation kind.

During IR lowering, entity-owned API intent becomes ordinary canonical API IR. The
lowering synthesizes request/response shapes, API declarations, route metadata,
repository operation linkage, and API server membership. Default names are
deterministic: `CreateAccount`, `GetAccount`, `ListAccounts`,
`UpdateAccountStatus`, `DeleteAccount`, `CreateAccountRequest`, and
`AccountResponse`.

Derived shapes are entity-owned and route-aware. `Create<Entity>Request` contains
only non-path key fields plus the explicit `create fields` allowlist; key fields
already present in the route are not repeated in the request body.
`Update<Entity>StatusRequest` follows the same rule and includes only `status`
when all key fields are route parameters. `<Entity>Response` includes key fields,
all non-foundational entity fields, `status`, and, by default, the foundational
`created_at` and `updated_at` timestamps. `<Entity>ListResponse` includes the list
selector context fields plus an array of `<Entity>Response`.

`list by` is deliberately constrained. It may reference only a valid entity key
prefix, a declared index name, or a declared index field prefix. It may not reference
arbitrary fields. List APIs therefore stay aligned with durable access paths that the
backend can index and enforce.

The validator also checks that entity API paths are backed by entity fields. Resource
paths for key-addressed operations must expose all key fields. `create` field allowlists
must reference entity fields and may not include `created_at`, `updated_at`, or `status`.
`update_status` requires the canonical `status` field and a state machine. `delete`
requires a conventional terminal `Deleted` state with garbage collection metadata.

## Authoring Checklist

Before committing an entity:

- The entity has a stable key.
- All key fields are declared in `fields`.
- `created_at`, `updated_at`, and `status` are declared.
- Lifecycle states and transitions are explicit.
- Terminal cleanup policy is explicit for every terminal state.
- Parent-child relationships are durable, not implied.
- Invariants are named and deterministic.
- Index declarations match expected query patterns.
- Entity-owned list APIs use only key or index-backed selectors.
