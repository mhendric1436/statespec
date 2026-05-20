package com.statespec.backend;

import com.statespec.backend.Backend.BackendException;
import com.statespec.backend.Backend.Transaction;
import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Optional;

public interface Queue
{
    record QueueDefinition(
        String queue,
        String channel,
        Duration visibilityTimeout,
        int maxAttempts,
        Optional<String> deadLetterQueue,
        String metadataJson
    )
    {
    }

    record RegisterQueueDefinitionRequest(QueueDefinition definition)
    {
    }

    record QueueDefinitionRegistration(
        QueueDefinition definition,
        boolean created
    )
    {
    }

    record QueueMessageRecord(
        String messageId,
        String queue,
        String channel,
        String status,
        long attempts,
        Optional<String> claimedBy,
        Optional<Instant> claimExpiresAt,
        String payloadJson
    )
    {
    }

    record EnqueueMessageRequest(
        String messageId,
        String queue,
        String channel,
        Optional<String> idempotencyKey,
        String payloadJson
    )
    {
    }

    record ClaimMessageRequest(
        String queue,
        String channel,
        String claimant,
        Instant now,
        Duration visibilityTimeout,
        int maxMessages
    )
    {
    }

    record AckMessageRequest(
        String messageId,
        String claimant
    )
    {
    }

    record FailMessageRequest(
        String messageId,
        String claimant,
        String reason,
        Instant now,
        int maxAttempts
    )
    {
    }

    QueueDefinitionRegistration registerDefinition(
        Backend backend,
        RegisterQueueDefinitionRequest request
    ) throws BackendException;

    QueueDefinitionRegistration registerDefinitionTx(
        Transaction tx,
        RegisterQueueDefinitionRequest request
    ) throws BackendException;

    Optional<QueueDefinition> inspectDefinition(
        Backend backend,
        String queue,
        String channel
    ) throws BackendException;

    Optional<QueueDefinition> inspectDefinitionTx(
        Transaction tx,
        String queue,
        String channel
    ) throws BackendException;

    QueueMessageRecord enqueue(
        Backend backend,
        EnqueueMessageRequest request
    ) throws BackendException;

    QueueMessageRecord enqueueTx(
        Transaction tx,
        EnqueueMessageRequest request
    ) throws BackendException;

    List<QueueMessageRecord> claim(
        Backend backend,
        ClaimMessageRequest request
    ) throws BackendException;

    List<QueueMessageRecord> claimTx(
        Transaction tx,
        ClaimMessageRequest request
    ) throws BackendException;

    void acknowledge(
        Backend backend,
        AckMessageRequest request
    ) throws BackendException;

    void acknowledgeTx(
        Transaction tx,
        AckMessageRequest request
    ) throws BackendException;

    QueueMessageRecord fail(
        Backend backend,
        FailMessageRequest request
    ) throws BackendException;

    QueueMessageRecord failTx(
        Transaction tx,
        FailMessageRequest request
    ) throws BackendException;

    Optional<QueueMessageRecord> inspect(
        Backend backend,
        String messageId
    ) throws BackendException;

    Optional<QueueMessageRecord> inspectTx(
        Transaction tx,
        String messageId
    ) throws BackendException;
}
