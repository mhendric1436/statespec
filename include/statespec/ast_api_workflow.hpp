#pragma once

#include "statespec/ast_core.hpp"

namespace statespec
{

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
    std::optional<std::string> from_assignable;
    std::optional<std::string> to_assignable;
    std::optional<std::string> binding;
    std::vector<WorkflowAssignmentDecl> payload;
    std::vector<WorkflowStatementDecl> statements;
    SourceRange range;
};

struct WorkflowChildSetDecl
{
    std::string name;
    std::optional<std::string> entity;
    std::optional<std::string> parent_field;
    std::optional<std::string> id_type;
    std::optional<std::string> pending;
    std::optional<std::string> creating;
    std::optional<std::string> succeeded;
    std::optional<std::string> failed;
    std::optional<std::string> desired_count;
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
    std::vector<WorkflowChildSetDecl> child_sets;
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

} // namespace statespec
