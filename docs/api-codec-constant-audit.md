# API Codec Constant Audit

This document records the current generated API entity codec field-name literals before
the codec generators are changed to use generated entity constants.

Scope:

- C++: `api/entities/<entity>/codecs.hpp`
- Go: `api/backend/entities/<entity>/codecs.go`
- Java: `api/com/statespec/generated/entities/<entity>/Codecs.java`
- Rust: `api/entities/<entity>/codecs.rs`

The audit uses `examples/api-entities-only.sspec` because it exercises canonical
entity-owned CRUD for `Account`, `Project`, and `Task`.

## Current Finding

Generated per-entity API codec files still use raw string literals for JSON object keys
and decoder diagnostic field names. Many of those literals correspond to durable entity
fields and should instead come from `common/entities/<entity>/constants.*`.

The next implementation pass should replace only entity-backed field names. API-local
contract names remain local to the API layer.

## Durable Entity Field Literals

These strings represent persisted entity fields and should be replaced by generated
common entity constants when they appear in entity-owned API codecs:

| Entity | Durable field literals observed in codecs | Constants source |
|---|---|---|
| `Account` | `tenant_id`, `account_id`, `display_name`, `status`, `created_at`, `updated_at` | `common/entities/account/constants.*` |
| `Project` | `tenant_id`, `project_id`, `account_id`, `name`, `status`, `created_at`, `updated_at` | `common/entities/project/constants.*` |
| `Task` | `tenant_id`, `task_id`, `project_id`, `account_id`, `title`, `priority`, `status`, `created_at`, `updated_at` | `common/entities/task/constants.*` |

The same rule applies to any future entity field used by generated CRUD request,
response, list, status-update, or delete codecs.

## API-Local Literals

These strings are API contract facts, not durable entity schema facts, and should stay
API-local unless a separate API constants artifact is introduced:

| Literal kind | Examples | Reason |
|---|---|---|
| Shape names | `CreateAccountRequest`, `AccountResponse`, `ListAccountsResponse` | API contract names, not persisted entity fields |
| List collection response keys | `accounts`, `projects`, `tasks` | Response envelope field names derived from API shape fields |
| Operation names and function names | `DecodeCreateAccountRequest`, `encode_create_account_response` | Generated API operation surface |

List selector fields such as `tenant_id` and `status` are entity fields and should use
entity constants; list result array keys such as `accounts` are API envelope fields and
may remain API-local.

## Language-Specific Current Patterns

C++ currently emits raw field names in member reads and response object writes:

```cpp
decode_string(require_member(context.body, "display_name"), "display_name")
body.emplace("tenant_id", statespec::backend::Json{response.tenant_id})
```

Target direction:

```cpp
decode_string(require_member(context.body, entities::account::constants::kAccountFieldDisplayName), entities::account::constants::kAccountFieldDisplayName)
body.emplace(entities::account::constants::kAccountFieldTenantId, statespec::backend::Json{response.tenant_id})
```

Go currently emits raw field names in `RequireMember`, `DecodeString`, and response map
writes:

```go
codecsupport.RequireMember(request.Body, "display_name")
codecsupport.DecodeString(displayNameJSON, "display_name")
body["tenant_id"] = common.JSONString(response.TenantId)
```

Target direction:

```go
codecsupport.RequireMember(request.Body, entityconstants.AccountFieldDisplayName)
codecsupport.DecodeString(displayNameJSON, entityconstants.AccountFieldDisplayName)
body[entityconstants.AccountFieldTenantId] = common.JSONString(response.TenantId)
```

Java currently emits raw field names in `requireMember`, `decodeString`, and response
map writes:

```java
decodeString(requireMember(request.body(), "display_name"), "display_name")
body.put("tenant_id", Json.string(response.tenant_id()))
```

Target direction:

```java
decodeString(requireMember(request.body(), Constants.ACCOUNT_FIELD_DISPLAY_NAME), Constants.ACCOUNT_FIELD_DISPLAY_NAME)
body.put(Constants.ACCOUNT_FIELD_TENANT_ID, Json.string(response.tenant_id()))
```

Rust currently emits raw field names in `require_member`, `decode_string`, and response
map writes:

```rust
decode_string(require_member(&request.body, "display_name")?, "display_name")?
body.insert("tenant_id".to_string(), Json::String(response.tenant_id.clone()));
```

Target direction:

```rust
decode_string(require_member(&request.body, entity_constants::ACCOUNT_FIELD_DISPLAY_NAME)?, entity_constants::ACCOUNT_FIELD_DISPLAY_NAME)?
body.insert(entity_constants::ACCOUNT_FIELD_TENANT_ID.to_string(), Json::String(response.tenant_id.clone()));
```

## Regression Expectations For The Implementation PRs

The implementation PRs should add or update generated-app assertions to verify that:

- per-entity API codec files import/include the exact matching common entity constants
  file;
- entity-backed request and response fields use generated constants;
- raw durable field literals are absent from codec logic for `Account`, `Project`, and
  `Task`;
- API-local shape names and list envelope keys are not forced into common entity
  constants;
- per-entity codec files do not import `model.*` or common descriptor facades.

