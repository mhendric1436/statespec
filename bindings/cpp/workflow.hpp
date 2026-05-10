#pragma once

#include "backend.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace statespec::backend
{

struct StartWorkflowRequest
{
    std::string workflow_execution_id;
    std::string workflow_name;
    std::string start_step;
    Json state;
};

struct ClaimWorkflowStepRequest
{
    std::string workflow_name;
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

    virtual WorkflowExecutionRecord start(
        ITransaction& tx,
        const StartWorkflowRequest& request
    ) = 0;

    virtual std::vector<WorkflowExecutionRecord> claim_steps(
        ITransaction& tx,
        const ClaimWorkflowStepRequest& request
    ) = 0;

    virtual WorkflowExecutionRecord complete_step(
        ITransaction& tx,
        const CompleteWorkflowStepRequest& request
    ) = 0;

    virtual WorkflowExecutionRecord fail_step(
        ITransaction& tx,
        const FailWorkflowStepRequest& request
    ) = 0;

    virtual WorkflowExecutionRecord cancel(
        ITransaction& tx,
        const CancelWorkflowRequest& request
    ) = 0;

    virtual std::optional<WorkflowExecutionRecord> inspect(
        ITransaction& tx,
        const std::string& workflow_execution_id
    ) = 0;
};

} // namespace statespec::backend
