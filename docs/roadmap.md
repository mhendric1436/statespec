# Roadmap

This roadmap tracks implementation state for the current compiler and generator pass.
The compiler owns syntax, validation, semantic lowering, IR, descriptors, and generated
contracts. Runtime adapters own concrete storage, transport, authentication, and remote
execution.

## Current Pass: External-System Metadata

The external-system metadata pass is implemented through the compiler and generated
contract layers.

Complete:

- Parse `external_system metadata` declarations, including `entity`, `profile_field`,
  `required_fields`, and `mappings`.
- Validate metadata entity references, tenant/key/profile fields, required metadata
  fields, unique key indexes, authoritative ownership, and the canonical
  `Active`/`Disabled`/`Deleted` lifecycle.
- Validate mapping roots, mapping path shape, unique targets, and required
  `metadata.*` sources.
- Lower metadata contracts, mappings, APIs, API servers, and route metadata into IR.
- Generate C++, Go, Java, and Rust descriptors for external systems, mappings, APIs,
  API servers, and route metadata.
- Generate mapping-plan helpers, mapping applicator contracts, missing-source
  diagnostics, metadata lookup helpers, and transaction-scoped resolver helpers.
- Generate OCC repository contracts and operator metadata API handler contracts for
  metadata upsert, get, disable, and delete flows.
- Generate OpenAPI for declared APIs and operator metadata API surfaces.
- Maintain an end-to-end regression fixture at
  `testdata/generators/external-system-metadata-e2e.sspec`.

Partial:

- OpenAPI schema lowering is intentionally conservative. Primitive and shape references
  are emitted, but richer `value`, `enum`, nested collection, and error-model semantics
  still need expansion.
- Formatter support is token-preserving. Canonical-order warnings exist, but the
  formatter does not yet own full AST-based declaration reordering.
- Policies are emitted as descriptors, but enforcement hooks are not generated.

Runtime-owned by design:

- Metadata persistence implementations behind generated repository contracts.
- OCC transaction lifecycle, retry policy, and commit handling.
- HTTP framework route registration, request decoding, response encoding, and
  middleware integration.
- Authentication and authorization enforcement for operator APIs.
- Secret lookup for credential references.
- Concrete external-system clients, wire serialization, timeout behavior, retry logic,
  circuit breakers, and service mesh integration.
- Worker loops, event transport, and remote-call execution.

## Next Priorities

1. Expand OpenAPI output for `value`, `enum`, nested collection, and error schemas.
2. Add generated policy enforcement hooks once expression type checking is stronger.
3. Add optional HTTP adapter generation after route, auth, and error contracts stabilize.
4. Add backend conformance tests for metadata repository implementations.
5. Continue moving formatter behavior from token-preserving output toward AST-owned
   canonical declaration ordering.
