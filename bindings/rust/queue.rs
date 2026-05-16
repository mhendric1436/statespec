use std::time::{Duration, SystemTime};

use crate::backend::{Backend, BackendResult};
use crate::json::Json;

#[derive(Debug, Clone)]
pub struct QueueDefinition {
    pub queue: String,
    pub channel: String,
    pub visibility_timeout: Duration,
    pub max_attempts: u32,
    pub dead_letter_queue: Option<String>,
    pub metadata: Json,
}

#[derive(Debug, Clone)]
pub struct CreateQueueRequest {
    pub definition: QueueDefinition,
}

#[derive(Debug, Clone)]
pub struct QueueCreation {
    pub definition: QueueDefinition,
    pub created: bool,
}

#[derive(Debug, Clone)]
pub struct QueueMessageRecord {
    pub message_id: String,
    pub queue: String,
    pub channel: String,
    pub status: String,
    pub attempts: u64,
    pub claimed_by: Option<String>,
    pub claim_expires_at: Option<SystemTime>,
    pub payload: Json,
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

pub trait QueueStore<B: Backend> {
    fn create(&self, backend: &B, request: &CreateQueueRequest) -> BackendResult<QueueCreation>;

    fn create_tx(&self, tx: &mut B::Tx, request: &CreateQueueRequest) -> BackendResult<QueueCreation>;

    fn inspect_definition(
        &self,
        backend: &B,
        queue: &str,
        channel: &str,
    ) -> BackendResult<Option<QueueDefinition>>;

    fn inspect_definition_tx(
        &self,
        tx: &mut B::Tx,
        queue: &str,
        channel: &str,
    ) -> BackendResult<Option<QueueDefinition>>;

    fn enqueue(&self, backend: &B, request: &EnqueueMessageRequest) -> BackendResult<QueueMessageRecord>;

    fn enqueue_tx(&self, tx: &mut B::Tx, request: &EnqueueMessageRequest) -> BackendResult<QueueMessageRecord>;

    fn claim(&self, backend: &B, request: &ClaimMessageRequest) -> BackendResult<Vec<QueueMessageRecord>>;

    fn claim_tx(&self, tx: &mut B::Tx, request: &ClaimMessageRequest) -> BackendResult<Vec<QueueMessageRecord>>;

    fn acknowledge(&self, backend: &B, request: &AckMessageRequest) -> BackendResult<()>;

    fn acknowledge_tx(&self, tx: &mut B::Tx, request: &AckMessageRequest) -> BackendResult<()>;

    fn fail(&self, backend: &B, request: &FailMessageRequest) -> BackendResult<QueueMessageRecord>;

    fn fail_tx(&self, tx: &mut B::Tx, request: &FailMessageRequest) -> BackendResult<QueueMessageRecord>;

    fn inspect(&self, backend: &B, message_id: &str) -> BackendResult<Option<QueueMessageRecord>>;

    fn inspect_tx(&self, tx: &mut B::Tx, message_id: &str) -> BackendResult<Option<QueueMessageRecord>>;
}
