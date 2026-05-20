#pragma once

#include "backend.hpp"

#include <algorithm>

namespace statespec::backend::memory
{

class InMemoryQueueStore : public IQueueStore
{
  public:
    QueueDefinitionRegistration register_definition(
        IBackend& backend,
        const RegisterQueueDefinitionRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = register_definitionTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    QueueDefinitionRegistration register_definitionTx(
        ITransaction& tx,
        const RegisterQueueDefinitionRequest& request
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        const auto key = definition_key(request.definition.queue, request.definition.channel);
        const auto existing =
            inspect_definitionTx(memory_tx, request.definition.queue, request.definition.channel);
        memory_tx.queue_definition_puts().insert_or_assign(key, request.definition);
        return QueueDefinitionRegistration{request.definition, !existing.has_value()};
    }

    std::optional<QueueDefinition> inspect_definition(
        IBackend& backend,
        const std::string& queue,
        const std::string& channel
    ) override
    {
        auto tx = backend.begin();
        auto result = inspect_definitionTx(*tx, queue, channel);
        backend.commit(*tx);
        return result;
    }

    std::optional<QueueDefinition> inspect_definitionTx(
        ITransaction& tx,
        const std::string& queue,
        const std::string& channel
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        const auto key = definition_key(queue, channel);
        if (const auto staged = memory_tx.queue_definition_puts().find(key);
            staged != memory_tx.queue_definition_puts().end())
        {
            return staged->second;
        }
        std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
        if (const auto iter = memory_tx.state().queue_definitions.find(key);
            iter != memory_tx.state().queue_definitions.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

    QueueMessageRecord enqueue(
        IBackend& backend,
        const EnqueueMessageRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = enqueueTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    QueueMessageRecord enqueueTx(
        ITransaction& tx,
        const EnqueueMessageRequest& request
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        if (request.idempotency_key.has_value())
        {
            const auto idempotency_key =
                definition_key(request.queue, request.channel) + "|" + *request.idempotency_key;
            if (const auto staged = memory_tx.queue_idempotency_puts().find(idempotency_key);
                staged != memory_tx.queue_idempotency_puts().end())
            {
                return *inspectTx(memory_tx, staged->second);
            }
            std::optional<std::string> existing_message_id;
            {
                std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
                if (const auto iter =
                        memory_tx.state().queue_idempotency_keys.find(idempotency_key);
                    iter != memory_tx.state().queue_idempotency_keys.end())
                {
                    existing_message_id = iter->second;
                }
            }
            if (existing_message_id.has_value())
            {
                return *inspectTx(memory_tx, *existing_message_id);
            }
            memory_tx.queue_idempotency_puts().insert_or_assign(
                idempotency_key, request.message_id
            );
        }

        QueueMessageRecord record;
        record.message_id = request.message_id;
        record.queue = request.queue;
        record.channel = request.channel;
        record.status = "Pending";
        record.payload = request.payload;
        memory_tx.queue_message_puts().insert_or_assign(record.message_id, record);
        return record;
    }

    std::vector<QueueMessageRecord> claim(
        IBackend& backend,
        const ClaimMessageRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = claimTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    std::vector<QueueMessageRecord> claimTx(
        ITransaction& tx,
        const ClaimMessageRequest& request
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        auto messages = all_messages(memory_tx);
        std::vector<QueueMessageRecord> claimed;
        for (auto& message : messages)
        {
            if (claimed.size() >= request.max_messages)
            {
                break;
            }
            if (message.queue != request.queue || message.channel != request.channel ||
                message.status == "Acked" || message.status == "DeadLettered")
            {
                continue;
            }
            const auto visible =
                message.status == "Pending" ||
                (message.claim_expires_at.has_value() && *message.claim_expires_at <= request.now);
            if (!visible)
            {
                continue;
            }
            message.status = "Claimed";
            message.claimed_by = request.claimant;
            message.claim_expires_at = request.now + request.visibility_timeout;
            ++message.attempts;
            memory_tx.queue_message_puts().insert_or_assign(message.message_id, message);
            claimed.push_back(message);
        }
        return claimed;
    }

    void acknowledge(
        IBackend& backend,
        const AckMessageRequest& request
    ) override
    {
        auto tx = backend.begin();
        acknowledgeTx(*tx, request);
        backend.commit(*tx);
    }

    void acknowledgeTx(
        ITransaction& tx,
        const AckMessageRequest& request
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        auto message = require_message(memory_tx, request.message_id);
        require_claimant(message, request.claimant);
        message.status = "Acked";
        memory_tx.queue_message_puts().insert_or_assign(message.message_id, message);
    }

    QueueMessageRecord fail(
        IBackend& backend,
        const FailMessageRequest& request
    ) override
    {
        auto tx = backend.begin();
        auto result = failTx(*tx, request);
        backend.commit(*tx);
        return result;
    }

    QueueMessageRecord failTx(
        ITransaction& tx,
        const FailMessageRequest& request
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        auto message = require_message(memory_tx, request.message_id);
        require_claimant(message, request.claimant);
        message.claimed_by.reset();
        message.claim_expires_at.reset();
        message.status = message.attempts >= request.max_attempts ? "DeadLettered" : "Pending";
        memory_tx.queue_message_puts().insert_or_assign(message.message_id, message);
        return message;
    }

    std::optional<QueueMessageRecord> inspect(
        IBackend& backend,
        const std::string& message_id
    ) override
    {
        auto tx = backend.begin();
        auto result = inspectTx(*tx, message_id);
        backend.commit(*tx);
        return result;
    }

    std::optional<QueueMessageRecord> inspectTx(
        ITransaction& tx,
        const std::string& message_id
    ) override
    {
        auto& memory_tx = as_memory_tx(tx);
        if (const auto staged = memory_tx.queue_message_puts().find(message_id);
            staged != memory_tx.queue_message_puts().end())
        {
            return staged->second;
        }
        std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
        if (const auto iter = memory_tx.state().queue_messages.find(message_id);
            iter != memory_tx.state().queue_messages.end())
        {
            return iter->second;
        }
        return std::nullopt;
    }

  private:
    static std::string definition_key(
        const std::string& queue,
        const std::string& channel
    )
    {
        return queue + ":" + channel;
    }

    static std::vector<QueueMessageRecord> all_messages(InMemoryTransaction& memory_tx)
    {
        std::vector<QueueMessageRecord> messages;
        {
            std::lock_guard<std::mutex> lock(memory_tx.state().mutex);
            for (const auto& [_, message] : memory_tx.state().queue_messages)
            {
                messages.push_back(message);
            }
        }
        for (const auto& [_, message] : memory_tx.queue_message_puts())
        {
            messages.push_back(message);
        }
        return messages;
    }

    static QueueMessageRecord require_message(
        ITransaction& tx,
        const std::string& message_id
    )
    {
        InMemoryQueueStore store;
        auto message = store.inspectTx(tx, message_id);
        if (!message.has_value())
        {
            throw BackendError("unknown queue message: " + message_id);
        }
        return *message;
    }

    static void require_claimant(
        const QueueMessageRecord& message,
        const std::string& claimant
    )
    {
        if (!message.claimed_by.has_value() || *message.claimed_by != claimant)
        {
            throw ConflictError(
                ConflictKind::QueueClaimConflict, "queue message is not claimed by caller"
            );
        }
    }
};

} // namespace statespec::backend::memory
