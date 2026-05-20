package com.statespec.generated;

import com.statespec.backend.Json;
import com.statespec.backend.Workflow;
import com.statespec.backend.memory.InMemoryBackend;
import com.statespec.backend.memory.InMemoryWorkflowStore;
import java.time.Duration;
import java.util.List;
import java.util.Optional;

public final class WorkerLinkingFixture {
    private WorkerLinkingFixture() {}

    private static final class LinkingStepHandler implements WorkflowStepHandlers.Handler {
        private boolean handled;

        @Override
        public void handleWorkflowStep(WorkflowStepHandlers.Context context) {
            if (!context.workflowName().equals("ProvisionService") ||
                !context.stepName().equals("validate_request")) {
                throw new IllegalStateException("unexpected workflow step");
            }
            handled = true;
        }
    }

    public static void main(String[] args) throws Exception {
        InMemoryBackend backend = new InMemoryBackend();
        InMemoryWorkflowStore workflows = new InMemoryWorkflowStore();

        workflows.registerDefinition(
            backend,
            new Workflow.RegisterWorkflowDefinitionRequest(
                new Workflow.WorkflowDefinition(
                    "ProvisionService",
                    1L,
                    "validate_request",
                    Duration.ofMinutes(1),
                    false,
                    List.of(
                        new Workflow.WorkflowStepDefinition(
                            "validate_request",
                            Duration.ofSeconds(1),
                            3
                        )
                    ),
                    "{}"
                )
            )
        );
        workflows.start(
            backend,
            new Workflow.StartWorkflowRequest(
                "wf-1",
                "ProvisionService",
                1L,
                "validate_request",
                "{}"
            )
        );

        LinkingStepHandler handler = new LinkingStepHandler();
        WorkflowRunner runner =
            new WorkflowRunner(
                backend,
                workflows,
                handler,
                "ProvisionWorker",
                Duration.ofMinutes(1),
                3
            );
        Optional<Workflow.WorkflowExecutionRecord> completed =
            runner.runOnce("wf-1", "ProvisionService", 1L);
        if (!handler.handled || completed.isEmpty() ||
            !completed.get().status().equals("Completed")) {
            throw new IllegalStateException(
                "generated worker runner did not complete linked workflow step"
            );
        }
    }
}
