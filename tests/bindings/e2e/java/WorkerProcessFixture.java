package com.statespec.generated;

import com.statespec.backend.Workflow;
import com.statespec.backend.memory.InMemoryBackend;
import com.statespec.backend.runtime.WorkflowStore;
import java.util.concurrent.atomic.AtomicBoolean;

public final class WorkerProcessFixture
{
    private WorkerProcessFixture() {}

    private static final class ProcessStepHandler
        implements com.statespec.generated.workflows.provision_service.Handlers
                       .ProvisionServiceV1StepHandler
    {
        private final AtomicBoolean handledValidateRequest = new AtomicBoolean(false);

        @Override
        public WorkflowStepHandlers.WorkflowStepResult
        handleValidateRequest(WorkflowStepHandlers.Context context)
        {
            if (!context.workflowName().equals("ProvisionService") ||
                !context.stepName().equals("validate_request"))
            {
                throw new IllegalStateException("unexpected workflow step");
            }
            handledValidateRequest.set(true);
            return WorkflowStepHandlers.complete("create_remote_service");
        }

        @Override
        public WorkflowStepHandlers.WorkflowStepResult
        handleCreateRemoteService(WorkflowStepHandlers.Context context)
        {
            return WorkflowStepHandlers.complete("wait_for_remote_service");
        }

        @Override
        public WorkflowStepHandlers.WorkflowStepResult
        handleWaitForRemoteService(WorkflowStepHandlers.Context context)
        {
            return WorkflowStepHandlers.complete();
        }
    }

    public static void main(String[] args) throws Exception
    {
        InMemoryBackend backend = new InMemoryBackend();
        WorkerRuntime runtime = new WorkerRuntime(backend);
        ProcessStepHandler handler = new ProcessStepHandler();
        var invokers =
            new java.util.LinkedHashMap<String, WorkflowStepHandlers.WorkflowStepInvoker>();
        WorkerRegistry.registerProvisionServiceWorkflowStepInvokers(invokers, handler);
        WorkerProcess.Config config = new WorkerProcess.Config(true, 1);
        WorkerProcess process = new WorkerProcess(runtime, invokers, config);

        try
        {
            process.join();
            throw new IllegalStateException("joining a WorkerProcess before start should fail");
        }
        catch (IllegalStateException expected)
        {
        }
        process.requestStop();

        runtime.bootstrap();
        WorkflowStore workflows = new WorkflowStore();
        workflows.start(
            backend, new Workflow.StartWorkflowRequest(
                         "wf-process-1", "ProvisionService", 1L, "validate_request", "{}"
                     )
        );

        process.start();
        try
        {
            process.start();
            throw new IllegalStateException("starting WorkerProcess twice should fail");
        }
        catch (IllegalStateException expected)
        {
        }
        Thread.sleep(10);
        if (!process.isRunning())
        {
            throw new IllegalStateException("started WorkerProcess should report running");
        }

        long deadline = System.currentTimeMillis() + 2000;
        while (!handler.handledValidateRequest.get() && System.currentTimeMillis() < deadline)
        {
            Thread.sleep(10);
        }
        if (!handler.handledValidateRequest.get())
        {
            throw new IllegalStateException("WorkerProcess did not execute a workflow step");
        }

        process.requestStop();
        if (process.join() != 0)
        {
            throw new IllegalStateException("stopped WorkerProcess should join cleanly");
        }
        if (process.isRunning())
        {
            throw new IllegalStateException("joined WorkerProcess should not report running");
        }
    }
}
