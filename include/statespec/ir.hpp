#pragma once

#include "statespec/ast.hpp"
#include "statespec/semantic.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

struct IrField
{
    std::string name;
    std::string type;
};

struct IrTenantScope
{
    std::string field_name;
};

struct IrSystemTenant
{
    std::string source;
    std::string config_key;
};

struct IrGarbageCollectionPolicy
{
    std::string after;
    std::string mode;
};

struct IrState
{
    std::string name;
    bool terminal = false;
    std::optional<IrGarbageCollectionPolicy> garbage_collection;
};

struct IrTransition
{
    std::string from;
    std::string to;
};

struct IrIndex
{
    std::string name;
    std::vector<std::string> fields;
    bool unique = false;
};

struct IrEntity
{
    std::string name;
    std::vector<std::string> key_fields;
    std::vector<IrField> fields;
    std::vector<IrIndex> indexes;
    std::vector<IrState> states;
    std::optional<std::string> initial_state;
    std::vector<std::string> terminal_states;
    std::vector<IrTransition> transitions;
};

struct IrMessage
{
    std::string name;
    std::optional<std::string> idempotency_key;
    std::vector<IrField> payload_fields;
};

struct IrQueue
{
    std::string name;
    std::optional<std::string> namespace_name;
    std::optional<std::string> channel;
    std::optional<std::string> visibility_timeout;
    std::optional<int> max_attempts;
    std::optional<std::string> dead_letter;
    std::vector<IrMessage> messages;
};

struct IrLease
{
    std::string name;
    std::optional<std::string> resource;
    std::optional<std::string> ttl;
    std::optional<std::string> renew_every;
    std::optional<std::string> holder;
    std::optional<bool> fencing_token;
    std::optional<std::string> max_ttl;
};

struct IrWorkflowStep
{
    std::string name;
    std::optional<std::string> expected_execution_time;
    std::optional<int> max_retries;
};

struct IrWorkflow
{
    std::string name;
    std::optional<int> version;
    std::optional<bool> singleton;
    std::optional<std::string> expected_execution_time;
    std::optional<std::string> start_step;
    std::vector<IrWorkflowStep> steps;
};

struct IrFeatureFlag
{
    std::string name;
    std::string type;
    std::string default_value;
    std::string scope;
    std::optional<std::string> owner;
    std::optional<std::string> description;
    std::optional<std::string> expires;
};

struct IrShape
{
    std::string name;
    std::vector<IrField> fields;
};

struct IrLog
{
    std::string name;
    std::string level;
    std::string event_name;
    std::vector<IrField> fields;
};

struct IrMetric
{
    std::string name;
    std::string kind;
    std::string backend_name;
    std::string unit;
    std::vector<IrField> labels;
};

struct IrWorker
{
    std::string name;
    std::optional<bool> singleton;
    std::optional<std::string> lease;
    std::optional<std::string> polls;
    std::optional<std::string> executes;
    std::optional<int> concurrency;
};

struct IrApi
{
    std::string name;
    std::optional<std::string> method;
    std::optional<std::string> path;
    std::optional<std::string> input;
    std::optional<std::string> output;
    std::optional<std::string> error;
    std::optional<std::string> starts_workflow;
    std::optional<std::string> enqueues;
};

struct IrPolicyRule
{
    std::string action;
    std::string condition;
};

struct IrQuota
{
    std::string name;
    std::string expression;
};

struct IrPolicy
{
    std::string name;
    std::optional<std::string> tenant_scoped_by;
    std::vector<IrPolicyRule> allows;
    std::vector<IrPolicyRule> denies;
    std::vector<IrQuota> quotas;
    std::vector<std::string> audits;
};

struct IrSystem
{
    std::string name;
    std::optional<IrTenantScope> tenant_scope;
    std::optional<IrSystemTenant> system_tenant;
    std::vector<IrShape> shapes;
    std::vector<IrFeatureFlag> feature_flags;
    std::vector<IrLog> logs;
    std::vector<IrMetric> metrics;
    std::vector<IrEntity> entities;
    std::vector<IrQueue> queues;
    std::vector<IrLease> leases;
    std::vector<IrWorker> workers;
    std::vector<IrApi> apis;
    std::vector<IrWorkflow> workflows;
    std::vector<IrPolicy> policies;
};

IrSystem lower_to_ir(const SemanticSystem& system);
IrSystem lower_to_ir(const Spec& spec);

} // namespace statespec
