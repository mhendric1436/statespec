#pragma once

#include <string_view>

namespace statespec::test
{

inline constexpr std::string_view ArtifactManifestTestRoot =
    "/tmp/statespec-artifact-manifest-test";
inline constexpr std::string_view ArtifactTierTestRoot = "/tmp/statespec-artifact-tier-test";
inline constexpr std::string_view TierSelectionTestRoot = "/tmp/statespec-tier-selection-test";

namespace diagnostic_codes
{

inline constexpr std::string_view DuplicateDeclaration = "SSPEC3001";
inline constexpr std::string_view UnknownReference = "SSPEC3002";

inline constexpr std::string_view EntityMissingKey = "SSPEC3101";
inline constexpr std::string_view EntityInvalidStateType = "SSPEC3102";
inline constexpr std::string_view EntityTerminalGcRequiresRetention = "SSPEC3103";
inline constexpr std::string_view EntityGcRequiresTerminalState = "SSPEC3104";
inline constexpr std::string_view EntityUnknownTransitionState = "SSPEC3105";
inline constexpr std::string_view EntityDuplicateFieldName = "SSPEC3106";
inline constexpr std::string_view EntityDuplicateState = "SSPEC3107";
inline constexpr std::string_view EntityMissingInitialState = "SSPEC3108";
inline constexpr std::string_view EntityStateTransitionGcConflict = "SSPEC3109";

inline constexpr std::string_view ShapeInvalidName = "SSPEC3201";

inline constexpr std::string_view TenantEntityMissingTenantField = "SSPEC3401";
inline constexpr std::string_view TenantQueueMessageMissingTenantField = "SSPEC3402";
inline constexpr std::string_view TenantApiInputMissingTenantField = "SSPEC3403";
inline constexpr std::string_view TenantEntityKeyMissingTenantField = "SSPEC3404";
inline constexpr std::string_view TenantPolicyScopeMismatch = "SSPEC3405";
inline constexpr std::string_view TenantApiPathMissingTenantIdentity = "SSPEC3406";
inline constexpr std::string_view TenantLogMissingTenantField = "SSPEC3407";
inline constexpr std::string_view TenantMetricMissingTenantLabel = "SSPEC3408";

inline constexpr std::string_view LeaseRenewEveryTooLong = "SSPEC3501";
inline constexpr std::string_view LeaseTtlTooLong = "SSPEC3502";

inline constexpr std::string_view RequiredDeclaration = "SSPEC4001";
inline constexpr std::string_view PositiveIntegerRequired = "SSPEC4002";
inline constexpr std::string_view NonNegativeIntegerRequired = "SSPEC4003";
inline constexpr std::string_view DurationRequired = "SSPEC4004";

inline constexpr std::string_view ApiMultiplePrimaryActions = "SSPEC4101";

inline constexpr std::string_view FeatureFlagInvalidName = "SSPEC4201";
inline constexpr std::string_view FeatureFlagInvalidType = "SSPEC4202";
inline constexpr std::string_view FeatureFlagInvalidDefault = "SSPEC4203";
inline constexpr std::string_view FeatureEnabledRequiresBoolFlag = "SSPEC4204";

inline constexpr std::string_view LogInvalidName = "SSPEC4301";
inline constexpr std::string_view LogInvalidLevel = "SSPEC4302";
inline constexpr std::string_view LogDuplicateEventName = "SSPEC4303";

inline constexpr std::string_view MetricInvalidName = "SSPEC4401";
inline constexpr std::string_view MetricInvalidKind = "SSPEC4402";
inline constexpr std::string_view MetricDuplicateBackendName = "SSPEC4403";
inline constexpr std::string_view MetricInvalidLabelType = "SSPEC4404";
inline constexpr std::string_view MetricHighCardinalityLabel = "SSPEC4405";

inline constexpr std::string_view ValueInvalidName = "SSPEC4501";
inline constexpr std::string_view EnumInvalidName = "SSPEC4601";
inline constexpr std::string_view EnumInvalidMemberName = "SSPEC4602";
inline constexpr std::string_view EventInvalidName = "SSPEC4701";

inline constexpr std::string_view ExternalSystemInvalidName = "SSPEC4901";
inline constexpr std::string_view ExternalSystemMetadataProfileFieldMissing = "SSPEC4902";
inline constexpr std::string_view ExternalSystemMetadataRequiredFieldMissing = "SSPEC4903";
inline constexpr std::string_view ExternalSystemMetadataTenantFieldMissing = "SSPEC4904";
inline constexpr std::string_view ExternalSystemMetadataTenantKeyMissing = "SSPEC4905";
inline constexpr std::string_view ExternalSystemMappingInvalidPath = "SSPEC4906";
inline constexpr std::string_view ExternalSystemMappingUnsupportedRoot = "SSPEC4907";
inline constexpr std::string_view ExternalSystemMappingDuplicateTarget = "SSPEC4908";
inline constexpr std::string_view ExternalSystemMappingFieldMissing = "SSPEC4909";
inline constexpr std::string_view ExternalSystemMappingRequiredFieldMissing = "SSPEC4910";
inline constexpr std::string_view ExternalSystemMetadataRequiresSystemOwnership = "SSPEC4911";
inline constexpr std::string_view ExternalSystemMetadataRequiresFoundationalFields = "SSPEC4912";
inline constexpr std::string_view ExternalSystemMetadataProfileFieldKeyRequired = "SSPEC4913";
inline constexpr std::string_view ExternalSystemMetadataUniqueKeyIndexRequired = "SSPEC4914";
inline constexpr std::string_view ExternalSystemMetadataLifecycleStatesRequired = "SSPEC4915";
inline constexpr std::string_view ExternalSystemMetadataDeletedGcRequired = "SSPEC4916";
inline constexpr std::string_view ExternalSystemMetadataLifecycleTransitionRequired = "SSPEC4917";

inline constexpr std::string_view InvalidExpression = "SSPEC5201";
inline constexpr std::string_view ExpressionFunctionNotAllowed = "SSPEC5202";

inline constexpr std::string_view NoncanonicalEntityOrder = "SSPEC6101";
inline constexpr std::string_view NoncanonicalWorkflowOrder = "SSPEC6102";
inline constexpr std::string_view NoncanonicalPolicyOrder = "SSPEC6103";
inline constexpr std::string_view NoncanonicalLogOrder = "SSPEC6104";
inline constexpr std::string_view NoncanonicalMetricOrder = "SSPEC6105";
inline constexpr std::string_view NoncanonicalSystemOrder = "SSPEC6106";

} // namespace diagnostic_codes

} // namespace statespec::test
