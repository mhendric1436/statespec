#pragma once

#include "backend.hpp"
#include "codec.hpp"

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
        ensure_collections(backend);
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
        const auto key = definition_key(request.definition.queue, request.definition.channel);
        const auto existing =
            inspect_definitionTx(tx, request.definition.queue, request.definition.channel);
        tx.put(kDefinitionsCollection, key, detail::queue_definition_to_json(request.definition));
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
        const auto record = tx.get(kDefinitionsCollection, definition_key(queue, channel));
        if (!record.has_value())
        {
            return std::nullopt;
        }
        return detail::queue_definition_from_json(record->document);
    }

    QueueMessageRecord enqueue(
        IBackend& backend,
        const EnqueueMessageRequest& request
    ) override
    {
        ensure_collections(backend);
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
        if (request.idempotency_key.has_value())
        {
            const auto idempotency_key =
                definition_key(request.queue, request.channel) + "|" + *request.idempotency_key;
            if (const auto existing = tx.get(kIdempotencyCollection, idempotency_key);
                existing.has_value())
            {
                return *inspectTx(tx, existing->document.at("message_id").as_string());
            }
            tx.put(
                kIdempotencyCollection, idempotency_key,
                Json::object({{"message_id", request.message_id}})
            );
        }

        QueueMessageRecord record;
        record.message_id = request.message_id;
        record.queue = request.queue;
        record.channel = request.channel;
        record.status = "Pending";
        record.payload = request.payload;
        tx.put(kMessagesCollection, record.message_id, detail::queue_message_to_json(record));
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
        auto messages = all_messages(tx);
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
            tx.put(kMessagesCollection, message.message_id, detail::queue_message_to_json(message));
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
        auto message = require_message(tx, request.message_id);
        require_claimant(message, request.claimant);
        message.status = "Acked";
        tx.put(kMessagesCollection, message.message_id, detail::queue_message_to_json(message));
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
        auto message = require_message(tx, request.message_id);
        require_claimant(message, request.claimant);
        message.claimed_by.reset();
        message.claim_expires_at.reset();
        message.status = message.attempts >= request.max_attempts ? "DeadLettered" : "Pending";
        tx.put(kMessagesCollection, message.message_id, detail::queue_message_to_json(message));
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
        const auto record = tx.get(kMessagesCollection, message_id);
        if (!record.has_value())
        {
            return std::nullopt;
        }
        return detail::queue_message_from_json(record->document);
    }

  private:
    static constexpr const char* kDefinitionsCollection = "statespec_queue_definitions";
    static constexpr const char* kMessagesCollection = "statespec_queue_messages";
    static constexpr const char* kIdempotencyCollection = "statespec_queue_idempotency";

    static void ensure_collections(IBackend& backend)
    {
        backend.ensure_collections(
            {CollectionDescriptor{.name = kDefinitionsCollection, .key_fields = {"queue"}},
             CollectionDescriptor{.name = kMessagesCollection, .key_fields = {"message_id"}},
             CollectionDescriptor{
                 .name = kIdempotencyCollection, .key_fields = {"idempotency_key"}
             }}
        );
    }

    static std::string definition_key(
        const std::string& queue,
        const std::string& channel
    )
    {
        return queue + ":" + channel;
    }

    static std::vector<QueueMessageRecord> all_messages(ITransaction& tx)
    {
        std::vector<QueueMessageRecord> messages;
        for (const auto& record : tx.query(kMessagesCollection, Query::all()))
        {
            messages.push_back(detail::queue_message_from_json(record.document));
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
