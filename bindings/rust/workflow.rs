use std::time::{Duration, SystemTime};

use crate::backend::{BackendResult, Json, Transaction, WorkflowExecutionRecord};

#[derive(Debug, Clone)]
pub struct StartWorkflowRequest {
    pub workflow_execution_id: String,
    pub workflow_name: String,
    pub start_step: String,
    pub state: Json,
}

#[derive(Debug, Clone)]
pub struct ClaimWorkflowStepRequest {
    pub workflow_name: String,
    pub worker: String,
    pub now: SystemTime,
    pub lease_duration: Duration,
    pub max_steps: u32,
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

pub trait WorkflowStore<Tx: Transaction> {
    fn start(&self, tx: &mut Tx, request: &StartWorkflowRequest) -> BackendResult<WorkflowExecutionRecord>;

    fn claim_steps(&self, tx: &mut Tx, request: &ClaimWorkflowStepRequest) -> BackendResult<Vec<WorkflowExecutionRecord>>;

    fn complete_step(&self, tx: &mut Tx, request: &CompleteWorkflowStepRequest) -> BackendResult<WorkflowExecutionRecord>;

    fn fail_step(&self, tx: &mut Tx, request: &FailWorkflowStepRequest) -> BackendResult<WorkflowExecutionRecord>;

    fn cancel(&self, tx: &mut Tx, request: &CancelWorkflowRequest) -> BackendResult<WorkflowExecutionRecord>;

    fn inspect(&self, tx: &mut Tx, workflow_execution_id: &str) -> BackendResult<Option<WorkflowExecutionRecord>>;
}
