package com.statespec.generated;

import com.statespec.backend.Backend;
import com.statespec.backend.Workflow;
import com.statespec.backend.memory.InMemoryBackend;
import com.statespec.backend.runtime.WorkflowStore;
import java.time.Duration;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.atomic.AtomicInteger;

public final class WorkerLongRunningFixture
{
    private WorkerLongRunningFixture() {}

    private static final class CountingWorkflowStore implements Workflow
    {
        private final WorkflowStore delegate = new WorkflowStore();
        private final AtomicInteger keepAliveCount = new AtomicInteger();

        int keepAliveCount()
        {
            return keepAliveCount.get();
        }

        @Override
        public WorkflowDefinitionRegistration registerDefinition(
            Backend backend,
            RegisterWorkflowDefinitionRequest request
        ) throws Backend.BackendException
        {
            return delegate.registerDefinition(backend, request);
        }

        @Override
        public WorkflowDefinitionRegistration registerDefinitionTx(
            Backend.Transaction tx,
            RegisterWorkflowDefinitionRequest request
        ) throws Backend.BackendException
        {
            return delegate.registerDefinitionTx(tx, request);
        }

        @Override
        public Optional<WorkflowDefinition> inspectDefinition(
            Backend backend,
            String workflowName,
            long workflowVersion
        ) throws Backend.BackendException
        {
            return delegate.inspectDefinition(backend, workflowName, workflowVersion);
        }

        @Override
        public Optional<WorkflowDefinition> inspectDefinitionTx(
            Backend.Transaction tx,
            String workflowName,
            long workflowVersion
        ) throws Backend.BackendException
        {
            return delegate.inspectDefinitionTx(tx, workflowName, workflowVersion);
        }

        @Override
        public WorkflowExecutionRecord start(
            Backend backend,
            StartWorkflowRequest request
        ) throws Backend.BackendException
        {
            return delegate.start(backend, request);
        }

        @Override
        public WorkflowExecutionRecord startTx(
            Backend.Transaction tx,
            StartWorkflowRequest request
        ) throws Backend.BackendException
        {
            return delegate.startTx(tx, request);
        }

        @Override
        public List<WorkflowExecutionRecord> claimSteps(
            Backend backend,
            ClaimWorkflowStepRequest request
        ) throws Backend.BackendException
        {
            return delegate.claimSteps(backend, request);
        }

        @Override
        public List<WorkflowExecutionRecord> claimStepsTx(
            Backend.Transaction tx,
            ClaimWorkflowStepRequest request
        ) throws Backend.BackendException
        {
            return delegate.claimStepsTx(tx, request);
        }

        @Override
        public WorkflowExecutionRecord keepAliveStep(
            Backend backend,
            KeepAliveWorkflowStepRequest request
        ) throws Backend.BackendException
        {
            keepAliveCount.incrementAndGet();
            return delegate.keepAliveStep(backend, request);
        }

        @Override
        public WorkflowExecutionRecord keepAliveStepTx(
            Backend.Transaction tx,
            KeepAliveWorkflowStepRequest request
        ) throws Backend.BackendException
        {
            return delegate.keepAliveStepTx(tx, request);
        }

        @Override
        public WorkflowExecutionRecord completeStep(
            Backend backend,
            CompleteWorkflowStepRequest request
        ) throws Backend.BackendException
        {
            return delegate.completeStep(backend, request);
        }

        @Override
        public WorkflowExecutionRecord completeStepTx(
            Backend.Transaction tx,
            CompleteWorkflowStepRequest request
        ) throws Backend.BackendException
        {
            return delegate.completeStepTx(tx, request);
        }

        @Override
        public WorkflowExecutionRecord failStep(
            Backend backend,
            FailWorkflowStepRequest request
        ) throws Backend.BackendException
        {
            return delegate.failStep(backend, request);
        }

        @Override
        public WorkflowExecutionRecord failStepTx(
            Backend.Transaction tx,
            FailWorkflowStepRequest request
        ) throws Backend.BackendException
        {
            return delegate.failStepTx(tx, request);
        }

        @Override
        public WorkflowExecutionRecord cancel(
            Backend backend,
            CancelWorkflowRequest request
        ) throws Backend.BackendException
        {
            return delegate.cancel(backend, request);
        }

        @Override
        public WorkflowExecutionRecord cancelTx(
            Backend.Transaction tx,
            CancelWorkflowRequest request
        ) throws Backend.BackendException
        {
            return delegate.cancelTx(tx, request);
        }

        @Override
        public Optional<WorkflowExecutionRecord> inspect(
            Backend backend,
            String workflowExecutionId
        ) throws Backend.BackendException
        {
            return delegate.inspect(backend, workflowExecutionId);
        }

        @Override
        public Optional<WorkflowExecutionRecord> inspectTx(
            Backend.Transaction tx,
            String workflowExecutionId
        ) throws Backend.BackendException
        {
            return delegate.inspectTx(tx, workflowExecutionId);
        }
    }

    private static final class LongRunningStepHandler
        implements com.statespec.generated.workflows.provision_service.Handlers
                       .ProvisionServiceV1StepHandler
    {
        private boolean handled;

        @Override
        public WorkflowStepHandlers.WorkflowStepResult handleValidateRequest(
            Backend.Transaction tx,
            WorkflowStepHandlers.Context context
        ) throws Exception
        {
            if (!tx.isOpen())
            {
                throw new IllegalStateException("workflow handler received a closed transaction");
            }
            if (!context.workflowName().equals("ProvisionService") ||
                !context.stepName().equals("validate_request"))
            {
                throw new IllegalStateException("unexpected workflow step");
            }
            Thread.sleep(850);
            handled = true;
            return WorkflowStepHandlers.complete();
        }

        @Override
        public WorkflowStepHandlers.WorkflowStepResult handleCreateRemoteService(
            Backend.Transaction tx,
            WorkflowStepHandlers.Context context
        )
        {
            if (!tx.isOpen())
            {
                throw new IllegalStateException("workflow handler received a closed transaction");
            }
            return WorkflowStepHandlers.fail("unexpected create_remote_service step");
        }

        @Override
        public WorkflowStepHandlers.WorkflowStepResult handleWaitForRemoteService(
            Backend.Transaction tx,
            WorkflowStepHandlers.Context context
        )
        {
            if (!tx.isOpen())
            {
                throw new IllegalStateException("workflow handler received a closed transaction");
            }
            return WorkflowStepHandlers.fail("unexpected wait_for_remote_service step");
        }
    }

    public static void main(String[] args) throws Exception
    {
        InMemoryBackend backend = new InMemoryBackend();
        CountingWorkflowStore workflows = new CountingWorkflowStore();

        workflows.registerDefinition(
            backend, new Workflow.RegisterWorkflowDefinitionRequest(new Workflow.WorkflowDefinition(
                         "ProvisionService", 1L, "validate_request", Duration.ofMinutes(1), false,
                         List.of(
                             new Workflow.WorkflowStepDefinition(
                                 "validate_request", Duration.ofSeconds(1), 3
                             ),
                             new Workflow.WorkflowStepDefinition(
                                 "create_remote_service", Duration.ofSeconds(1), 3
                             ),
                             new Workflow.WorkflowStepDefinition(
                                 "wait_for_remote_service", Duration.ofSeconds(1), 3
                             )
                         ),
                         "{}"
                     ))
        );
        workflows.start(
            backend, new Workflow.StartWorkflowRequest(
                         "wf-long", "ProvisionService", 1L, "validate_request", "{}"
                     )
        );

        LongRunningStepHandler handler = new LongRunningStepHandler();
        var invokers =
            new java.util.LinkedHashMap<String, WorkflowStepHandlers.WorkflowStepInvoker>();
        WorkerRegistry.registerProvisionServiceWorkflowStepInvokers(invokers, handler);
        WorkflowRunner runner = new WorkflowRunner(
            backend, workflows, invokers, "ProvisionWorker", Duration.ofSeconds(1), 3
        );
        Optional<Workflow.WorkflowExecutionRecord> completed =
            runner.runOnce("wf-long", "ProvisionService", 1L);
        if (!handler.handled || completed.isEmpty() ||
            !completed.get().status().equals("Completed"))
        {
            throw new IllegalStateException("long-running workflow step did not complete");
        }
        if (workflows.keepAliveCount() < 2)
        {
            throw new IllegalStateException(
                "long-running workflow step did not receive periodic keepalive"
            );
        }
    }
}
