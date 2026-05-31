use std::time::Duration;

use statespec_generated::backend::{Backend, Transaction};
use statespec_generated::json::Json;
use statespec_generated::memory_backend::InMemoryBackend;
use statespec_generated::runtime_workflows::RuntimeWorkflowStore;
use statespec_generated::workflow::{
    RegisterWorkflowDefinitionRequest, StartWorkflowRequest, WorkflowDefinition,
    WorkflowStepDefinition, WorkflowStore,
};
use statespec_generated::workflow_provision_service_handlers::ProvisionServiceV1StepHandler;
use statespec_generated::workflow_provision_service_registry;
use statespec_generated::workflow_runner::WorkflowRunner;
use statespec_generated::workflow_step_handlers::{
    WorkflowStepHandlerContext, WorkflowStepInvokerMap, WorkflowStepResult,
};

struct LinkingWorkflowStepHandler;

impl ProvisionServiceV1StepHandler<InMemoryBackend> for LinkingWorkflowStepHandler {
    fn handle_validate_request(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        context: &WorkflowStepHandlerContext,
    ) -> statespec_generated::backend::BackendResult<WorkflowStepResult> {
        assert!(tx.is_open(), "workflow handler received a closed transaction");
        assert_eq!(context.workflow_name, "ProvisionService");
        assert_eq!(context.step_name, "validate_request");
        Ok(WorkflowStepResult::complete(Some(
            "create_remote_service".to_string(),
        )))
    }

    fn handle_create_remote_service(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        _context: &WorkflowStepHandlerContext,
    ) -> statespec_generated::backend::BackendResult<WorkflowStepResult> {
        assert!(tx.is_open(), "workflow handler received a closed transaction");
        Ok(WorkflowStepResult::fail(
            "unexpected create_remote_service step",
        ))
    }

    fn handle_wait_for_remote_service(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        _context: &WorkflowStepHandlerContext,
    ) -> statespec_generated::backend::BackendResult<WorkflowStepResult> {
        assert!(tx.is_open(), "workflow handler received a closed transaction");
        Ok(WorkflowStepResult::fail(
            "unexpected wait_for_remote_service step",
        ))
    }
}

#[test]
fn generated_worker_runner_links_with_memory_backend() {
    let backend = InMemoryBackend::new();
    let workflows = RuntimeWorkflowStore::new();

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
                    steps: vec![
                        WorkflowStepDefinition {
                            name: "validate_request".to_string(),
                            expected_execution_time: Duration::from_secs(1),
                            max_retries: 3,
                        },
                        WorkflowStepDefinition {
                            name: "create_remote_service".to_string(),
                            expected_execution_time: Duration::from_secs(1),
                            max_retries: 3,
                        },
                        WorkflowStepDefinition {
                            name: "wait_for_remote_service".to_string(),
                            expected_execution_time: Duration::from_secs(1),
                            max_retries: 3,
                        },
                    ],
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

    let mut workflow_step_invokers = WorkflowStepInvokerMap::new();
    workflow_provision_service_registry::register_workflow_step_invokers::<InMemoryBackend>(
        &mut workflow_step_invokers,
        std::sync::Arc::new(LinkingWorkflowStepHandler),
    );
    let runner = WorkflowRunner {
        backend: &backend,
        workflow_store: &workflows,
        workflow_step_invokers: &workflow_step_invokers,
        worker_name: "ProvisionWorker".to_string(),
        lease_duration: Duration::from_secs(60),
        max_attempts: 3,
    };
    let advanced = runner
        .run_once("wf-1", "ProvisionService", 1)
        .expect("workflow runner failed")
        .expect("workflow step was not claimed");
    assert_eq!(advanced.status, "Running");
    assert_eq!(advanced.current_step, "create_remote_service");
}
