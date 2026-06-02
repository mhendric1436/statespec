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
    std::optional<std::string> from_assignable;
    std::optional<std::string> to_assignable;
    std::optional<std::string> binding;
    std::vector<IrWorkflowAssignment> payload;
    std::vector<IrWorkflowStatement> statements;
};

struct IrWorkflowChildSet
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
};

struct IrWorkflowChildWorkflow
{
    std::string name;
    std::optional<std::string> child_entity;
    std::optional<std::string> child_workflow;
    std::optional<std::string> child_id_field;
    std::optional<std::string> child_id_type;
    std::optional<std::string> parent_ref_field;
    std::optional<std::string> parent_ref_expression;
    std::optional<std::string> desired_count;
    std::vector<IrWorkflowAssignment> create_assignments;
    std::optional<std::string> success_expression;
    std::optional<std::string> failure_expression;
    std::optional<std::string> id_bucket_base;
    std::optional<std::string> pending_bucket;
    std::optional<std::string> creating_bucket;
    std::optional<std::string> succeeded_bucket;
    std::optional<std::string> failed_bucket;
    std::optional<std::string> generate_ids_step;
    std::optional<std::string> create_children_step;
    std::optional<std::string> wait_children_step;
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
    std::vector<IrWorkflowChildSet> child_sets;
    std::vector<IrWorkflowChildWorkflow> child_workflows;
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
    // Entity-owned CRUD APIs are identified by entity + repository_operation.
    // Generators must use this metadata instead of operation-name heuristics when
    // deciding whether to emit generated CRUD handlers.
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
