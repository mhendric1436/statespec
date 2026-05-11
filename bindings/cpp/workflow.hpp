#pragma once

#include "backend.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace statespec::backend
{

using Timestamp = std::chrono::system_clock::time_point;

struct WorkflowStepDefinition
{
    std::string name;
    std::chrono::seconds expected_execution_time;
    std::uint32_t max_retries = 0;
};

struct WorkflowDefinition
{
    std::string workflow_name;
    std::int64_t workflow_version = 0;
    std::string start_step;
    std::chrono::seconds expected_execution_time;
    bool singleton = false;
    std::vector<WorkflowStepDefinition> steps;
    Json metadata;
};

struct RegisterWorkflowDefinitionRequest
{
    WorkflowDefinition definition;
};

struct WorkflowDefinitionRegistration
{
    WorkflowDefinition definition;
    bool created = false;
};

struct WorkflowExecutionRecord
{
    std::string workflow_execution_id;
    std::string workflow_name;
    std::int64_t workflow_version = 0;
    std::string current_step;
    std::string status;
    std::uint64_t attempt = 0;
    std::optional<std::string> claimed_by;
    std::optional<Timestamp> claim_expires_at;
    Json state;
};

struct StartWorkflowRequest
{
    std::string workflow_execution_id;
    std::string workflow_name;
    std::int64_t workflow_version = 0;
    std::string start_step;
    Json state;
};

struct ClaimWorkflowStepRequest
{
    std::string workflow_execution_id;
    std::string workflow_name;
    std::int64_t workflow_version = 0;
    std::string worker;
    Timestamp now;
    std::chrono::seconds lease_duration;
    std::uint32_t max_steps = 1;
};

struct CompleteWorkflowStepRequest
{
    std::string workflow_execution_id;
    std::string worker;
    std::string completed_step;
    std::optional<std::string> next_step;
    Json state;
};

struct FailWorkflowStepRequest
{
    std::string workflow_execution_id;
    std::string worker;
    std::string failed_step;
    std::string reason;
    Timestamp now;
    std::uint32_t max_attempts = 1;
};

struct CancelWorkflowRequest
{
    std::string workflow_execution_id;
    std::string reason;
};

class IWorkflowStore
{
  public:
    virtual ~IWorkflowStore() = default;

    virtual WorkflowDefinitionRegistration register_definition(
        IBackend& backend,
        const RegisterWorkflowDefinitionRequest& request
    ) = 0;

    virtual WorkflowDefinitionRegistration register_definition(
        ITransaction& tx,
        const RegisterWorkflowDefinitionRequest& request
    ) = 0;

    virtual std::optional<WorkflowDefinition> inspect_definition(
        IBackend& backend,
        const std::string& workflow_name,
        std::int64_t workflow_version
    ) = 0;

    virtual std::optional<WorkflowDefinition> inspect_definition(
        ITransaction& tx,
        const std::string& workflow_name,
        std::int64_t workflow_version
    ) = 0;

    virtual WorkflowExecutionRecord start(
        IBackend& backend,
        const StartWorkflowRequest& request
    ) = 0;

    virtual WorkflowExecutionRecord start(
        ITransaction& tx,
        const StartWorkflowRequest& request
    ) = 0;

    virtual std::vector<WorkflowExecutionRecord> claim_steps(
        IBackend& backend,
        const ClaimWorkflowStepRequest& request
    ) = 0;

    virtual std::vector<WorkflowExecutionRecord> claim_steps(
        ITransaction& tx,
        const ClaimWorkflowStepRequest& request
    ) = 0;

    virtual WorkflowExecutionRecord complete_step(
        IBackend& backend,
        const CompleteWorkflowStepRequest& request
    ) = 0;

    virtual WorkflowExecutionRecord complete_step(
        ITransaction& tx,
        const CompleteWorkflowStepRequest& request
    ) = 0;

    virtual WorkflowExecutionRecord fail_step(
        IBackend& backend,
        const FailWorkflowStepRequest& request
    ) = 0;

    virtual WorkflowExecutionRecord fail_step(
        ITransaction& tx,
        const FailWorkflowStepRequest& request
    ) = 0;

    virtual WorkflowExecutionRecord cancel(
        IBackend& backend,
        const CancelWorkflowRequest& request
    ) = 0;

    virtual WorkflowExecutionRecord cancel(
        ITransaction& tx,
        const CancelWorkflowRequest& request
    ) = 0;

    virtual std::optional<WorkflowExecutionRecord> inspect(
        IBackend& backend,
        const std::string& workflow_execution_id
    ) = 0;

    virtual std::optional<WorkflowExecutionRecord> inspect(
        ITransaction& tx,
        const std::string& workflow_execution_id
    ) = 0;
};

} // namespace statespec::backend
