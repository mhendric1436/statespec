use std::time::{Duration, SystemTime};

use crate::backend::{BackendResult, Json, LeaseRecord, QueueMessageRecord, Transaction, WorkflowExecutionRecord};

#[derive(Debug, Clone)]
pub struct LeaseAcquireRequest {
    pub resource: String,
    pub holder: String,
    pub now: SystemTime,
    pub ttl: Duration,
}

#[derive(Debug, Clone)]
pub struct LeaseRenewRequest {
    pub resource: String,
    pub holder: String,
    pub fencing_token: u64,
    pub now: SystemTime,
    pub ttl: Duration,
}

#[derive(Debug, Clone)]
pub struct LeaseReleaseRequest {
    pub resource: String,
    pub holder: String,
    pub fencing_token: u64,
}

#[derive(Debug, Clone)]
pub struct LeaseAcquireResult {
    pub acquired: bool,
    pub lease: Option<LeaseRecord>,
}

pub trait LeaseStore<Tx: Transaction> {
    fn acquire(&self, tx: &mut Tx, request: &LeaseAcquireRequest) -> BackendResult<LeaseAcquireResult>;

    fn renew(&self, tx: &mut Tx, request: &LeaseRenewRequest) -> BackendResult<LeaseRecord>;

    fn release(&self, tx: &mut Tx, request: &LeaseReleaseRequest) -> BackendResult<()>;

    fn inspect(&self, tx: &mut Tx, resource: &str) -> BackendResult<Option<LeaseRecord>>;
}

#[derive(Debug, Clone)]
pub struct EnqueueMessageRequest {
    pub message_id: String,
    pub queue: String,
    pub channel: String,
    pub idempotency_key: Option<String>,
    pub payload: Json,
}

#[derive(Debug, Clone)]
pub struct ClaimMessageRequest {
    pub queue: String,
    pub channel: String,
    pub claimant: String,
    pub now: SystemTime,
    pub visibility_timeout: Duration,
    pub max_messages: u32,
}

#[derive(Debug, Clone)]
pub struct AckMessageRequest {
    pub message_id: String,
    pub claimant: String,
}

#[derive(Debug, Clone)]
pub struct FailMessageRequest {
    pub message_id: String,
    pub claimant: String,
    pub reason: String,
    pub now: SystemTime,
    pub max_attempts: u32,
}

pub trait QueueStore<Tx: Transaction> {
    fn enqueue(&self, tx: &mut Tx, request: &EnqueueMessageRequest) -> BackendResult<QueueMessageRecord>;

    fn claim(&self, tx: &mut Tx, request: &ClaimMessageRequest) -> BackendResult<Vec<QueueMessageRecord>>;

    fn acknowledge(&self, tx: &mut Tx, request: &AckMessageRequest) -> BackendResult<()>;

    fn fail(&self, tx: &mut Tx, request: &FailMessageRequest) -> BackendResult<QueueMessageRecord>;

    fn inspect(&self, tx: &mut Tx, message_id: &str) -> BackendResult<Option<QueueMessageRecord>>;
}

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
