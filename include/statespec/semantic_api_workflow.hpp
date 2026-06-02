#pragma once

#include "statespec/child_workflow_names.hpp"
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
    std::optional<std::string> from_assignable;
    std::optional<std::string> to_assignable;
    std::optional<std::string> binding;
    std::vector<SemanticWorkflowAssignment> payload;
    std::vector<SemanticWorkflowStatement> statements;
};

struct SemanticWorkflowChildSet
{
    std::string name;
    std::optional<SemanticReference> entity;
    std::optional<std::string> parent_field;
    std::optional<std::string> id_type;
    std::optional<std::string> pending;
    std::optional<std::string> creating;
    std::optional<std::string> succeeded;
    std::optional<std::string> failed;
    std::optional<std::string> desired_count;
};

struct SemanticWorkflowChildWorkflow
{
    std::string name;
    std::optional<SemanticReference> child_entity;
    std::optional<SemanticReference> child_workflow;
    std::optional<std::string> child_id_field;
    std::optional<std::string> child_id_type;
    std::optional<std::string> parent_ref_field;
    std::optional<std::string> parent_ref_expression;
    std::optional<std::string> desired_count;
    std::vector<SemanticWorkflowAssignment> create_assignments;
    std::optional<std::string> success_expression;
    std::optional<std::string> failure_expression;
    std::optional<ChildWorkflowDerivedNames> derived_names;
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
    std::vector<SemanticWorkflowChildSet> child_sets;
    std::vector<SemanticWorkflowChildWorkflow> child_workflows;
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
