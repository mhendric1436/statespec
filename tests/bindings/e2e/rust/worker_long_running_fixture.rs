use std::sync::atomic::{AtomicUsize, Ordering};
use std::thread;
use std::time::Duration;

use statespec_generated::backend::{Backend, BackendResult, Transaction};
use statespec_generated::json::Json;
use statespec_generated::memory_backend::InMemoryBackend;
use statespec_generated::runtime_workflows::RuntimeWorkflowStore;
use statespec_generated::workflow::{
    CancelWorkflowRequest, ClaimWorkflowStepRequest, CompleteWorkflowStepRequest,
    FailWorkflowStepRequest, KeepAliveWorkflowStepRequest, RegisterWorkflowDefinitionRequest,
    StartWorkflowRequest, WorkflowDefinition, WorkflowDefinitionRegistration,
    WorkflowExecutionRecord, WorkflowStepDefinition, WorkflowStore,
};
use statespec_generated::workflow_provision_service_handlers::ProvisionServiceV1StepHandler;
use statespec_generated::workflow_provision_service_registry;
use statespec_generated::workflow_runner::WorkflowRunner;
use statespec_generated::workflow_step_handlers::{
    WorkflowStepHandlerContext, WorkflowStepInvokerMap, WorkflowStepResult,
};

struct CountingWorkflowStore {
    inner: RuntimeWorkflowStore,
    keep_alive_count: AtomicUsize,
}

impl CountingWorkflowStore {
    fn new() -> Self {
        Self {
            inner: RuntimeWorkflowStore::new(),
            keep_alive_count: AtomicUsize::new(0),
        }
    }

    fn keep_alive_count(&self) -> usize {
        self.keep_alive_count.load(Ordering::Relaxed)
    }
}

impl WorkflowStore<InMemoryBackend> for CountingWorkflowStore {
    fn register_definition(
        &self,
        backend: &InMemoryBackend,
        request: &RegisterWorkflowDefinitionRequest,
    ) -> BackendResult<WorkflowDefinitionRegistration> {
        self.inner.register_definition(backend, request)
    }

    fn register_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &RegisterWorkflowDefinitionRequest,
    ) -> BackendResult<WorkflowDefinitionRegistration> {
        <RuntimeWorkflowStore as WorkflowStore<InMemoryBackend>>::register_definition_tx(
            &self.inner,
            tx,
            request,
        )
    }

    fn inspect_definition(
        &self,
        backend: &InMemoryBackend,
        workflow_name: &str,
        workflow_version: i64,
    ) -> BackendResult<Option<WorkflowDefinition>> {
        self.inner
            .inspect_definition(backend, workflow_name, workflow_version)
    }

    fn inspect_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        workflow_name: &str,
        workflow_version: i64,
    ) -> BackendResult<Option<WorkflowDefinition>> {
        <RuntimeWorkflowStore as WorkflowStore<InMemoryBackend>>::inspect_definition_tx(
            &self.inner,
            tx,
            workflow_name,
            workflow_version,
        )
    }

    fn start(
        &self,
        backend: &InMemoryBackend,
        request: &StartWorkflowRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        self.inner.start(backend, request)
    }

    fn start_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &StartWorkflowRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        <RuntimeWorkflowStore as WorkflowStore<InMemoryBackend>>::start_tx(&self.inner, tx, request)
    }

    fn claim_steps(
        &self,
        backend: &InMemoryBackend,
        request: &ClaimWorkflowStepRequest,
    ) -> BackendResult<Vec<WorkflowExecutionRecord>> {
        self.inner.claim_steps(backend, request)
    }

    fn claim_steps_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &ClaimWorkflowStepRequest,
    ) -> BackendResult<Vec<WorkflowExecutionRecord>> {
        <RuntimeWorkflowStore as WorkflowStore<InMemoryBackend>>::claim_steps_tx(
            &self.inner,
            tx,
            request,
        )
    }

    fn keep_alive_step(
        &self,
        backend: &InMemoryBackend,
        request: &KeepAliveWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        self.keep_alive_count.fetch_add(1, Ordering::Relaxed);
        self.inner.keep_alive_step(backend, request)
    }

    fn keep_alive_step_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &KeepAliveWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        <RuntimeWorkflowStore as WorkflowStore<InMemoryBackend>>::keep_alive_step_tx(
            &self.inner,
            tx,
            request,
        )
    }

    fn complete_step(
        &self,
        backend: &InMemoryBackend,
        request: &CompleteWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        self.inner.complete_step(backend, request)
    }

    fn complete_step_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &CompleteWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        <RuntimeWorkflowStore as WorkflowStore<InMemoryBackend>>::complete_step_tx(
            &self.inner,
            tx,
            request,
        )
    }

    fn fail_step(
        &self,
        backend: &InMemoryBackend,
        request: &FailWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        self.inner.fail_step(backend, request)
    }

    fn fail_step_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &FailWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        <RuntimeWorkflowStore as WorkflowStore<InMemoryBackend>>::fail_step_tx(
            &self.inner,
            tx,
            request,
        )
    }

    fn cancel(
        &self,
        backend: &InMemoryBackend,
        request: &CancelWorkflowRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        self.inner.cancel(backend, request)
    }

    fn cancel_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &CancelWorkflowRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        <RuntimeWorkflowStore as WorkflowStore<InMemoryBackend>>::cancel_tx(
            &self.inner,
            tx,
            request,
        )
    }

    fn inspect(
        &self,
        backend: &InMemoryBackend,
        workflow_execution_id: &str,
    ) -> BackendResult<Option<WorkflowExecutionRecord>> {
        self.inner.inspect(backend, workflow_execution_id)
    }

    fn inspect_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        workflow_execution_id: &str,
    ) -> BackendResult<Option<WorkflowExecutionRecord>> {
        <RuntimeWorkflowStore as WorkflowStore<InMemoryBackend>>::inspect_tx(
            &self.inner,
            tx,
            workflow_execution_id,
        )
    }
}

struct LongRunningWorkflowStepHandler;

impl ProvisionServiceV1StepHandler<InMemoryBackend> for LongRunningWorkflowStepHandler {
    fn handle_validate_request(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        context: &WorkflowStepHandlerContext,
    ) -> BackendResult<WorkflowStepResult> {
        assert!(
            tx.is_open(),
            "workflow handler received a closed transaction"
        );
        assert_eq!(context.workflow_name, "ProvisionService");
        assert_eq!(context.step_name, "validate_request");
        thread::sleep(Duration::from_millis(850));
        Ok(WorkflowStepResult::complete(None))
    }

    fn handle_create_remote_service(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        _context: &WorkflowStepHandlerContext,
    ) -> BackendResult<WorkflowStepResult> {
        assert!(
            tx.is_open(),
            "workflow handler received a closed transaction"
        );
        Ok(WorkflowStepResult::fail(
            "unexpected create_remote_service step",
        ))
    }

    fn handle_wait_for_remote_service(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        _context: &WorkflowStepHandlerContext,
    ) -> BackendResult<WorkflowStepResult> {
        assert!(
            tx.is_open(),
            "workflow handler received a closed transaction"
        );
        Ok(WorkflowStepResult::fail(
            "unexpected wait_for_remote_service step",
        ))
    }
}

#[test]
fn long_running_workflow_step_receives_periodic_keepalive() {
    let backend = InMemoryBackend::new();
    let workflows = CountingWorkflowStore::new();

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
                workflow_execution_id: "wf-long".to_string(),
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
        std::sync::Arc::new(LongRunningWorkflowStepHandler),
    );
    let runner = WorkflowRunner {
        backend: &backend,
        workflow_store: &workflows,
        workflow_step_invokers: &workflow_step_invokers,
        worker_name: "ProvisionWorker".to_string(),
        lease_duration: Duration::from_secs(1),
        max_attempts: 3,
    };
    let completed = runner
        .run_once("wf-long", "ProvisionService", 1)
        .expect("workflow runner failed")
        .expect("workflow step was not claimed");
    assert_eq!(completed.status, "Completed");
    assert!(
        workflows.keep_alive_count() >= 2,
        "long-running workflow step did not receive periodic keepalive"
    );
}
