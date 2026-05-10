package com.statespec.backend;

import com.statespec.backend.BackendModel.BackendException;
import com.statespec.backend.BackendModel.Transaction;

import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Optional;

public interface Queue {
    record QueueMessageRecord(
        String messageId,
        String queue,
        String channel,
        String status,
        long attempts,
        Optional<String> claimedBy,
        Optional<Instant> claimExpiresAt,
        String payloadJson
    ) {}

    record EnqueueMessageRequest(
        String messageId,
        String queue,
        String channel,
        Optional<String> idempotencyKey,
        String payloadJson
    ) {}

    record ClaimMessageRequest(
        String queue,
        String channel,
        String claimant,
        Instant now,
        Duration visibilityTimeout,
        int maxMessages
    ) {}

    record AckMessageRequest(
        String messageId,
        String claimant
    ) {}

    record FailMessageRequest(
        String messageId,
        String claimant,
        String reason,
        Instant now,
        int maxAttempts
    ) {}

    QueueMessageRecord enqueue(
        Transaction tx,
        EnqueueMessageRequest request
    ) throws BackendException;

    List<QueueMessageRecord> claim(
        Transaction tx,
        ClaimMessageRequest request
    ) throws BackendException;

    void acknowledge(
        Transaction tx,
        AckMessageRequest request
    ) throws BackendException;

    QueueMessageRecord fail(
        Transaction tx,
        FailMessageRequest request
    ) throws BackendException;

    Optional<QueueMessageRecord> inspect(
        Transaction tx,
        String messageId
    ) throws BackendException;
}
