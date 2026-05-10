#pragma once

#include "backend.hpp"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace statespec::backend
{

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

} // namespace statespec::backend
