use std::time::Duration;

use statespec_generated::json::Json;
use statespec_generated::memory_backend::InMemoryBackend;
use statespec_generated::memory_workflows::InMemoryWorkflowStore;
use statespec_generated::workflow::{
    RegisterWorkflowDefinitionRequest, StartWorkflowRequest, WorkflowDefinition,
    WorkflowStepDefinition, WorkflowStore,
};
use statespec_generated::workflow_runner::WorkflowRunner;
use statespec_generated::workflow_step_handlers::{
    WorkflowStepHandler, WorkflowStepHandlerContext,
};

struct LinkingWorkflowStepHandler;

impl WorkflowStepHandler for LinkingWorkflowStepHandler {
    fn handle_workflow_step(
        &self,
        context: &WorkflowStepHandlerContext,
    ) -> statespec_generated::backend::BackendResult<()> {
        assert_eq!(context.workflow_name, "ProvisionService");
        assert_eq!(context.step_name, "validate_request");
        Ok(())
    }
}

#[test]
fn generated_worker_runner_links_with_memory_backend() {
    let backend = InMemoryBackend::new();
    let workflows = InMemoryWorkflowStore::new();

    workflows
        .register_definition(
            &backend,
            &RegisterWorkflowDefinitionRequest {
                definition: WorkflowDefinition {
                    workflow_name: "ProvisionService".to_string(),
                    workflow_version: 1,
                    start_step: "validate_request".to_string(),
                    expected_execution_time: Duration::from_secs(60),
                    singleton: false,
                    steps: vec![WorkflowStepDefinition {
                        name: "validate_request".to_string(),
                        expected_execution_time: Duration::from_secs(1),
                        max_retries: 3,
                    }],
                    metadata: Json::Object(Default::default()),
                },
            },
        )
        .unwrap();
    workflows
        .start(
            &backend,
            &StartWorkflowRequest {
                workflow_execution_id: "wf-1".to_string(),
                workflow_name: "ProvisionService".to_string(),
                workflow_version: 1,
                start_step: "validate_request".to_string(),
                state: Json::Object(Default::default()),
            },
        )
        .unwrap();

    let handler = LinkingWorkflowStepHandler;
    let runner = WorkflowRunner {
        backend: &backend,
        workflow_store: &workflows,
        handler: &handler,
        worker_name: "ProvisionWorker".to_string(),
        lease_duration: Duration::from_secs(60),
        max_attempts: 3,
    };
    let completed = runner
        .run_once("wf-1", "ProvisionService", 1)
        .expect("workflow runner failed")
        .expect("workflow step was not claimed");
    assert_eq!(completed.status, "Completed");
}
