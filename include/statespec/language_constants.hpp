#pragma once

#include <string_view>

namespace statespec
{

inline constexpr std::string_view DefaultTenantIdFieldName = "tenant_id";
inline constexpr std::string_view DefaultSystemTenantIdConfigKey = "STATESPEC_SYSTEM_TENANT_ID";

inline constexpr std::string_view SyntaxKeywordStatespec = "statespec";
inline constexpr std::string_view SyntaxKeywordInclude = "include";
inline constexpr std::string_view SyntaxKeywordAs = "as";
inline constexpr std::string_view SyntaxKeywordSystem = "system";
inline constexpr std::string_view SyntaxKeywordNamespace = "namespace";
inline constexpr std::string_view SyntaxKeywordValue = "value";
inline constexpr std::string_view SyntaxKeywordEnum = "enum";
inline constexpr std::string_view SyntaxKeywordShape = "shape";
inline constexpr std::string_view SyntaxKeywordExternalSystem = "external_system";
inline constexpr std::string_view SyntaxKeywordFeatureFlag = "feature_flag";
inline constexpr std::string_view SyntaxKeywordLog = "log";
inline constexpr std::string_view SyntaxKeywordMetric = "metric";
inline constexpr std::string_view SyntaxKeywordEntity = "entity";
inline constexpr std::string_view SyntaxKeywordKey = "key";
inline constexpr std::string_view SyntaxKeywordVersion = "version";
inline constexpr std::string_view SyntaxKeywordFields = "fields";
inline constexpr std::string_view SyntaxKeywordStateMachine = "state_machine";
inline constexpr std::string_view SyntaxKeywordState = "state";
inline constexpr std::string_view SyntaxKeywordInitial = "initial";
inline constexpr std::string_view SyntaxKeywordTerminal = "terminal";
inline constexpr std::string_view SyntaxKeywordOwnership = "ownership";
inline constexpr std::string_view SyntaxKeywordAuthority = "authority";
inline constexpr std::string_view SyntaxKeywordSystemOfRecord = "system_of_record";
inline constexpr std::string_view SyntaxKeywordLifecycle = "lifecycle";
inline constexpr std::string_view SyntaxKeywordControl = "control";
inline constexpr std::string_view SyntaxKeywordRelations = "relations";
inline constexpr std::string_view SyntaxKeywordParent = "parent";
inline constexpr std::string_view SyntaxKeywordRef = "ref";
inline constexpr std::string_view SyntaxKeywordOptional = "optional";
inline constexpr std::string_view SyntaxKeywordChildren = "children";
inline constexpr std::string_view SyntaxKeywordInvariants = "invariants";
inline constexpr std::string_view SyntaxKeywordIndexes = "indexes";
inline constexpr std::string_view SyntaxKeywordAnnotations = "annotations";
inline constexpr std::string_view SyntaxKeywordEvent = "event";
inline constexpr std::string_view SyntaxKeywordQueue = "queue";
inline constexpr std::string_view SyntaxKeywordMessage = "message";
inline constexpr std::string_view SyntaxKeywordPayload = "payload";
inline constexpr std::string_view SyntaxKeywordLease = "lease";
inline constexpr std::string_view SyntaxKeywordWorker = "worker";
inline constexpr std::string_view SyntaxKeywordApiServer = "api_server";
inline constexpr std::string_view SyntaxKeywordApi = "api";
inline constexpr std::string_view SyntaxKeywordBehavior = "behavior";
inline constexpr std::string_view SyntaxKeywordWorkflow = "workflow";
inline constexpr std::string_view SyntaxKeywordStep = "step";
inline constexpr std::string_view SyntaxKeywordChildSet = "child_set";
inline constexpr std::string_view SyntaxKeywordPolicy = "policy";
inline constexpr std::string_view SyntaxKeywordOn = "on";
inline constexpr std::string_view SyntaxKeywordWhen = "when";
inline constexpr std::string_view SyntaxKeywordWhere = "where";
inline constexpr std::string_view SyntaxKeywordRequire = "require";
inline constexpr std::string_view SyntaxKeywordSet = "set";
inline constexpr std::string_view SyntaxKeywordEmit = "emit";
inline constexpr std::string_view SyntaxKeywordEmits = "emits";
inline constexpr std::string_view SyntaxKeywordEnqueue = "enqueue";
inline constexpr std::string_view SyntaxKeywordAcquire = "acquire";
inline constexpr std::string_view SyntaxKeywordRenew = "renew";
inline constexpr std::string_view SyntaxKeywordRelease = "release";
inline constexpr std::string_view SyntaxKeywordStart = "start";
inline constexpr std::string_view SyntaxKeywordLoad = "load";
inline constexpr std::string_view SyntaxKeywordAllocates = "allocates";
inline constexpr std::string_view SyntaxKeywordReturns = "returns";
inline constexpr std::string_view SyntaxKeywordAtomic = "atomic";
inline constexpr std::string_view SyntaxKeywordForEach = "for_each";
inline constexpr std::string_view SyntaxKeywordIn = "in";
inline constexpr std::string_view SyntaxKeywordCreate = "create";
inline constexpr std::string_view SyntaxKeywordChild = "child";
inline constexpr std::string_view SyntaxKeywordObserve = "observe";
inline constexpr std::string_view SyntaxKeywordMove = "move";
inline constexpr std::string_view SyntaxKeywordFrom = "from";
inline constexpr std::string_view SyntaxKeywordTo = "to";
inline constexpr std::string_view SyntaxKeywordTransitionTo = "transition_to";
inline constexpr std::string_view SyntaxKeywordComplete = "complete";
inline constexpr std::string_view SyntaxKeywordFail = "fail";
inline constexpr std::string_view SyntaxKeywordReserve = "reserve";
inline constexpr std::string_view SyntaxKeywordMaterialize = "materialize";
inline constexpr std::string_view SyntaxKeywordReconcile = "reconcile";
inline constexpr std::string_view SyntaxKeywordAllow = "allow";
inline constexpr std::string_view SyntaxKeywordDeny = "deny";
inline constexpr std::string_view SyntaxKeywordQuota = "quota";
inline constexpr std::string_view SyntaxKeywordAudit = "audit";
inline constexpr std::string_view SyntaxKeywordTenant = "tenant";
inline constexpr std::string_view SyntaxKeywordScopedBy = "scoped_by";
inline constexpr std::string_view SyntaxKeywordMethod = "method";
inline constexpr std::string_view SyntaxKeywordPath = "path";
inline constexpr std::string_view SyntaxKeywordInput = "input";
inline constexpr std::string_view SyntaxKeywordOutput = "output";
inline constexpr std::string_view SyntaxKeywordError = "error";
inline constexpr std::string_view SyntaxKeywordAuthz = "authz";
inline constexpr std::string_view SyntaxKeywordStarts = "starts";
inline constexpr std::string_view SyntaxKeywordEnqueues = "enqueues";
inline constexpr std::string_view SyntaxKeywordServes = "serves";
inline constexpr std::string_view SyntaxKeywordPolls = "polls";
inline constexpr std::string_view SyntaxKeywordExecutes = "executes";
inline constexpr std::string_view SyntaxKeywordSingleton = "singleton";
inline constexpr std::string_view SyntaxKeywordConcurrency = "concurrency";

inline constexpr std::string_view SyntaxIdentifierAfter = "after";
inline constexpr std::string_view SyntaxIdentifierBy = "by";
inline constexpr std::string_view SyntaxIdentifierChannel = "channel";
inline constexpr std::string_view SyntaxIdentifierConfigured = "configured";
inline constexpr std::string_view SyntaxIdentifierDeadLetter = "dead_letter";
inline constexpr std::string_view SyntaxIdentifierDefault = "default";
inline constexpr std::string_view SyntaxIdentifierDescription = "description";
inline constexpr std::string_view SyntaxIdentifierDelete = "delete";
inline constexpr std::string_view SyntaxIdentifierEventName = "event_name";
inline constexpr std::string_view SyntaxIdentifierExpectedExecutionTime = "expected_execution_time";
inline constexpr std::string_view SyntaxIdentifierExpires = "expires";
inline constexpr std::string_view SyntaxIdentifierFencingToken = "fencing_token";
inline constexpr std::string_view SyntaxIdentifierGet = "get";
inline constexpr std::string_view SyntaxIdentifierGarbageCollection = "garbage_collection";
inline constexpr std::string_view SyntaxIdentifierHolder = "holder";
inline constexpr std::string_view SyntaxIdentifierIdempotencyKey = "idempotency_key";
inline constexpr std::string_view SyntaxIdentifierIndex = "index";
inline constexpr std::string_view SyntaxIdentifierKind = "kind";
inline constexpr std::string_view SyntaxIdentifierLabels = "labels";
inline constexpr std::string_view SyntaxIdentifierList = "list";
inline constexpr std::string_view SyntaxIdentifierLevel = "level";
inline constexpr std::string_view SyntaxIdentifierMappings = "mappings";
inline constexpr std::string_view SyntaxIdentifierMaxAttempts = "max_attempts";
inline constexpr std::string_view SyntaxIdentifierMaxTtl = "max_ttl";
inline constexpr std::string_view SyntaxIdentifierMetadata = "metadata";
inline constexpr std::string_view SyntaxIdentifierMode = "mode";
inline constexpr std::string_view SyntaxIdentifierName = "name";
inline constexpr std::string_view SyntaxIdentifierOnParentDelete = "on_parent_delete";
inline constexpr std::string_view SyntaxIdentifierOwner = "owner";
inline constexpr std::string_view SyntaxIdentifierParentMustBeIn = "parent_must_be_in";
inline constexpr std::string_view SyntaxIdentifierProfileField = "profile_field";
inline constexpr std::string_view SyntaxIdentifierRenewEvery = "renew_every";
inline constexpr std::string_view SyntaxIdentifierRequiredFields = "required_fields";
inline constexpr std::string_view SyntaxIdentifierResource = "resource";
inline constexpr std::string_view SyntaxIdentifierScope = "scope";
inline constexpr std::string_view SyntaxIdentifierSystemTenant = "system_tenant";
inline constexpr std::string_view SyntaxIdentifierTtl = "ttl";
inline constexpr std::string_view SyntaxIdentifierType = "type";
inline constexpr std::string_view SyntaxIdentifierUnique = "unique";
inline constexpr std::string_view SyntaxIdentifierUniqueWithinParent = "unique_within_parent";
inline constexpr std::string_view SyntaxIdentifierUnit = "unit";
inline constexpr std::string_view SyntaxIdentifierUpdateStatus = "update_status";
inline constexpr std::string_view SyntaxIdentifierUser = "user";
inline constexpr std::string_view SyntaxIdentifierVisibilityTimeout = "visibility_timeout";

inline constexpr std::string_view SyntaxBooleanTrue = "true";
inline constexpr std::string_view SyntaxBooleanFalse = "false";

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
