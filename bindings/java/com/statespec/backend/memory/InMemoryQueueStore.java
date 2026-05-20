package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.Queue;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Optional;

public final class InMemoryQueueStore implements Queue
{
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
        var memoryTx = InMemoryTransaction.require(tx);
        var existing =
            inspectDefinitionTx(tx, request.definition().queue(), request.definition().channel());
        memoryTx.queueDefinitionPuts.put(
            queueDefinitionKey(request.definition().queue(), request.definition().channel()),
            request.definition()
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
        var memoryTx = InMemoryTransaction.require(tx);
        var key = queueDefinitionKey(queue, channel);
        if (memoryTx.queueDefinitionPuts.containsKey(key))
        {
            return Optional.of(memoryTx.queueDefinitionPuts.get(key));
        }
        synchronized (memoryTx.state)
        {
            return Optional.ofNullable(memoryTx.state.queueDefinitions.get(key));
        }
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
        var memoryTx = InMemoryTransaction.require(tx);
        if (request.idempotencyKey().isPresent())
        {
            var idempotencyKey = queueDefinitionKey(request.queue(), request.channel()) + "|" +
                                 request.idempotencyKey().get();
            if (memoryTx.queueIdempotencyPuts.containsKey(idempotencyKey))
            {
                return requireMessage(memoryTx, memoryTx.queueIdempotencyPuts.get(idempotencyKey));
            }
            synchronized (memoryTx.state)
            {
                if (memoryTx.state.queueIdempotencyKeys.containsKey(idempotencyKey))
                {
                    return requireMessage(
                        memoryTx, memoryTx.state.queueIdempotencyKeys.get(idempotencyKey)
                    );
                }
            }
            memoryTx.queueIdempotencyPuts.put(idempotencyKey, request.messageId());
        }
        var record = new QueueMessageRecord(
            request.messageId(), request.queue(), request.channel(), "Pending", 0L,
            Optional.empty(), Optional.empty(), request.payloadJson()
        );
        memoryTx.queueMessagePuts.put(record.messageId(), record);
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
        var memoryTx = InMemoryTransaction.require(tx);
        var claimed = new ArrayList<QueueMessageRecord>();
        for (var message : allMessages(memoryTx))
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
            memoryTx.queueMessagePuts.put(updated.messageId(), updated);
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
        var memoryTx = InMemoryTransaction.require(tx);
        var message = requireMessage(memoryTx, request.messageId());
        requireClaimant(message, request.claimant());
        memoryTx.queueMessagePuts.put(
            message.messageId(), new QueueMessageRecord(
                                     message.messageId(), message.queue(), message.channel(),
                                     "Acked", message.attempts(), message.claimedBy(),
                                     message.claimExpiresAt(), message.payloadJson()
                                 )
        );
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
        var memoryTx = InMemoryTransaction.require(tx);
        var message = requireMessage(memoryTx, request.messageId());
        requireClaimant(message, request.claimant());
        var status = message.attempts() >= request.maxAttempts() ? "DeadLettered" : "Pending";
        var updated = new QueueMessageRecord(
            message.messageId(), message.queue(), message.channel(), status, message.attempts(),
            Optional.empty(), Optional.empty(), message.payloadJson()
        );
        memoryTx.queueMessagePuts.put(updated.messageId(), updated);
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
        var memoryTx = InMemoryTransaction.require(tx);
        if (memoryTx.queueMessagePuts.containsKey(messageId))
        {
            return Optional.of(memoryTx.queueMessagePuts.get(messageId));
        }
        synchronized (memoryTx.state)
        {
            return Optional.ofNullable(memoryTx.state.queueMessages.get(messageId));
        }
    }

    private QueueMessageRecord requireMessage(
        InMemoryTransaction tx,
        String messageId
    ) throws Backend.BackendException
    {
        if (tx.queueMessagePuts.containsKey(messageId))
        {
            return tx.queueMessagePuts.get(messageId);
        }
        synchronized (tx.state)
        {
            if (tx.state.queueMessages.containsKey(messageId))
            {
                return tx.state.queueMessages.get(messageId);
            }
        }
        throw new Backend.BackendException("unknown queue message " + messageId);
    }

    private List<QueueMessageRecord> allMessages(InMemoryTransaction tx)
    {
        var byId = new HashMap<String, QueueMessageRecord>();
        synchronized (tx.state)
        {
            byId.putAll(tx.state.queueMessages);
        }
        byId.putAll(tx.queueMessagePuts);
        return new ArrayList<>(byId.values());
    }

    private static String queueDefinitionKey(
        String queue,
        String channel
    )
    {
        return InMemoryTransaction.definitionKey(queue, channel);
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
