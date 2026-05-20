package com.statespec.backend.memory;

import com.statespec.backend.Backend;
import com.statespec.backend.Workflow;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Optional;

public final class InMemoryWorkflowStore implements Workflow
{
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
        var memoryTx = InMemoryTransaction.require(tx);
        var existing = inspectDefinitionTx(
            tx, request.definition().workflowName(), request.definition().workflowVersion()
        );
        memoryTx.workflowDefinitionPuts.put(
            workflowDefinitionKey(
                request.definition().workflowName(), request.definition().workflowVersion()
            ),
            request.definition()
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
        var memoryTx = InMemoryTransaction.require(tx);
        var key = workflowDefinitionKey(workflowName, workflowVersion);
        if (memoryTx.workflowDefinitionPuts.containsKey(key))
        {
            return Optional.of(memoryTx.workflowDefinitionPuts.get(key));
        }
        synchronized (memoryTx.state)
        {
            return Optional.ofNullable(memoryTx.state.workflowDefinitions.get(key));
        }
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
        var memoryTx = InMemoryTransaction.require(tx);
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
        memoryTx.workflowExecutionPuts.put(record.workflowExecutionId(), record);
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
        var memoryTx = InMemoryTransaction.require(tx);
        var claimed = new ArrayList<WorkflowExecutionRecord>();
        for (var execution : allExecutions(memoryTx))
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
            memoryTx.workflowExecutionPuts.put(updated.workflowExecutionId(), updated);
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
        var memoryTx = InMemoryTransaction.require(tx);
        var execution = requireExecution(tx, request.workflowExecutionId());
        requireClaim(execution, request.worker(), request.currentStep());
        var updated = new WorkflowExecutionRecord(
            execution.workflowExecutionId(), execution.workflowName(), execution.workflowVersion(),
            execution.currentStep(), execution.status(), execution.attempt(), execution.claimedBy(),
            Optional.of(request.now().plus(request.leaseDuration())), execution.stateJson()
        );
        memoryTx.workflowExecutionPuts.put(updated.workflowExecutionId(), updated);
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
        var memoryTx = InMemoryTransaction.require(tx);
        var execution = requireExecution(tx, request.workflowExecutionId());
        requireClaim(execution, request.worker(), request.completedStep());
        var status = request.nextStep().isPresent() ? "Running" : "Completed";
        var step = request.nextStep().orElse(execution.currentStep());
        var updated = new WorkflowExecutionRecord(
            execution.workflowExecutionId(), execution.workflowName(), execution.workflowVersion(),
            step, status, execution.attempt(), Optional.empty(), Optional.empty(),
            request.stateJson()
        );
        memoryTx.workflowExecutionPuts.put(updated.workflowExecutionId(), updated);
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
        var memoryTx = InMemoryTransaction.require(tx);
        var execution = requireExecution(tx, request.workflowExecutionId());
        requireClaim(execution, request.worker(), request.failedStep());
        var status = execution.attempt() >= request.maxAttempts() ? "Failed" : "Running";
        var updated = new WorkflowExecutionRecord(
            execution.workflowExecutionId(), execution.workflowName(), execution.workflowVersion(),
            execution.currentStep(), status, execution.attempt(), Optional.empty(),
            Optional.empty(), execution.stateJson()
        );
        memoryTx.workflowExecutionPuts.put(updated.workflowExecutionId(), updated);
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
        var memoryTx = InMemoryTransaction.require(tx);
        var execution = requireExecution(tx, request.workflowExecutionId());
        var updated = new WorkflowExecutionRecord(
            execution.workflowExecutionId(), execution.workflowName(), execution.workflowVersion(),
            execution.currentStep(), "Canceled", execution.attempt(), Optional.empty(),
            Optional.empty(), execution.stateJson()
        );
        memoryTx.workflowExecutionPuts.put(updated.workflowExecutionId(), updated);
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
        var memoryTx = InMemoryTransaction.require(tx);
        if (memoryTx.workflowExecutionPuts.containsKey(workflowExecutionId))
        {
            return Optional.of(memoryTx.workflowExecutionPuts.get(workflowExecutionId));
        }
        synchronized (memoryTx.state)
        {
            return Optional.ofNullable(memoryTx.state.workflowExecutions.get(workflowExecutionId));
        }
    }

    private WorkflowExecutionRecord requireExecution(
        Backend.Transaction tx,
        String workflowExecutionId
    ) throws Backend.BackendException
    {
        return inspectTx(tx, workflowExecutionId)
            .orElseThrow(() -> new Backend.BackendException("unknown workflow execution"));
    }

    private List<WorkflowExecutionRecord> allExecutions(InMemoryTransaction tx)
    {
        var byId = new HashMap<String, WorkflowExecutionRecord>();
        synchronized (tx.state)
        {
            byId.putAll(tx.state.workflowExecutions);
        }
        byId.putAll(tx.workflowExecutionPuts);
        return new ArrayList<>(byId.values());
    }

    private static String workflowDefinitionKey(
        String name,
        long version
    )
    {
        return InMemoryTransaction.definitionKey(name, version);
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
