#pragma once

#include "statespec/source.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

struct ImportDecl
{
    std::string name;
    std::optional<std::string> alias;
    SourceRange range;
};

struct FieldDecl
{
    std::string name;
    std::string type;
    SourceRange range;
};

struct StateDecl
{
    std::string name;
    bool terminal = false;
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

struct EntityDecl
{
    std::string name;
    std::vector<std::string> key_fields;
    std::vector<FieldDecl> fields;
    std::vector<IndexDecl> indexes;
    std::optional<StateMachineDecl> state_machine;
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

struct WorkflowStepDecl
{
    std::string name;
    std::optional<std::string> expected_execution_time;
    std::optional<int> max_retries;
    SourceRange range;
};

struct WorkflowDecl
{
    std::string name;
    std::optional<int> version;
    std::optional<bool> singleton;
    std::optional<std::string> expected_execution_time;
    std::optional<std::string> start_step;
    std::vector<WorkflowStepDecl> steps;
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
    SourceRange range;
};

struct SystemDecl
{
    std::string name;
    std::vector<EntityDecl> entities;
    std::vector<QueueDecl> queues;
    std::vector<LeaseDecl> leases;
    std::vector<WorkerDecl> workers;
    std::vector<ApiDecl> apis;
    std::vector<WorkflowDecl> workflows;
    std::vector<PolicyDecl> policies;
    SourceRange range;
};

struct Spec
{
    std::optional<std::string> version;
    std::vector<ImportDecl> imports;
    std::optional<SystemDecl> system;
};

} // namespace statespec
