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
        ITransaction& tx,
        const RegisterWorkflowDefinitionRequest& request
    ) = 0;

    virtual std::optional<WorkflowDefinition> inspect_definition(
        ITransaction& tx,
        const std::string& workflow_name,
        std::int64_t workflow_version
    ) = 0;

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

class Workflow : public IWorkflowStore
{
  public:
    explicit Workflow(IBackend& backend)
        : backend_(backend)
    {
    }

    WorkflowDefinitionRegistration register_definition(
        const RegisterWorkflowDefinitionRequest& request
    )
    {
        auto tx = backend_.begin();
        try
        {
            auto result = register_definition(*tx, request);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    std::optional<WorkflowDefinition> inspect_definition(
        const std::string& workflow_name,
        std::int64_t workflow_version
    )
    {
        auto tx = backend_.begin();
        try
        {
            auto result = inspect_definition(*tx, workflow_name, workflow_version);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    WorkflowExecutionRecord start(const StartWorkflowRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = start(*tx, request);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    std::vector<WorkflowExecutionRecord> claim_steps(const ClaimWorkflowStepRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = claim_steps(*tx, request);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    WorkflowExecutionRecord complete_step(const CompleteWorkflowStepRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = complete_step(*tx, request);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    WorkflowExecutionRecord fail_step(const FailWorkflowStepRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = fail_step(*tx, request);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    WorkflowExecutionRecord cancel(const CancelWorkflowRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = cancel(*tx, request);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    std::optional<WorkflowExecutionRecord> inspect(const std::string& workflow_execution_id)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = inspect(*tx, workflow_execution_id);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

  private:
    IBackend& backend_;
};

} // namespace statespec::backend
