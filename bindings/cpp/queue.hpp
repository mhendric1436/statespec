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

struct RegisterQueueDefinitionRequest
{
    QueueDefinition definition;
};

struct QueueDefinitionRegistration
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

    virtual QueueDefinitionRegistration register_definition(
        IBackend& backend,
        const RegisterQueueDefinitionRequest& request
    ) = 0;

    virtual QueueDefinitionRegistration register_definitionTx(
        ITransaction& tx,
        const RegisterQueueDefinitionRequest& request
    ) = 0;

    virtual std::optional<QueueDefinition> inspect_definition(
        IBackend& backend,
        const std::string& queue,
        const std::string& channel
    ) = 0;

    virtual std::optional<QueueDefinition> inspect_definitionTx(
        ITransaction& tx,
        const std::string& queue,
        const std::string& channel
    ) = 0;

    virtual QueueMessageRecord enqueue(
        IBackend& backend,
        const EnqueueMessageRequest& request
    ) = 0;

    virtual QueueMessageRecord enqueueTx(
        ITransaction& tx,
        const EnqueueMessageRequest& request
    ) = 0;

    virtual std::vector<QueueMessageRecord> claim(
        IBackend& backend,
        const ClaimMessageRequest& request
    ) = 0;

    virtual std::vector<QueueMessageRecord> claimTx(
        ITransaction& tx,
        const ClaimMessageRequest& request
    ) = 0;

    virtual void acknowledge(
        IBackend& backend,
        const AckMessageRequest& request
    ) = 0;

    virtual void acknowledgeTx(
        ITransaction& tx,
        const AckMessageRequest& request
    ) = 0;

    virtual QueueMessageRecord fail(
        IBackend& backend,
        const FailMessageRequest& request
    ) = 0;

    virtual QueueMessageRecord failTx(
        ITransaction& tx,
        const FailMessageRequest& request
    ) = 0;

    virtual std::optional<QueueMessageRecord> inspect(
        IBackend& backend,
        const std::string& message_id
    ) = 0;

    virtual std::optional<QueueMessageRecord> inspectTx(
        ITransaction& tx,
        const std::string& message_id
    ) = 0;
};

} // namespace statespec::backend
