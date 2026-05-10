package com.statespec.backend;

import com.statespec.backend.BackendModel.BackendException;
import com.statespec.backend.BackendModel.Transaction;

import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Optional;

public interface WorkflowStore {
    record WorkflowExecutionRecord(
        String workflowExecutionId,
        String workflowName,
        long workflowVersion,
        String currentStep,
        String status,
        long attempt,
        Optional<String> claimedBy,
        Optional<Instant> claimExpiresAt,
        String stateJson
    ) {}

    record StartWorkflowRequest(
        String workflowExecutionId,
        String workflowName,
        long workflowVersion,
        String startStep,
        String stateJson
    ) {}

    record ClaimWorkflowStepRequest(
        String workflowName,
        long workflowVersion,
        String worker,
        Instant now,
        Duration leaseDuration,
        int maxSteps
    ) {}

    record CompleteWorkflowStepRequest(
        String workflowExecutionId,
        String worker,
        String completedStep,
        Optional<String> nextStep,
        String stateJson
    ) {}

    record FailWorkflowStepRequest(
        String workflowExecutionId,
        String worker,
        String failedStep,
        String reason,
        Instant now,
        int maxAttempts
    ) {}

    record CancelWorkflowRequest(
        String workflowExecutionId,
        String reason
    ) {}

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
