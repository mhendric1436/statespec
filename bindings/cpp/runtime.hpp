#pragma once

#include "backend.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace statespec::backend
{

struct LeaseAcquireRequest
{
    std::string resource;
    std::string holder;
    Timestamp now;
    std::chrono::seconds ttl;
};

struct LeaseRenewRequest
{
    std::string resource;
    std::string holder;
    std::uint64_t fencing_token = 0;
    Timestamp now;
    std::chrono::seconds ttl;
};

struct LeaseReleaseRequest
{
    std::string resource;
    std::string holder;
    std::uint64_t fencing_token = 0;
};

struct LeaseAcquireResult
{
    bool acquired = false;
    std::optional<LeaseRecord> lease;
};

class ILeaseStore
{
  public:
    virtual ~ILeaseStore() = default;

    virtual LeaseAcquireResult acquire(
        ITransaction& tx,
        const LeaseAcquireRequest& request
    ) = 0;

    virtual LeaseRecord renew(
        ITransaction& tx,
        const LeaseRenewRequest& request
    ) = 0;

    virtual void release(
        ITransaction& tx,
        const LeaseReleaseRequest& request
    ) = 0;

    virtual std::optional<LeaseRecord> inspect(
        ITransaction& tx,
        const std::string& resource
    ) = 0;
};

struct EnqueueMessageRequest
{
    std::string message_id;
    std::string queue;
    std::string channel;
    std::optional<std::string> idempotency_key;
    Json payload;
};

struct ClaimMessageRequest
{
    std::string queue;
    std::string channel;
    std::string claimant;
    Timestamp now;
    std::chrono::seconds visibility_timeout;
    std::uint32_t max_messages = 1;
};

struct AckMessageRequest
{
    std::string message_id;
    std::string claimant;
};

struct FailMessageRequest
{
    std::string message_id;
    std::string claimant;
    std::string reason;
    Timestamp now;
    std::uint32_t max_attempts = 1;
};

class IQueueStore
{
  public:
    virtual ~IQueueStore() = default;

    virtual QueueMessageRecord enqueue(
        ITransaction& tx,
        const EnqueueMessageRequest& request
    ) = 0;

    virtual std::vector<QueueMessageRecord> claim(
        ITransaction& tx,
        const ClaimMessageRequest& request
    ) = 0;

    virtual void acknowledge(
        ITransaction& tx,
        const AckMessageRequest& request
    ) = 0;

    virtual QueueMessageRecord fail(
        ITransaction& tx,
        const FailMessageRequest& request
    ) = 0;

    virtual std::optional<QueueMessageRecord> inspect(
        ITransaction& tx,
        const std::string& message_id
    ) = 0;
};

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
