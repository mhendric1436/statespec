package com.statespec.backend;

import com.statespec.backend.BackendModel.Backend;
import com.statespec.backend.BackendModel.BackendException;
import com.statespec.backend.BackendModel.Transaction;

import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Optional;

public abstract class Queue {
    private final Backend backend;

    protected Queue(Backend backend) {
        this.backend = backend;
    }

    public record QueueDefinition(
        String queue,
        String channel,
        Duration visibilityTimeout,
        int maxAttempts,
        Optional<String> deadLetterQueue,
        String metadataJson
    ) {}

    public record CreateQueueRequest(
        QueueDefinition definition
    ) {}

    public record QueueCreation(
        QueueDefinition definition,
        boolean created
    ) {}

    public record QueueMessageRecord(
        String messageId,
        String queue,
        String channel,
        String status,
        long attempts,
        Optional<String> claimedBy,
        Optional<Instant> claimExpiresAt,
        String payloadJson
    ) {}

    public record EnqueueMessageRequest(
        String messageId,
        String queue,
        String channel,
        Optional<String> idempotencyKey,
        String payloadJson
    ) {}

    public record ClaimMessageRequest(
        String queue,
        String channel,
        String claimant,
        Instant now,
        Duration visibilityTimeout,
        int maxMessages
    ) {}

    public record AckMessageRequest(
        String messageId,
        String claimant
    ) {}

    public record FailMessageRequest(
        String messageId,
        String claimant,
        String reason,
        Instant now,
        int maxAttempts
    ) {}

    protected abstract QueueCreation create(
        Transaction tx,
        CreateQueueRequest request
    ) throws BackendException;

    protected abstract Optional<QueueDefinition> inspectDefinition(
        Transaction tx,
        String queue,
        String channel
    ) throws BackendException;

    protected abstract QueueMessageRecord enqueue(
        Transaction tx,
        EnqueueMessageRequest request
    ) throws BackendException;

    protected abstract List<QueueMessageRecord> claim(
        Transaction tx,
        ClaimMessageRequest request
    ) throws BackendException;

    protected abstract void acknowledge(
        Transaction tx,
        AckMessageRequest request
    ) throws BackendException;

    protected abstract QueueMessageRecord fail(
        Transaction tx,
        FailMessageRequest request
    ) throws BackendException;

    protected abstract Optional<QueueMessageRecord> inspect(
        Transaction tx,
        String messageId
    ) throws BackendException;

    public final QueueCreation create(CreateQueueRequest request) throws BackendException {
        Transaction tx = backend.begin();
        try {
            QueueCreation result = create(tx, request);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final Optional<QueueDefinition> inspectDefinition(
        String queue,
        String channel
    ) throws BackendException {
        Transaction tx = backend.begin();
        try {
            Optional<QueueDefinition> result = inspectDefinition(tx, queue, channel);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final QueueMessageRecord enqueue(EnqueueMessageRequest request) throws BackendException {
        Transaction tx = backend.begin();
        try {
            QueueMessageRecord result = enqueue(tx, request);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final List<QueueMessageRecord> claim(ClaimMessageRequest request) throws BackendException {
        Transaction tx = backend.begin();
        try {
            List<QueueMessageRecord> result = claim(tx, request);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final void acknowledge(AckMessageRequest request) throws BackendException {
        Transaction tx = backend.begin();
        try {
            acknowledge(tx, request);
            backend.commit(tx);
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final QueueMessageRecord fail(FailMessageRequest request) throws BackendException {
        Transaction tx = backend.begin();
        try {
            QueueMessageRecord result = fail(tx, request);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final Optional<QueueMessageRecord> inspect(String messageId) throws BackendException {
        Transaction tx = backend.begin();
        try {
            Optional<QueueMessageRecord> result = inspect(tx, messageId);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }
}
