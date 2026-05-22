#pragma once

#include <string_view>

namespace statespec
{

inline constexpr std::string_view DefaultTenantIdFieldName = "tenant_id";
inline constexpr std::string_view DefaultSystemTenantIdConfigKey = "STATESPEC_SYSTEM_TENANT_ID";

inline constexpr std::string_view EntityCreatedAtFieldName = "created_at";
inline constexpr std::string_view EntityUpdatedAtFieldName = "updated_at";
inline constexpr std::string_view EntityStatusFieldName = "status";

inline constexpr std::string_view EntityCreatedAtFieldType = "timestamp";
inline constexpr std::string_view EntityUpdatedAtFieldType = "timestamp";
inline constexpr std::string_view EntityStatusFieldType = "string";

inline constexpr std::string_view GarbageCollectionModeDelete = "delete";
inline constexpr std::string_view GarbageCollectionModeTombstone = "tombstone";
inline constexpr std::string_view GarbageCollectionModeArchive = "archive";

inline constexpr std::string_view OwnershipAuthoritySystem = "system";
inline constexpr std::string_view OwnershipAuthorityExternal = "external";
inline constexpr std::string_view OwnershipSystemOfRecordSelf = "self";
inline constexpr std::string_view OwnershipLifecycleAuthoritative = "authoritative";
inline constexpr std::string_view OwnershipLifecycleManaged = "managed";
inline constexpr std::string_view OwnershipLifecycleObserved = "observed";
inline constexpr std::string_view OwnershipLifecycleProjected = "projected";

// This is a generator convention for default soft-delete handlers, not a global semantic rule.
// Entity status/state values remain entity-owned and should be referenced through entity-specific
// constants in generated code.
inline constexpr std::string_view ConventionalSoftDeleteTerminalStateName = "Deleted";

} // namespace statespec
