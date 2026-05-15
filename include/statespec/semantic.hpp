#pragma once

#include "statespec/ast.hpp"
#include "statespec/symbol_table.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

struct SemanticReference
{
    std::string name;
    std::optional<SymbolKind> kind;

    bool resolved() const
    {
        return kind.has_value();
    }
};

struct SemanticField
{
    std::string name;
    std::string type;
};

struct SemanticTenantScope
{
    std::string field_name;
};

struct SemanticSystemTenant
{
    std::string source;
    std::string config_key;
};

struct SemanticGarbageCollectionPolicy
{
    std::string after;
    std::string mode;
};

struct SemanticState
{
    std::string name;
    bool terminal = false;
    std::optional<SemanticGarbageCollectionPolicy> garbage_collection;
};

struct SemanticTransition
{
    std::string from;
    std::string to;
};

struct SemanticIndex
{
    std::string name;
    std::vector<std::string> fields;
    bool unique = false;
};

struct SemanticEntity
{
    std::string name;
    std::vector<std::string> key_fields;
    std::vector<SemanticField> fields;
    std::vector<SemanticIndex> indexes;
    std::vector<SemanticState> states;
    std::optional<std::string> initial_state;
    std::vector<std::string> terminal_states;
    std::vector<SemanticTransition> transitions;
};

struct SemanticFeatureFlag
{
    std::string name;
    std::string type;
    std::string default_value;
    std::string scope;
    std::optional<SemanticReference> scope_target;
    std::optional<std::string> owner;
    std::optional<std::string> description;
    std::optional<std::string> expires;
};

struct SemanticLog
{
    std::string name;
    std::string level;
    std::string event_name;
    std::vector<SemanticField> fields;
};

struct SemanticMetric
{
    std::string name;
    std::string kind;
    std::string backend_name;
    std::string unit;
    std::vector<SemanticField> labels;
};

struct SemanticMessage
{
    std::string name;
    std::optional<std::string> idempotency_key;
    std::vector<SemanticField> payload_fields;
};

struct SemanticQueue
{
    std::string name;
    std::optional<std::string> namespace_name;
    std::optional<std::string> channel;
    std::optional<std::string> visibility_timeout;
    std::optional<int> max_attempts;
    std::optional<SemanticReference> dead_letter;
    std::vector<SemanticMessage> messages;
};

struct SemanticLease
{
    std::string name;
    std::optional<std::string> resource;
    std::optional<std::string> ttl;
    std::optional<std::string> renew_every;
    std::optional<std::string> holder;
    std::optional<bool> fencing_token;
    std::optional<std::string> max_ttl;
};

struct SemanticWorker
{
    std::string name;
    std::optional<bool> singleton;
    std::optional<SemanticReference> lease;
    std::optional<SemanticReference> polls;
    std::optional<SemanticReference> executes;
    std::optional<int> concurrency;
};

struct SemanticApi
{
    std::string name;
    std::optional<std::string> method;
    std::optional<std::string> path;
    std::optional<SemanticReference> input;
    std::optional<SemanticReference> output;
    std::optional<SemanticReference> error;
    std::optional<SemanticReference> starts_workflow;
    std::optional<SemanticReference> enqueues;
};

struct SemanticWorkflowStep
{
    std::string name;
    std::optional<std::string> expected_execution_time;
    std::optional<int> max_retries;
};

struct SemanticWorkflow
{
    std::string name;
    std::optional<int> version;
    std::optional<bool> singleton;
    std::optional<std::string> expected_execution_time;
    std::optional<std::string> start_step;
    std::vector<SemanticWorkflowStep> steps;
};

struct SemanticPolicyRule
{
    SemanticReference action;
    std::string condition;
};

struct SemanticQuota
{
    std::string name;
    std::string expression;
};

struct SemanticPolicy
{
    std::string name;
    std::optional<std::string> tenant_scoped_by;
    std::vector<SemanticPolicyRule> allows;
    std::vector<SemanticPolicyRule> denies;
    std::vector<SemanticQuota> quotas;
    std::vector<SemanticReference> audits;
};

struct SemanticSystem
{
    std::string name;
    std::optional<SemanticTenantScope> tenant_scope;
    std::optional<SemanticSystemTenant> system_tenant;
    std::vector<SemanticFeatureFlag> feature_flags;
    std::vector<SemanticLog> logs;
    std::vector<SemanticMetric> metrics;
    std::vector<SemanticEntity> entities;
    std::vector<SemanticQueue> queues;
    std::vector<SemanticLease> leases;
    std::vector<SemanticWorker> workers;
    std::vector<SemanticApi> apis;
    std::vector<SemanticWorkflow> workflows;
    std::vector<SemanticPolicy> policies;
};

SemanticSystem resolve_semantics(const Spec& spec);

} // namespace statespec
