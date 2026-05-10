package com.statespec.backend;

import com.statespec.backend.BackendModel.BackendException;
import com.statespec.backend.BackendModel.JsonEquals;
import com.statespec.backend.BackendModel.LeaseRecord;
import com.statespec.backend.BackendModel.QueueMessageRecord;
import com.statespec.backend.BackendModel.Transaction;
import com.statespec.backend.BackendModel.WorkflowExecutionRecord;

import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Optional;

public final class RuntimeStores {
    private RuntimeStores() {}

    public record LeaseAcquireRequest(
        String resource,
        String holder,
        Instant now,
        Duration ttl
    ) {}

    public record LeaseRenewRequest(
        String resource,
        String holder,
        long fencingToken,
        Instant now,
        Duration ttl
    ) {}

    public record LeaseReleaseRequest(
        String resource,
        String holder,
        long fencingToken
    ) {}

    public record LeaseAcquireResult(
        boolean acquired,
        Optional<LeaseRecord> lease
    ) {}

    public interface LeaseStore {
        LeaseAcquireResult acquire(
            Transaction tx,
            LeaseAcquireRequest request
        ) throws BackendException;

        LeaseRecord renew(
            Transaction tx,
            LeaseRenewRequest request
        ) throws BackendException;

        void release(
            Transaction tx,
            LeaseReleaseRequest request
        ) throws BackendException;

        Optional<LeaseRecord> inspect(
            Transaction tx,
            String resource
        ) throws BackendException;
    }

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

    public interface QueueStore {
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

    public record StartWorkflowRequest(
        String workflowExecutionId,
        String workflowName,
        String startStep,
        String stateJson
    ) {}

    public record ClaimWorkflowStepRequest(
        String workflowName,
        String worker,
        Instant now,
        Duration leaseDuration,
        int maxSteps
    ) {}

    public record CompleteWorkflowStepRequest(
        String workflowExecutionId,
        String worker,
        String completedStep,
        Optional<String> nextStep,
        String stateJson
    ) {}

    public record FailWorkflowStepRequest(
        String workflowExecutionId,
        String worker,
        String failedStep,
        String reason,
        Instant now,
        int maxAttempts
    ) {}

    public record CancelWorkflowRequest(
        String workflowExecutionId,
        String reason
    ) {}

    public interface WorkflowStore {
        WorkflowExecutionRecord start(
            Transaction tx,
            StartWorkflowRequest request
        ) throws BackendException;

        List<WorkflowExecutionRecord> claimSteps(
            Transaction tx,
            ClaimWorkflowStepRequest request
        ) throws BackendException;

        WorkflowExecutionRecord completeStep(
            Transaction tx,
            CompleteWorkflowStepRequest request
        ) throws BackendException;

        WorkflowExecutionRecord failStep(
            Transaction tx,
            FailWorkflowStepRequest request
        ) throws BackendException;

        WorkflowExecutionRecord cancel(
            Transaction tx,
            CancelWorkflowRequest request
        ) throws BackendException;

        Optional<WorkflowExecutionRecord> inspect(
            Transaction tx,
            String workflowExecutionId
        ) throws BackendException;
    }
}
