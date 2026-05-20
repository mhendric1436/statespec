use std::time::{Duration, SystemTime};

use crate::backend::{Backend, BackendResult};
use crate::json::Json;

#[derive(Debug, Clone)]
pub struct WorkflowStepDefinition {
    pub name: String,
    pub expected_execution_time: Duration,
    pub max_retries: u32,
}

#[derive(Debug, Clone)]
pub struct WorkflowDefinition {
    pub workflow_name: String,
    pub workflow_version: i64,
    pub start_step: String,
    pub expected_execution_time: Duration,
    pub singleton: bool,
    pub steps: Vec<WorkflowStepDefinition>,
    pub metadata: Json,
}

#[derive(Debug, Clone)]
pub struct RegisterWorkflowDefinitionRequest {
    pub definition: WorkflowDefinition,
}

#[derive(Debug, Clone)]
pub struct WorkflowDefinitionRegistration {
    pub definition: WorkflowDefinition,
    pub created: bool,
}

#[derive(Debug, Clone)]
pub struct WorkflowExecutionRecord {
    pub workflow_execution_id: String,
    pub workflow_name: String,
    pub workflow_version: i64,
    pub current_step: String,
    pub status: String,
    pub attempt: u64,
    pub claimed_by: Option<String>,
    pub claim_expires_at: Option<SystemTime>,
    pub state: Json,
}

#[derive(Debug, Clone)]
pub struct StartWorkflowRequest {
    pub workflow_execution_id: String,
    pub workflow_name: String,
    pub workflow_version: i64,
    pub start_step: String,
    pub state: Json,
}

#[derive(Debug, Clone)]
pub struct ClaimWorkflowStepRequest {
    pub workflow_execution_id: String,
    pub workflow_name: String,
    pub workflow_version: i64,
    pub worker: String,
    pub now: SystemTime,
    pub lease_duration: Duration,
    pub max_steps: u32,
}

#[derive(Debug, Clone)]
pub struct KeepAliveWorkflowStepRequest {
    pub workflow_execution_id: String,
    pub worker: String,
    pub current_step: String,
    pub now: SystemTime,
    pub lease_duration: Duration,
}

#[derive(Debug, Clone)]
pub struct CompleteWorkflowStepRequest {
    pub workflow_execution_id: String,
    pub worker: String,
    pub completed_step: String,
    pub next_step: Option<String>,
    pub state: Json,
}

#[derive(Debug, Clone)]
pub struct FailWorkflowStepRequest {
    pub workflow_execution_id: String,
    pub worker: String,
    pub failed_step: String,
    pub reason: String,
    pub now: SystemTime,
    pub max_attempts: u32,
}

#[derive(Debug, Clone)]
pub struct CancelWorkflowRequest {
    pub workflow_execution_id: String,
    pub reason: String,
}

pub trait WorkflowStore<B: Backend> {
    fn register_definition(
        &self,
        backend: &B,
        request: &RegisterWorkflowDefinitionRequest,
    ) -> BackendResult<WorkflowDefinitionRegistration>;

    fn register_definition_tx(
        &self,
        tx: &mut B::Tx,
        request: &RegisterWorkflowDefinitionRequest,
    ) -> BackendResult<WorkflowDefinitionRegistration>;

    fn inspect_definition(
        &self,
        backend: &B,
        workflow_name: &str,
        workflow_version: i64,
    ) -> BackendResult<Option<WorkflowDefinition>>;

    fn inspect_definition_tx(
        &self,
        tx: &mut B::Tx,
        workflow_name: &str,
        workflow_version: i64,
    ) -> BackendResult<Option<WorkflowDefinition>>;

    fn start(
        &self,
        backend: &B,
        request: &StartWorkflowRequest,
    ) -> BackendResult<WorkflowExecutionRecord>;

    fn start_tx(
        &self,
        tx: &mut B::Tx,
        request: &StartWorkflowRequest,
    ) -> BackendResult<WorkflowExecutionRecord>;

    fn claim_steps(
        &self,
        backend: &B,
        request: &ClaimWorkflowStepRequest,
    ) -> BackendResult<Vec<WorkflowExecutionRecord>>;

    fn claim_steps_tx(
        &self,
        tx: &mut B::Tx,
        request: &ClaimWorkflowStepRequest,
    ) -> BackendResult<Vec<WorkflowExecutionRecord>>;

    fn keep_alive_step(
        &self,
        backend: &B,
        request: &KeepAliveWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord>;

    fn keep_alive_step_tx(
        &self,
        tx: &mut B::Tx,
        request: &KeepAliveWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord>;

    fn complete_step(
        &self,
        backend: &B,
        request: &CompleteWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord>;

    fn complete_step_tx(
        &self,
        tx: &mut B::Tx,
        request: &CompleteWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord>;

    fn fail_step(
        &self,
        backend: &B,
        request: &FailWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord>;

    fn fail_step_tx(
        &self,
        tx: &mut B::Tx,
        request: &FailWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord>;

    fn cancel(
        &self,
        backend: &B,
        request: &CancelWorkflowRequest,
    ) -> BackendResult<WorkflowExecutionRecord>;

    fn cancel_tx(
        &self,
        tx: &mut B::Tx,
        request: &CancelWorkflowRequest,
    ) -> BackendResult<WorkflowExecutionRecord>;

    fn inspect(
        &self,
        backend: &B,
        workflow_execution_id: &str,
    ) -> BackendResult<Option<WorkflowExecutionRecord>>;

    fn inspect_tx(
        &self,
        tx: &mut B::Tx,
        workflow_execution_id: &str,
    ) -> BackendResult<Option<WorkflowExecutionRecord>>;
}
