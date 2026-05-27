#pragma once

#include "statespec/ir_core.hpp"

namespace statespec
{

struct IrWorkflowLoad
{
    std::string entity;
    std::string key_field;
    std::string binding;
};

struct IrWorkflowAssignment
{
    std::string name;
    std::string expression;
};

struct IrWorkflowStatement
{
    std::string kind;
    std::optional<std::string> target;
    std::optional<std::string> expression;
    std::optional<std::string> assignable;
    std::optional<std::string> binding;
    std::vector<IrWorkflowAssignment> payload;
};

struct IrWorkflowStep
{
    std::string name;
    std::optional<std::string> expected_execution_time;
    std::optional<int> max_retries;
    std::vector<IrWorkflowStatement> statements;
};

struct IrWorkflow
{
    std::string name;
    std::optional<int> version;
    std::optional<bool> singleton;
    std::optional<std::string> expected_execution_time;
    std::optional<std::string> start_step;
    std::optional<std::string> on;
    std::optional<std::string> input;
    std::optional<std::string> state;
    std::vector<IrWorkflowLoad> loads;
    std::vector<IrWorkflowStep> steps;
};

struct IrApiServer
{
    std::string name;
    std::vector<std::string> serves;
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
    std::optional<std::string> entity;
    std::optional<std::string> repository_operation;
    std::vector<std::string> list_selector;
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

} // namespace statespec
