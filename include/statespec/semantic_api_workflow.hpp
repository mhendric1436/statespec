#pragma once

#include "statespec/semantic_core.hpp"

namespace statespec
{

struct SemanticApiServer
{
    std::string name;
    std::vector<SemanticReference> serves;
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

struct SemanticWorkflowLoad
{
    SemanticReference entity;
    std::string key_field;
    std::string binding;
};

struct SemanticWorkflowAssignment
{
    std::string name;
    std::string expression;
};

struct SemanticWorkflowStatement
{
    std::string kind;
    std::optional<std::string> target;
    std::optional<SemanticReference> resolved_target;
    std::optional<std::string> expression;
    std::optional<std::string> assignable;
    std::optional<std::string> binding;
    std::vector<SemanticWorkflowAssignment> payload;
};

struct SemanticWorkflowStep
{
    std::string name;
    std::optional<std::string> expected_execution_time;
    std::optional<int> max_retries;
    std::vector<SemanticWorkflowStatement> statements;
};

struct SemanticWorkflow
{
    std::string name;
    std::optional<int> version;
    std::optional<bool> singleton;
    std::optional<std::string> expected_execution_time;
    std::optional<std::string> start_step;
    std::optional<SemanticReference> on;
    std::optional<SemanticReference> input;
    std::optional<SemanticReference> state;
    std::vector<SemanticWorkflowLoad> loads;
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

} // namespace statespec
