package com.statespec.backend.runtime;

import com.statespec.backend.Backend;
import com.statespec.backend.Workflow;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public final class WorkflowStore implements Workflow
{
    private static final String DEFINITIONS = "workflows.definitions";
    private static final String EXECUTIONS = "workflows.executions";

    @Override
    public WorkflowDefinitionRegistration registerDefinition(
        Backend backend,
        RegisterWorkflowDefinitionRequest request
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
    public WorkflowDefinitionRegistration registerDefinitionTx(
        Backend.Transaction tx,
        RegisterWorkflowDefinitionRequest request
    ) throws Backend.BackendException
    {
        var existing = inspectDefinitionTx(
            tx, request.definition().workflowName(), request.definition().workflowVersion()
        );
        tx.put(
            DEFINITIONS,
            workflowDefinitionKey(
                request.definition().workflowName(), request.definition().workflowVersion()
            ),
            WorkflowCodec.workflowDefinitionToJson(request.definition())
        );
        return new WorkflowDefinitionRegistration(request.definition(), existing.isEmpty());
    }

    @Override
    public Optional<WorkflowDefinition> inspectDefinition(
        Backend backend,
        String workflowName,
        long workflowVersion
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        var result = inspectDefinitionTx(tx, workflowName, workflowVersion);
        backend.commit(tx);
        return result;
    }

    @Override
    public Optional<WorkflowDefinition> inspectDefinitionTx(
        Backend.Transaction tx,
        String workflowName,
        long workflowVersion
    ) throws Backend.BackendException
    {
        var key = workflowDefinitionKey(workflowName, workflowVersion);
        return tx.get(DEFINITIONS, key)
            .map(record -> WorkflowCodec.workflowDefinitionFromJson(record.document()));
    }

    @Override
    public WorkflowExecutionRecord start(
        Backend backend,
        StartWorkflowRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = startTx(tx, request);
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
    public WorkflowExecutionRecord startTx(
        Backend.Transaction tx,
        StartWorkflowRequest request
    ) throws Backend.BackendException
    {
        var existing = inspectTx(tx, request.workflowExecutionId());
        if (existing.isPresent())
        {
            return existing.get();
        }
        var record = new WorkflowExecutionRecord(
            request.workflowExecutionId(), request.workflowName(), request.workflowVersion(),
            request.startStep(), "Running", 0L, Optional.empty(), Optional.empty(),
            request.stateJson()
        );
        tx.put(
            EXECUTIONS, record.workflowExecutionId(), WorkflowCodec.workflowExecutionToJson(record)
        );
        return record;
    }

    @Override
    public List<WorkflowExecutionRecord> claimSteps(
        Backend backend,
        ClaimWorkflowStepRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = claimStepsTx(tx, request);
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
    public List<WorkflowExecutionRecord> claimStepsTx(
        Backend.Transaction tx,
        ClaimWorkflowStepRequest request
    ) throws Backend.BackendException
    {
        var claimed = new ArrayList<WorkflowExecutionRecord>();
        for (var execution : allExecutions(tx))
        {
            if (claimed.size() >= request.maxSteps())
            {
                break;
            }
            if (!execution.workflowExecutionId().equals(request.workflowExecutionId()) ||
                !execution.workflowName().equals(request.workflowName()) ||
                execution.workflowVersion() != request.workflowVersion() ||
                !execution.status().equals("Running"))
            {
                continue;
            }
            var visible = execution.claimedBy().isEmpty() ||
                          (execution.claimExpiresAt().isPresent() &&
                           !execution.claimExpiresAt().get().isAfter(request.now()));
            if (!visible)
            {
                continue;
            }
            var updated = new WorkflowExecutionRecord(
                execution.workflowExecutionId(), execution.workflowName(),
                execution.workflowVersion(), execution.currentStep(), execution.status(),
                execution.attempt() + 1, Optional.of(request.worker()),
                Optional.of(request.now().plus(request.leaseDuration())), execution.stateJson()
            );
            tx.put(
                EXECUTIONS, updated.workflowExecutionId(),
                WorkflowCodec.workflowExecutionToJson(updated)
            );
            claimed.add(updated);
        }
        return claimed;
    }

    @Override
    public WorkflowExecutionRecord keepAliveStep(
        Backend backend,
        KeepAliveWorkflowStepRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = keepAliveStepTx(tx, request);
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
    public WorkflowExecutionRecord keepAliveStepTx(
        Backend.Transaction tx,
        KeepAliveWorkflowStepRequest request
    ) throws Backend.BackendException
    {
        var execution = requireExecution(tx, request.workflowExecutionId());
        requireClaim(execution, request.worker(), request.currentStep());
        var updated = new WorkflowExecutionRecord(
            execution.workflowExecutionId(), execution.workflowName(), execution.workflowVersion(),
            execution.currentStep(), execution.status(), execution.attempt(), execution.claimedBy(),
            Optional.of(request.now().plus(request.leaseDuration())), execution.stateJson()
        );
        tx.put(
            EXECUTIONS, updated.workflowExecutionId(),
            WorkflowCodec.workflowExecutionToJson(updated)
        );
        return updated;
    }

    @Override
    public WorkflowExecutionRecord completeStep(
        Backend backend,
        CompleteWorkflowStepRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = completeStepTx(tx, request);
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
    public WorkflowExecutionRecord completeStepTx(
        Backend.Transaction tx,
        CompleteWorkflowStepRequest request
    ) throws Backend.BackendException
    {
        var execution = requireExecution(tx, request.workflowExecutionId());
        requireClaim(execution, request.worker(), request.completedStep());
        var status = request.nextStep().isPresent() ? "Running" : "Completed";
        var step = request.nextStep().orElse(execution.currentStep());
        var updated = new WorkflowExecutionRecord(
            execution.workflowExecutionId(), execution.workflowName(), execution.workflowVersion(),
            step, status, execution.attempt(), Optional.empty(), Optional.empty(),
            request.stateJson()
        );
        tx.put(
            EXECUTIONS, updated.workflowExecutionId(),
            WorkflowCodec.workflowExecutionToJson(updated)
        );
        return updated;
    }

    @Override
    public WorkflowExecutionRecord failStep(
        Backend backend,
        FailWorkflowStepRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = failStepTx(tx, request);
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
    public WorkflowExecutionRecord failStepTx(
        Backend.Transaction tx,
        FailWorkflowStepRequest request
    ) throws Backend.BackendException
    {
        var execution = requireExecution(tx, request.workflowExecutionId());
        requireClaim(execution, request.worker(), request.failedStep());
        var status = execution.attempt() >= request.maxAttempts() ? "Failed" : "Running";
        var updated = new WorkflowExecutionRecord(
            execution.workflowExecutionId(), execution.workflowName(), execution.workflowVersion(),
            execution.currentStep(), status, execution.attempt(), Optional.empty(),
            Optional.empty(), execution.stateJson()
        );
        tx.put(
            EXECUTIONS, updated.workflowExecutionId(),
            WorkflowCodec.workflowExecutionToJson(updated)
        );
        return updated;
    }

    @Override
    public WorkflowExecutionRecord cancel(
        Backend backend,
        CancelWorkflowRequest request
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        try
        {
            var result = cancelTx(tx, request);
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
    public WorkflowExecutionRecord cancelTx(
        Backend.Transaction tx,
        CancelWorkflowRequest request
    ) throws Backend.BackendException
    {
        var execution = requireExecution(tx, request.workflowExecutionId());
        var updated = new WorkflowExecutionRecord(
            execution.workflowExecutionId(), execution.workflowName(), execution.workflowVersion(),
            execution.currentStep(), "Canceled", execution.attempt(), Optional.empty(),
            Optional.empty(), execution.stateJson()
        );
        tx.put(
            EXECUTIONS, updated.workflowExecutionId(),
            WorkflowCodec.workflowExecutionToJson(updated)
        );
        return updated;
    }

    @Override
    public Optional<WorkflowExecutionRecord> inspect(
        Backend backend,
        String workflowExecutionId
    ) throws Backend.BackendException
    {
        var tx = backend.begin();
        var result = inspectTx(tx, workflowExecutionId);
        backend.commit(tx);
        return result;
    }

    @Override
    public Optional<WorkflowExecutionRecord> inspectTx(
        Backend.Transaction tx,
        String workflowExecutionId
    ) throws Backend.BackendException
    {
        return tx.get(EXECUTIONS, workflowExecutionId)
            .map(record -> WorkflowCodec.workflowExecutionFromJson(record.document()));
    }

    private WorkflowExecutionRecord requireExecution(
        Backend.Transaction tx,
        String workflowExecutionId
    ) throws Backend.BackendException
    {
        return inspectTx(tx, workflowExecutionId)
            .orElseThrow(() -> new Backend.BackendException("unknown workflow execution"));
    }

    private List<WorkflowExecutionRecord> allExecutions(Backend.Transaction tx)
        throws Backend.BackendException
    {
        var records = tx.query(EXECUTIONS, new Backend.Query.All());
        var executions = new ArrayList<WorkflowExecutionRecord>();
        for (var record : records)
        {
            executions.add(WorkflowCodec.workflowExecutionFromJson(record.document()));
        }
        return executions;
    }

    private static String workflowDefinitionKey(
        String name,
        long version
    )
    {
        return Codec.definitionKey(name, version);
    }

    private static void requireClaim(
        WorkflowExecutionRecord execution,
        String worker,
        String step
    ) throws Backend.BackendException
    {
        if (execution.claimedBy().isEmpty() || !execution.claimedBy().get().equals(worker) ||
            !execution.currentStep().equals(step))
        {
            throw new Backend.ConflictException(
                Backend.ConflictKind.WORKFLOW_CLAIM_CONFLICT,
                "workflow step is not claimed by caller"
            );
        }
    }
}
