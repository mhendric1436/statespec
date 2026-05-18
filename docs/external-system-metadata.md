# External System Metadata

External systems name integration boundaries in the canonical model. Runtime metadata for
executing remote API calls should not be exposed wholesale through user-facing APIs.
StateSpec should keep user APIs focused on business intent while service operators manage
endpoint, routing, credential, timeout, retry, and circuit-breaker metadata through
tenant-scoped operator state.

## Runtime Planes

Use three separate planes:

| Plane | Responsibility |
|---|---|
| User API plane | Accepts caller intent, domain identifiers, and request payload fields. |
| Operator API plane | Manages remote execution metadata as durable tenant-scoped state. |
| Execution plane | Resolves operator metadata and user/API/workflow fields into remote client calls. |

User APIs should not require callers to know base URLs, auth references, service mesh
routes, retry policies, timeout policies, mTLS identities, or circuit-breaker settings.
Those fields are operational metadata. They are often unavailable to product users and may
be secret-adjacent.

## Tenant Scope

Operator metadata is service data and must align with the system tenant model.

Rules:

- Operator metadata entities must include the system tenant field in `key`.
- Operator metadata entities must include the system tenant field in `fields`.
- Operator APIs must include tenant identity in the path.
- Operator API input and output shapes must include the tenant field.
- Remote-call metadata reads must be scoped by the current service tenant.
- Field mappings must not join user input from one tenant with metadata from another
  tenant.

For a system using `tenant scoped_by tenant_id`, an operator-managed metadata entity
should follow the same foundational entity shape as other durable entities:

```statespec
entity ExternalSystemEndpoint {
  key tenant_id, external_system_id, profile

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
    external_system_id string
    profile string
    base_url string
    auth_ref string
    timeout_ms int
    retry_policy string
  }

  state_machine {
    state Active
    state Disabled
    state Deleted {
      terminal: true
      garbage_collection {
        after: P30D
        mode: tombstone
      }
    }

    initial Active
    terminal [Deleted]

    Active -> Disabled
    Disabled -> Active
    Active -> Deleted
    Disabled -> Deleted
  }
}
```

## Operator APIs

Operator APIs manage metadata with OCC-backed writes. They are service-operator contracts,
not product-user contracts.

```statespec
shape UpsertExternalSystemEndpointRequest {
  tenant_id string
  external_system_id string
  profile string
  base_url string
  auth_ref string
  timeout_ms int
  retry_policy string
}

shape ExternalSystemEndpointResponse {
  tenant_id string
  external_system_id string
  profile string
  base_url string
  auth_ref string
  timeout_ms int
  retry_policy string
  status string
}

shape DisableExternalSystemEndpointRequest {
  tenant_id string
  external_system_id string
  profile string
}

api UpsertExternalSystemEndpoint {
  method PUT
  path "/v1/tenants/{tenant_id}/operators/external-systems/{external_system_id}/profiles/{profile}"
  input UpsertExternalSystemEndpointRequest
  output ExternalSystemEndpointResponse
}

api GetExternalSystemEndpoint {
  method GET
  path "/v1/tenants/{tenant_id}/operators/external-systems/{external_system_id}/profiles/{profile}"
  output ExternalSystemEndpointResponse
}

api DisableExternalSystemEndpoint {
  method PATCH
  path "/v1/tenants/{tenant_id}/operators/external-systems/{external_system_id}/profiles/{profile}/disable"
  input DisableExternalSystemEndpointRequest
  output ExternalSystemEndpointResponse
}

api DeleteExternalSystemEndpoint {
  method DELETE
  path "/v1/tenants/{tenant_id}/operators/external-systems/{external_system_id}/profiles/{profile}"
  output ExternalSystemEndpointResponse
}

api_server OperatorApi {
  serves UpsertExternalSystemEndpoint
  serves GetExternalSystemEndpoint
  serves DisableExternalSystemEndpoint
  serves DeleteExternalSystemEndpoint
  concurrency 4
}
```

The request and response shapes include `tenant_id`. Policies should restrict these APIs
to operator roles or control-plane callers.

## Remote Call Mapping

Remote calls combine fields from multiple sources:

| Source | Examples |
|---|---|
| User API input | amount, currency, customer ID, requested region. |
| Durable entity or workflow state | order ID, payment ID, idempotency key. |
| Operator metadata entity | base URL, auth reference, timeout, retry policy. |

The mapping from these sources to remote API fields should be explicit in future IR or
grammar. The first implementation should prefer descriptors and generated helper APIs
until several real integrations prove the stable syntax.

Conceptual mapping:

```text
remote_call Stripe.CreatePayment
  user.amount        -> request.amount
  user.currency      -> request.currency
  order.payment_id   -> request.idempotency_key
  metadata.base_url  -> client.base_url
  metadata.auth_ref  -> client.auth_ref
  metadata.timeout_ms -> client.timeout_ms
```

## OCC Requirement

Generated runtime helpers should read operator metadata through the same backend
transaction model used for entities, queues, leases, workflows, logs, metrics, and feature
flags. A workflow or API handler should resolve metadata inside its caller-managed
transaction, then execute the remote call with the resolved configuration.

Generated binding descriptors expose the metadata entity, tenant field, metadata entity key
fields, profile field, and required execution metadata fields. Runtime adapters should use
that descriptor contract to build tenant-scoped backend lookups instead of hard-coding
metadata entity names or key shapes.

The reference bindings expose external-system metadata lookup request and resolution types,
plus resolver interfaces with transaction-aware methods. Resolvers should validate required
metadata fields from the loaded document before remote execution.

Generated descriptor files include helper functions that convert an
`external_system` metadata descriptor and runtime key values into the binding-native lookup
request. Runtime adapters should prefer those helpers over manually copying descriptor
fields into lookup requests.

Generated descriptor files also include transaction-scoped resolve helpers that call the
binding resolver's `Tx` method. These helpers intentionally keep metadata reads inside the
caller-managed OCC transaction.

The binding lookup types expose key-completeness helpers. Generated resolve helpers check
that every metadata entity key field has a provided runtime key value before calling the
resolver, preventing malformed lookups from reaching backend-specific implementations.

Metadata writes should use compare-and-swap semantics so two operators cannot silently
overwrite each other's endpoint, credential reference, or retry policy updates.

## Authoring Guidance

Keep `external_system` declarations small and stable:

```statespec
external_system Stripe {
  owner: "billing"
  kind: "payments"

  metadata {
    entity ExternalSystemEndpoint
    profile_field profile
    required_fields [base_url, auth_ref, timeout_ms, retry_policy]
  }
}
```

The `external_system` declaration identifies the integration boundary. Operator metadata
entities describe runtime configuration. User APIs expose only the fields users can
reasonably provide.

The metadata block is intentionally narrow. `entity` names the operator-managed metadata
entity, `profile_field` identifies the entity field used to select a metadata profile, and
`required_fields` lists execution metadata fields that generated descriptors and future
validators can require before making a remote call.

The validator resolves `metadata.entity` to a declared entity and checks that
`profile_field` and every `required_fields` entry exist on that entity. In tenant-scoped
systems, the metadata entity must also declare the system tenant field and include it in
the entity key, so operator configuration is isolated by the same tenant boundary as
entities, APIs, queues, logs, and metrics.

A complete example is available in
[`examples/external-system-metadata.sspec`](../examples/external-system-metadata.sspec).
