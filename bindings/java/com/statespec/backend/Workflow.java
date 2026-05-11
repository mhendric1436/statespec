package com.statespec.backend;

import com.statespec.backend.BackendModel.Backend;
import com.statespec.backend.BackendModel.BackendException;
import com.statespec.backend.BackendModel.Transaction;

import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Optional;

public abstract class Workflow {
    private final Backend backend;

    protected Workflow(Backend backend) {
        this.backend = backend;
    }

    public record WorkflowStepDefinition(
        String name,
        Duration expectedExecutionTime,
        int maxRetries
    ) {}

    public record WorkflowDefinition(
        String workflowName,
        long workflowVersion,
        String startStep,
        Duration expectedExecutionTime,
        boolean singleton,
        List<WorkflowStepDefinition> steps,
        String metadataJson
    ) {}

    public record RegisterWorkflowDefinitionRequest(
        WorkflowDefinition definition
    ) {}

    public record WorkflowDefinitionRegistration(
        WorkflowDefinition definition,
        boolean created
    ) {}

    public record WorkflowExecutionRecord(
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

    public record StartWorkflowRequest(
        String workflowExecutionId,
        String workflowName,
        long workflowVersion,
        String startStep,
        String stateJson
    ) {}

    public record ClaimWorkflowStepRequest(
        String workflowExecutionId,
        String workflowName,
        long workflowVersion,
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

    protected abstract WorkflowDefinitionRegistration registerDefinition(
        Transaction tx,
        RegisterWorkflowDefinitionRequest request
    ) throws BackendException;

    protected abstract Optional<WorkflowDefinition> inspectDefinition(
        Transaction tx,
        String workflowName,
        long workflowVersion
    ) throws BackendException;

    protected abstract WorkflowExecutionRecord start(
        Transaction tx,
        StartWorkflowRequest request
    ) throws BackendException;

    protected abstract List<WorkflowExecutionRecord> claimSteps(
        Transaction tx,
        ClaimWorkflowStepRequest request
    ) throws BackendException;

    protected abstract WorkflowExecutionRecord completeStep(
        Transaction tx,
        CompleteWorkflowStepRequest request
    ) throws BackendException;

    protected abstract WorkflowExecutionRecord failStep(
        Transaction tx,
        FailWorkflowStepRequest request
    ) throws BackendException;

    protected abstract WorkflowExecutionRecord cancel(
        Transaction tx,
        CancelWorkflowRequest request
    ) throws BackendException;

    protected abstract Optional<WorkflowExecutionRecord> inspect(
        Transaction tx,
        String workflowExecutionId
    ) throws BackendException;

    public final WorkflowDefinitionRegistration registerDefinition(
        RegisterWorkflowDefinitionRequest request
    ) throws BackendException {
        Transaction tx = backend.begin();
        try {
            WorkflowDefinitionRegistration result = registerDefinition(tx, request);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final Optional<WorkflowDefinition> inspectDefinition(
        String workflowName,
        long workflowVersion
    ) throws BackendException {
        Transaction tx = backend.begin();
        try {
            Optional<WorkflowDefinition> result = inspectDefinition(tx, workflowName, workflowVersion);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final WorkflowExecutionRecord start(StartWorkflowRequest request) throws BackendException {
        Transaction tx = backend.begin();
        try {
            WorkflowExecutionRecord result = start(tx, request);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final List<WorkflowExecutionRecord> claimSteps(
        ClaimWorkflowStepRequest request
    ) throws BackendException {
        Transaction tx = backend.begin();
        try {
            List<WorkflowExecutionRecord> result = claimSteps(tx, request);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final WorkflowExecutionRecord completeStep(
        CompleteWorkflowStepRequest request
    ) throws BackendException {
        Transaction tx = backend.begin();
        try {
            WorkflowExecutionRecord result = completeStep(tx, request);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final WorkflowExecutionRecord failStep(FailWorkflowStepRequest request) throws BackendException {
        Transaction tx = backend.begin();
        try {
            WorkflowExecutionRecord result = failStep(tx, request);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final WorkflowExecutionRecord cancel(CancelWorkflowRequest request) throws BackendException {
        Transaction tx = backend.begin();
        try {
            WorkflowExecutionRecord result = cancel(tx, request);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }

    public final Optional<WorkflowExecutionRecord> inspect(String workflowExecutionId) throws BackendException {
        Transaction tx = backend.begin();
        try {
            Optional<WorkflowExecutionRecord> result = inspect(tx, workflowExecutionId);
            backend.commit(tx);
            return result;
        } catch (BackendException | RuntimeException e) {
            tx.abort();
            throw e;
        }
    }
}
