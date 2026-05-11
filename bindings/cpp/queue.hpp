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

struct QueueDefinition
{
    std::string queue;
    std::string channel;
    std::chrono::seconds visibility_timeout;
    std::uint32_t max_attempts = 1;
    std::optional<std::string> dead_letter_queue;
    Json metadata;
};

struct CreateQueueRequest
{
    QueueDefinition definition;
};

struct QueueCreation
{
    QueueDefinition definition;
    bool created = false;
};

struct QueueMessageRecord
{
    std::string message_id;
    std::string queue;
    std::string channel;
    std::string status;
    std::uint64_t attempts = 0;
    std::optional<std::string> claimed_by;
    std::optional<Timestamp> claim_expires_at;
    Json payload;
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

    virtual QueueCreation create(
        ITransaction& tx,
        const CreateQueueRequest& request
    ) = 0;

    virtual std::optional<QueueDefinition> inspect_definition(
        ITransaction& tx,
        const std::string& queue,
        const std::string& channel
    ) = 0;

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

class Queue : public IQueueStore
{
  public:
    explicit Queue(IBackend& backend)
        : backend_(backend)
    {
    }

    QueueCreation create(const CreateQueueRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = create(*tx, request);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    std::optional<QueueDefinition> inspect_definition(
        const std::string& queue,
        const std::string& channel
    )
    {
        auto tx = backend_.begin();
        try
        {
            auto result = inspect_definition(*tx, queue, channel);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    QueueMessageRecord enqueue(const EnqueueMessageRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = enqueue(*tx, request);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    std::vector<QueueMessageRecord> claim(const ClaimMessageRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = claim(*tx, request);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    void acknowledge(const AckMessageRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            acknowledge(*tx, request);
            backend_.commit(*tx);
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    QueueMessageRecord fail(const FailMessageRequest& request)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = fail(*tx, request);
            backend_.commit(*tx);
            return result;
        }
        catch (...)
        {
            tx->abort();
            throw;
        }
    }

    std::optional<QueueMessageRecord> inspect(const std::string& message_id)
    {
        auto tx = backend_.begin();
        try
        {
            auto result = inspect(*tx, message_id);
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
