package com.statespec.generated;

import com.statespec.backend.Json;
import com.statespec.backend.Workflow;
import com.statespec.backend.memory.InMemoryBackend;
import com.statespec.backend.runtime.WorkflowStore;
import java.time.Duration;
import java.util.List;
import java.util.Optional;

public final class WorkerLinkingFixture
{
    private WorkerLinkingFixture() {}

    private static final class LinkingStepHandler
        implements com.statespec.generated.workflows.provision_service.Handlers
                       .ProvisionServiceV1StepHandler
    {
        private boolean handled;

        @Override
        public WorkflowStepHandlers.WorkflowStepResult
        handleValidateRequest(WorkflowStepHandlers.Context context)
        {
            if (!context.workflowName().equals("ProvisionService") ||
                !context.stepName().equals("validate_request"))
            {
                throw new IllegalStateException("unexpected workflow step");
            }
            handled = true;
            return WorkflowStepHandlers.complete("create_remote_service");
        }

        @Override
        public WorkflowStepHandlers.WorkflowStepResult
        handleCreateRemoteService(WorkflowStepHandlers.Context context)
        {
            return WorkflowStepHandlers.fail("unexpected create_remote_service step");
        }

        @Override
        public WorkflowStepHandlers.WorkflowStepResult
        handleWaitForRemoteService(WorkflowStepHandlers.Context context)
        {
            return WorkflowStepHandlers.fail("unexpected wait_for_remote_service step");
        }
    }

    public static void main(String[] args) throws Exception
    {
        InMemoryBackend backend = new InMemoryBackend();
        WorkflowStore workflows = new WorkflowStore();

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
                         "wf-1", "ProvisionService", 1L, "validate_request", "{}"
                     )
        );

        LinkingStepHandler handler = new LinkingStepHandler();
        var invokers =
            new java.util.LinkedHashMap<String, WorkflowStepHandlers.WorkflowStepInvoker>();
        WorkerRegistry.registerProvisionServiceWorkflowStepInvokers(invokers, handler);
        WorkflowRunner runner = new WorkflowRunner(
            backend, workflows, invokers, "ProvisionWorker", Duration.ofMinutes(1), 3
        );
        Optional<Workflow.WorkflowExecutionRecord> advanced =
            runner.runOnce("wf-1", "ProvisionService", 1L);
        if (!handler.handled || advanced.isEmpty() || !advanced.get().status().equals("Running") ||
            !advanced.get().currentStep().equals("create_remote_service"))
        {
            throw new IllegalStateException(
                "generated worker runner did not advance linked workflow step"
            );
        }
    }
}
