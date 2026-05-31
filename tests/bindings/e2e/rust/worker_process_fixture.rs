use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::thread;
use std::time::Duration;

use statespec_generated::json::Json;
use statespec_generated::memory_backend::InMemoryBackend;
use statespec_generated::runtime_workflows::RuntimeWorkflowStore;
use statespec_generated::worker_process::WorkerProcess;
use statespec_generated::worker_process::WorkerProcessConfig;
use statespec_generated::worker_runtime::WorkerRuntime;
use statespec_generated::workflow::{StartWorkflowRequest, WorkflowStore};
use statespec_generated::workflow_provision_service_handlers::ProvisionServiceV1StepHandler;
use statespec_generated::workflow_provision_service_registry;
use statespec_generated::workflow_step_handlers::{
    WorkflowStepHandlerContext, WorkflowStepInvokerMap, WorkflowStepResult,
};

#[derive(Clone)]
struct ProcessWorkflowStepHandler {
    handled_validate_request: Arc<AtomicBool>,
}

impl ProvisionServiceV1StepHandler<InMemoryBackend> for ProcessWorkflowStepHandler {
    fn handle_validate_request(
        &self,
        _tx: &mut <InMemoryBackend as statespec_generated::backend::Backend>::Tx,
        context: &WorkflowStepHandlerContext,
    ) -> statespec_generated::backend::BackendResult<WorkflowStepResult> {
        assert_eq!(context.workflow_name, "ProvisionService");
        assert_eq!(context.step_name, "validate_request");
        self.handled_validate_request.store(true, Ordering::SeqCst);
        Ok(WorkflowStepResult::complete(Some(
            "create_remote_service".to_string(),
        )))
    }

    fn handle_create_remote_service(
        &self,
        _tx: &mut <InMemoryBackend as statespec_generated::backend::Backend>::Tx,
        _context: &WorkflowStepHandlerContext,
    ) -> statespec_generated::backend::BackendResult<WorkflowStepResult> {
        Ok(WorkflowStepResult::complete(Some(
            "wait_for_remote_service".to_string(),
        )))
    }

    fn handle_wait_for_remote_service(
        &self,
        _tx: &mut <InMemoryBackend as statespec_generated::backend::Backend>::Tx,
        _context: &WorkflowStepHandlerContext,
    ) -> statespec_generated::backend::BackendResult<WorkflowStepResult> {
        Ok(WorkflowStepResult::complete(None))
    }
}

#[test]
fn generated_worker_process_lifecycle() {
    let backend = InMemoryBackend::new();
    let runtime = WorkerRuntime::new(backend.clone());
    let handled_validate_request = Arc::new(AtomicBool::new(false));
    let handler = ProcessWorkflowStepHandler {
        handled_validate_request: Arc::clone(&handled_validate_request),
    };
    let mut workflow_step_invokers = WorkflowStepInvokerMap::new();
    workflow_provision_service_registry::register_workflow_step_invokers::<InMemoryBackend>(
        &mut workflow_step_invokers,
        Arc::new(handler),
    );
    let workflow_step_invokers = Arc::new(workflow_step_invokers);
    let config = WorkerProcessConfig {
        bootstrap_on_start: true,
        worker_poll_interval_ms: 1,
    };
    let process = Arc::new(WorkerProcess::with_config(
        runtime,
        workflow_step_invokers,
        config,
    ));

    assert!(
        process.join().is_err(),
        "joining a WorkerProcess before start should fail"
    );
    process.request_stop();

    process
        .runtime
        .bootstrap()
        .expect("bootstrapping Worker runtime failed");
    let workflows = RuntimeWorkflowStore::new();
    workflows
        .start(
            &backend,
            &StartWorkflowRequest {
                workflow_execution_id: "wf-process-1".to_string(),
                workflow_name: "ProvisionService".to_string(),
                workflow_version: 1,
                start_step: "validate_request".to_string(),
                state: Json::Object(Default::default()),
            },
        )
        .expect("starting workflow failed");

    process.start().expect("starting WorkerProcess failed");
    assert!(
        process.start().is_err(),
        "starting WorkerProcess twice should fail"
    );
    thread::sleep(Duration::from_millis(10));
    assert!(
        process.is_running(),
        "started WorkerProcess should report running"
    );

    for _ in 0..200 {
        if handled_validate_request.load(Ordering::SeqCst) {
            break;
        }
        thread::sleep(Duration::from_millis(10));
    }
    assert!(
        handled_validate_request.load(Ordering::SeqCst),
        "WorkerProcess did not execute a workflow step"
    );

    process.request_stop();
    process
        .join()
        .expect("stopped WorkerProcess should join cleanly");
    assert!(
        !process.is_running(),
        "joined WorkerProcess should not report running"
    );
}
