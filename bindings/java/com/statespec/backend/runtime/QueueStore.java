package com.statespec.backend.runtime;

import com.statespec.backend.Backend;
import com.statespec.backend.Json;
import com.statespec.backend.Queue;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public final class QueueStore implements Queue
{
    private static final String DEFINITIONS = Backend.RuntimeCollections.QUEUE_DEFINITIONS;
    private static final String MESSAGES = Backend.RuntimeCollections.QUEUE_MESSAGES;
    private static final String IDEMPOTENCY = Backend.RuntimeCollections.QUEUE_IDEMPOTENCY;

    @Override
    public QueueDefinitionRegistration registerDefinition(
        Backend backend,
        RegisterQueueDefinitionRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = registerDefinitionTx(tx, request);
            backend.commit(tx);
            return result;
        }
        catch (Backend.BackendException error)
        {
            tx.abort();
            throw error;
        }
    }

    @Override
    public QueueDefinitionRegistration registerDefinitionTx(
        Backend.Transaction tx,
        RegisterQueueDefinitionRequest request
    ) throws Backend.BackendException
    {
        var existing =
            inspectDefinitionTx(tx, request.definition().queue(), request.definition().channel());
        tx.put(
            DEFINITIONS,
            queueDefinitionKey(request.definition().queue(), request.definition().channel()),
            QueueCodec.queueDefinitionToJson(request.definition())
        );
        return new QueueDefinitionRegistration(request.definition(), existing.isEmpty());
    }

    @Override
    public Optional<QueueDefinition> inspectDefinition(
        Backend backend,
        String queue,
        String channel
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        var result = inspectDefinitionTx(tx, queue, channel);
        backend.commit(tx);
        return result;
    }

    @Override
    public Optional<QueueDefinition> inspectDefinitionTx(
        Backend.Transaction tx,
        String queue,
        String channel
    ) throws Backend.BackendException
    {
        var key = queueDefinitionKey(queue, channel);
        return tx.get(DEFINITIONS, key)
            .map(record -> QueueCodec.queueDefinitionFromJson(record.document()));
    }

    @Override
    public QueueMessageRecord enqueue(
        Backend backend,
        EnqueueMessageRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = enqueueTx(tx, request);
            backend.commit(tx);
            return result;
        }
        catch (Backend.BackendException error)
        {
            tx.abort();
            throw error;
        }
    }

    @Override
    public QueueMessageRecord enqueueTx(
        Backend.Transaction tx,
        EnqueueMessageRequest request
    ) throws Backend.BackendException
    {
        if (request.idempotencyKey().isPresent())
        {
            var idempotencyKey = queueDefinitionKey(request.queue(), request.channel()) + "|" +
                                 request.idempotencyKey().get();
            var existing = tx.get(IDEMPOTENCY, idempotencyKey);
            if (existing.isPresent())
            {
                return requireMessage(tx, Codec.stringFromJson(existing.get().document()));
            }
            tx.put(IDEMPOTENCY, idempotencyKey, Json.string(request.messageId()));
        }
        var record = new QueueMessageRecord(
            request.messageId(), request.queue(), request.channel(), "Pending", 0L,
            Optional.empty(), Optional.empty(), request.payloadJson()
        );
        tx.put(MESSAGES, record.messageId(), QueueCodec.queueMessageToJson(record));
        return record;
    }

    @Override
    public List<QueueMessageRecord> claim(
        Backend backend,
        ClaimMessageRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = claimTx(tx, request);
            backend.commit(tx);
            return result;
        }
        catch (Backend.BackendException error)
        {
            tx.abort();
            throw error;
        }
    }

    @Override
    public List<QueueMessageRecord> claimTx(
        Backend.Transaction tx,
        ClaimMessageRequest request
    ) throws Backend.BackendException
    {
        var claimed = new ArrayList<QueueMessageRecord>();
        for (var message : allMessages(tx))
        {
            if (claimed.size() >= request.maxMessages())
            {
                break;
            }
            if (!message.queue().equals(request.queue()) ||
                !message.channel().equals(request.channel()) || message.status().equals("Acked") ||
                message.status().equals("DeadLettered"))
            {
                continue;
            }
            var visible = message.status().equals("Pending") ||
                          (message.claimExpiresAt().isPresent() &&
                           !message.claimExpiresAt().get().isAfter(request.now()));
            if (!visible)
            {
                continue;
            }
            var updated = new QueueMessageRecord(
                message.messageId(), message.queue(), message.channel(), "Claimed",
                message.attempts() + 1, Optional.of(request.claimant()),
                Optional.of(request.now().plus(request.visibilityTimeout())), message.payloadJson()
            );
            tx.put(MESSAGES, updated.messageId(), QueueCodec.queueMessageToJson(updated));
            claimed.add(updated);
        }
        return claimed;
    }

    @Override
    public void acknowledge(
        Backend backend,
        AckMessageRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            acknowledgeTx(tx, request);
            backend.commit(tx);
        }
        catch (Backend.BackendException error)
        {
            tx.abort();
            throw error;
        }
    }

    @Override
    public void acknowledgeTx(
        Backend.Transaction tx,
        AckMessageRequest request
    ) throws Backend.BackendException
    {
        var message = requireMessage(tx, request.messageId());
        requireClaimant(message, request.claimant());
        var updated = new QueueMessageRecord(
            message.messageId(), message.queue(), message.channel(), "Acked", message.attempts(),
            message.claimedBy(), message.claimExpiresAt(), message.payloadJson()
        );
        tx.put(MESSAGES, updated.messageId(), QueueCodec.queueMessageToJson(updated));
    }

    @Override
    public QueueMessageRecord fail(
        Backend backend,
        FailMessageRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = failTx(tx, request);
            backend.commit(tx);
            return result;
        }
        catch (Backend.BackendException error)
        {
            tx.abort();
            throw error;
        }
    }

    @Override
    public QueueMessageRecord failTx(
        Backend.Transaction tx,
        FailMessageRequest request
    ) throws Backend.BackendException
    {
        var message = requireMessage(tx, request.messageId());
        requireClaimant(message, request.claimant());
        var status = message.attempts() >= request.maxAttempts() ? "DeadLettered" : "Pending";
        var updated = new QueueMessageRecord(
            message.messageId(), message.queue(), message.channel(), status, message.attempts(),
            Optional.empty(), Optional.empty(), message.payloadJson()
        );
        tx.put(MESSAGES, updated.messageId(), QueueCodec.queueMessageToJson(updated));
        return updated;
    }

    @Override
    public Optional<QueueMessageRecord> inspect(
        Backend backend,
        String messageId
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        var result = inspectTx(tx, messageId);
        backend.commit(tx);
        return result;
    }

    @Override
    public Optional<QueueMessageRecord> inspectTx(
        Backend.Transaction tx,
        String messageId
    ) throws Backend.BackendException
    {
        return tx.get(MESSAGES, messageId)
            .map(record -> QueueCodec.queueMessageFromJson(record.document()));
    }

    private QueueMessageRecord requireMessage(
        Backend.Transaction tx,
        String messageId
    ) throws Backend.BackendException
    {
        return tx.get(MESSAGES, messageId)
            .map(record -> QueueCodec.queueMessageFromJson(record.document()))
            .orElseThrow(() -> new Backend.BackendException("unknown queue message " + messageId));
    }

    private List<QueueMessageRecord> allMessages(Backend.Transaction tx)
        throws Backend.BackendException
    {
        var records = tx.query(MESSAGES, new Backend.Query.All());
        var messages = new ArrayList<QueueMessageRecord>();
        for (var record : records)
        {
            messages.add(QueueCodec.queueMessageFromJson(record.document()));
        }
        return messages;
    }

    private static String queueDefinitionKey(
        String queue,
        String channel
    )
    {
        return Codec.definitionKey(queue, channel);
    }

    private static void requireClaimant(
        QueueMessageRecord message,
        String claimant
    ) throws Backend.BackendException
    {
        if (message.claimedBy().isEmpty() || !message.claimedBy().get().equals(claimant))
        {
            throw new Backend.ConflictException(
                Backend.ConflictKind.QUEUE_CLAIM_CONFLICT, "queue message is not claimed by caller"
            );
        }
    }
}
