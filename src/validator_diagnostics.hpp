#pragma once

namespace statespec::validator_detail::diagnostic_codes
{

inline constexpr const char* MissingSystem = "SSPEC1001";

inline constexpr const char* DuplicateDeclaration = "SSPEC3001";
inline constexpr const char* UnknownReference = "SSPEC3002";

inline constexpr const char* EntityMissingKey = "SSPEC3101";
inline constexpr const char* EntityInvalidStateType = "SSPEC3102";
inline constexpr const char* EntityTerminalGcRequiresRetention = "SSPEC3103";
inline constexpr const char* EntityGcRequiresTerminalState = "SSPEC3104";
inline constexpr const char* EntityUnknownTransitionState = "SSPEC3105";
inline constexpr const char* EntityDuplicateFieldName = "SSPEC3106";
inline constexpr const char* EntityDuplicateState = "SSPEC3107";
inline constexpr const char* EntityMissingInitialState = "SSPEC3108";
inline constexpr const char* EntityStateTransitionGcConflict = "SSPEC3109";

inline constexpr const char* ShapeInvalidName = "SSPEC3201";

inline constexpr const char* OwnershipInvalidAuthority = "SSPEC3301";
inline constexpr const char* OwnershipInvalidLifecycle = "SSPEC3302";
inline constexpr const char* RelationInvalidParentDelete = "SSPEC3303";
inline constexpr const char* RelationDetachRequiresOptionalParent = "SSPEC3304";
inline constexpr const char* RelationUniqueFieldMissing = "SSPEC3305";
inline constexpr const char* ChildUnknownRelation = "SSPEC3306";

inline constexpr const char* TenantEntityMissingTenantField = "SSPEC3401";
inline constexpr const char* TenantQueueMessageMissingTenantField = "SSPEC3402";
inline constexpr const char* TenantApiInputMissingTenantField = "SSPEC3403";
inline constexpr const char* TenantEntityKeyMissingTenantField = "SSPEC3404";
inline constexpr const char* TenantPolicyScopeMismatch = "SSPEC3405";
inline constexpr const char* TenantApiPathMissingTenantIdentity = "SSPEC3406";
inline constexpr const char* TenantLogMissingTenantField = "SSPEC3407";
inline constexpr const char* TenantMetricMissingTenantLabel = "SSPEC3408";

inline constexpr const char* LeaseRenewEveryTooLong = "SSPEC3501";
inline constexpr const char* LeaseTtlTooLong = "SSPEC3502";

inline constexpr const char* RequiredDeclaration = "SSPEC4001";
inline constexpr const char* PositiveIntegerRequired = "SSPEC4002";
inline constexpr const char* NonNegativeIntegerRequired = "SSPEC4003";
inline constexpr const char* DurationRequired = "SSPEC4004";

inline constexpr const char* ApiMultiplePrimaryActions = "SSPEC4101";

inline constexpr const char* FeatureFlagInvalidName = "SSPEC4201";
inline constexpr const char* FeatureFlagInvalidType = "SSPEC4202";
inline constexpr const char* FeatureFlagInvalidDefault = "SSPEC4203";
inline constexpr const char* FeatureEnabledRequiresBoolFlag = "SSPEC4204";

inline constexpr const char* LogInvalidName = "SSPEC4301";
inline constexpr const char* LogInvalidLevel = "SSPEC4302";
inline constexpr const char* LogDuplicateEventName = "SSPEC4303";

inline constexpr const char* MetricInvalidName = "SSPEC4401";
inline constexpr const char* MetricInvalidKind = "SSPEC4402";
inline constexpr const char* MetricDuplicateBackendName = "SSPEC4403";
inline constexpr const char* MetricInvalidLabelType = "SSPEC4404";
inline constexpr const char* MetricHighCardinalityLabel = "SSPEC4405";

inline constexpr const char* ValueInvalidName = "SSPEC4501";

inline constexpr const char* EnumInvalidName = "SSPEC4601";
inline constexpr const char* EnumInvalidMemberName = "SSPEC4602";

inline constexpr const char* EventInvalidName = "SSPEC4701";

inline constexpr const char* ExternalSystemInvalidName = "SSPEC4901";
inline constexpr const char* ExternalSystemMetadataProfileFieldMissing = "SSPEC4902";
inline constexpr const char* ExternalSystemMetadataRequiredFieldMissing = "SSPEC4903";
inline constexpr const char* ExternalSystemMetadataTenantFieldMissing = "SSPEC4904";
inline constexpr const char* ExternalSystemMetadataTenantKeyMissing = "SSPEC4905";
inline constexpr const char* ExternalSystemMappingInvalidPath = "SSPEC4906";
inline constexpr const char* ExternalSystemMappingUnsupportedRoot = "SSPEC4907";
inline constexpr const char* ExternalSystemMappingDuplicateTarget = "SSPEC4908";
inline constexpr const char* ExternalSystemMappingFieldMissing = "SSPEC4909";
inline constexpr const char* ExternalSystemMappingRequiredFieldMissing = "SSPEC4910";
inline constexpr const char* ExternalSystemMetadataRequiresSystemOwnership = "SSPEC4911";
inline constexpr const char* ExternalSystemMetadataRequiresFoundationalFields = "SSPEC4912";
inline constexpr const char* ExternalSystemMetadataProfileFieldKeyRequired = "SSPEC4913";
inline constexpr const char* ExternalSystemMetadataUniqueKeyIndexRequired = "SSPEC4914";
inline constexpr const char* ExternalSystemMetadataLifecycleStatesRequired = "SSPEC4915";
inline constexpr const char* ExternalSystemMetadataDeletedGcRequired = "SSPEC4916";
inline constexpr const char* ExternalSystemMetadataLifecycleTransitionRequired = "SSPEC4917";

inline constexpr const char* InvalidExpression = "SSPEC5201";
inline constexpr const char* ExpressionFunctionNotAllowed = "SSPEC5202";

inline constexpr const char* NoncanonicalEntityOrder = "SSPEC6101";
inline constexpr const char* NoncanonicalWorkflowOrder = "SSPEC6102";
inline constexpr const char* NoncanonicalPolicyOrder = "SSPEC6103";
inline constexpr const char* NoncanonicalLogOrder = "SSPEC6104";
inline constexpr const char* NoncanonicalMetricOrder = "SSPEC6105";
inline constexpr const char* NoncanonicalSystemOrder = "SSPEC6106";

} // namespace statespec::validator_detail::diagnostic_codes

namespace statespec::validator_detail::diagnostic_fragments
{

inline constexpr const char* MustUsePascalCase = " must use PascalCase";
inline constexpr const char* DuplicateDeclarationPrefix = "duplicate declaration '";
inline constexpr const char* UnknownReferencePrefix = "unknown ";
inline constexpr const char* ReferenceSuffix = " reference '";
inline constexpr const char* MustDeclare = " must declare ";
inline constexpr const char* MustBePositiveInteger = " must be a positive integer";
inline constexpr const char* MustBeNonNegative = " must be non-negative";
inline constexpr const char* MustBeIso8601Duration = " must be an ISO-8601 duration";
inline constexpr const char* InvalidExpressionPrefix = "invalid expression: ";

} // namespace statespec::validator_detail::diagnostic_fragments
