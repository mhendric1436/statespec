#pragma once

#include "statespec/source.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

inline constexpr const char* DefaultTenantIdFieldName = "tenant_id";
inline constexpr const char* DefaultSystemTenantIdConfigKey = "STATESPEC_SYSTEM_TENANT_ID";

struct IncludeDecl
{
    std::string path;
    SourceRange range;
};

struct FieldDecl
{
    std::string name;
    std::string type;
    SourceRange range;
};

struct TenantScopeDecl
{
    std::string field_name = DefaultTenantIdFieldName;
    SourceRange range;
};

enum class SystemTenantSource
{
    Configured,
};

struct SystemTenantDecl
{
    SystemTenantSource source = SystemTenantSource::Configured;
    std::string config_key = DefaultSystemTenantIdConfigKey;
    SourceRange range;
};

struct GarbageCollectionPolicyDecl
{
    std::optional<std::string> after;
    std::optional<std::string> mode;
    SourceRange range;
};

struct StateDecl
{
    std::string name;
    bool terminal = false;
    std::optional<GarbageCollectionPolicyDecl> garbage_collection;
    std::optional<SourceRange> duplicate_garbage_collection_range;
    SourceRange range;
};

struct TransitionDecl
{
    std::string from;
    std::string to;
    SourceRange range;
};

struct StateMachineDecl
{
    std::vector<StateDecl> states;
    std::optional<std::string> initial_state;
    std::vector<std::string> terminal_states;
    std::vector<TransitionDecl> transitions;
    SourceRange range;
};

struct IndexDecl
{
    std::string name;
    std::vector<std::string> fields;
    bool unique = false;
    SourceRange range;
};

struct OwnershipDecl
{
    std::optional<std::string> authority;
    std::optional<std::string> system_of_record;
    std::optional<std::string> lifecycle;
    SourceRange range;
};

struct RelationDecl
{
    std::string kind;
    std::string name;
    std::string target;
    bool optional = false;
    std::optional<std::string> relation_kind;
    std::optional<std::string> on_parent_delete;
    std::vector<std::string> parent_must_be_in;
    std::vector<std::string> unique_within_parent;
    SourceRange range;
};

struct ChildDecl
{
    std::string name;
    std::string target_entity;
    std::string relation;
    SourceRange range;
};

struct InvariantDecl
{
    std::string name;
    std::string expression;
    SourceRange range;
};

struct BlockMemberOrder
{
    std::string kind;
    SourceRange range;
};

struct EntityDecl
{
    std::string name;
    std::vector<std::string> key_fields;
    std::vector<FieldDecl> fields;
    std::vector<IndexDecl> indexes;
    std::optional<OwnershipDecl> ownership;
    std::vector<RelationDecl> relations;
    std::vector<ChildDecl> children;
    std::vector<InvariantDecl> invariants;
    std::optional<StateMachineDecl> state_machine;
    std::vector<BlockMemberOrder> member_order;
    SourceRange range;
};

struct ValueDecl
{
    std::string name;
    std::string type;
    std::optional<std::string> constraint;
    SourceRange range;
};

struct EnumMemberDecl
{
    std::string name;
    std::optional<std::string> value;
    std::optional<std::string> value_kind;
    SourceRange range;
};

struct EnumDecl
{
    std::string name;
    std::vector<EnumMemberDecl> members;
    SourceRange range;
};

struct EventDecl
{
    std::string name;
    std::vector<FieldDecl> fields;
    SourceRange range;
};

struct NamespaceDecl
{
    std::string name;
    std::vector<std::string> members;
    SourceRange range;
};

struct ExternalSystemPropertyDecl
{
    std::string name;
    std::string value;
    std::optional<std::string> value_kind;
    SourceRange range;
};

struct ExternalSystemDecl
{
    std::string name;
    std::vector<ExternalSystemPropertyDecl> properties;
    SourceRange range;
};

struct FeatureFlagDecl
{
    std::string name;
    std::optional<std::string> type;
    std::optional<std::string> default_value;
    std::optional<std::string> default_value_kind;
    std::optional<std::string> scope;
    std::optional<std::string> owner;
    std::optional<std::string> description;
    std::optional<std::string> expires;
    SourceRange range;
};

struct ShapeDecl
{
    std::string name;
    std::vector<FieldDecl> fields;
    SourceRange range;
};

struct LogDecl
{
    std::string name;
    std::optional<std::string> level;
    std::optional<std::string> event_name;
    std::vector<FieldDecl> fields;
    std::vector<BlockMemberOrder> member_order;
    SourceRange range;
};

struct MetricDecl
{
    std::string name;
    std::optional<std::string> kind;
    std::optional<std::string> backend_name;
    std::optional<std::string> unit;
    std::vector<FieldDecl> labels;
    std::vector<BlockMemberOrder> member_order;
    SourceRange range;
};

struct MessageDecl
{
    std::string name;
    std::optional<std::string> idempotency_key;
    std::vector<FieldDecl> payload_fields;
    SourceRange range;
};

struct QueueDecl
{
    std::string name;
    std::optional<std::string> namespace_name;
    std::optional<std::string> channel;
    std::optional<std::string> visibility_timeout;
    std::optional<int> max_attempts;
    std::optional<std::string> dead_letter;
    std::vector<MessageDecl> messages;
    SourceRange range;
};

struct LeaseDecl
{
    std::string name;
    std::optional<std::string> resource;
    std::optional<std::string> ttl;
    std::optional<std::string> renew_every;
    std::optional<std::string> holder;
    std::optional<bool> fencing_token;
    std::optional<std::string> max_ttl;
    SourceRange range;
};

struct WorkerDecl
{
    std::string name;
    std::optional<bool> singleton;
    std::optional<std::string> lease;
    std::optional<std::string> polls;
    std::optional<std::string> executes;
    std::optional<int> concurrency;
    SourceRange range;
};

struct ApiServerDecl
{
    std::string name;
    std::vector<std::string> serves;
    std::optional<int> concurrency;
    SourceRange range;
};

struct ApiDecl
{
    std::string name;
    std::optional<std::string> method;
    std::optional<std::string> path;
    std::optional<std::string> input;
    std::optional<std::string> output;
    std::optional<std::string> error;
    std::optional<std::string> starts_workflow;
    std::optional<std::string> enqueues;
    SourceRange range;
};

struct WorkflowLoadDecl
{
    std::string entity;
    std::string key_field;
    std::string binding;
    SourceRange range;
};

struct WorkflowAssignmentDecl
{
    std::string name;
    std::string expression;
    SourceRange range;
};

struct WorkflowStatementDecl
{
    std::string kind;
    std::optional<std::string> target;
    std::optional<std::string> expression;
    std::optional<std::string> assignable;
    std::optional<std::string> binding;
    std::vector<WorkflowAssignmentDecl> payload;
    SourceRange range;
};

struct WorkflowStepDecl
{
    std::string name;
    std::optional<std::string> expected_execution_time;
    std::optional<int> max_retries;
    std::vector<WorkflowStatementDecl> statements;
    SourceRange range;
};

struct WorkflowDecl
{
    std::string name;
    std::optional<int> version;
    std::optional<bool> singleton;
    std::optional<std::string> expected_execution_time;
    std::optional<std::string> start_step;
    std::optional<std::string> on;
    std::optional<std::string> input;
    std::optional<std::string> state;
    std::vector<WorkflowLoadDecl> loads;
    std::vector<WorkflowStepDecl> steps;
    std::vector<BlockMemberOrder> member_order;
    SourceRange range;
};

struct PolicyRuleDecl
{
    std::string action;
    std::string condition;
    SourceRange range;
};

struct QuotaDecl
{
    std::string name;
    std::string expression;
    SourceRange range;
};

struct PolicyDecl
{
    std::string name;
    std::optional<std::string> tenant_scoped_by;
    std::vector<PolicyRuleDecl> allows;
    std::vector<PolicyRuleDecl> denies;
    std::vector<QuotaDecl> quotas;
    std::vector<std::string> audits;
    std::vector<BlockMemberOrder> member_order;
    SourceRange range;
};

struct SystemDecl
{
    std::string name;
    std::optional<TenantScopeDecl> tenant_scope;
    std::optional<SystemTenantDecl> system_tenant;
    std::vector<NamespaceDecl> namespaces;
    std::vector<ValueDecl> values;
    std::vector<EnumDecl> enums;
    std::vector<EventDecl> events;
    std::vector<ShapeDecl> shapes;
    std::vector<ExternalSystemDecl> external_systems;
    std::vector<FeatureFlagDecl> feature_flags;
    std::vector<LogDecl> logs;
    std::vector<MetricDecl> metrics;
    std::vector<EntityDecl> entities;
    std::vector<QueueDecl> queues;
    std::vector<LeaseDecl> leases;
    std::vector<WorkerDecl> workers;
    std::vector<ApiServerDecl> api_servers;
    std::vector<ApiDecl> apis;
    std::vector<WorkflowDecl> workflows;
    std::vector<PolicyDecl> policies;
    std::vector<BlockMemberOrder> member_order;
    SourceRange range;
};

struct Spec
{
    std::optional<std::string> version;
    std::vector<IncludeDecl> includes;
    std::optional<SystemDecl> system;
};

} // namespace statespec
