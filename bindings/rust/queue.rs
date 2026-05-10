use std::time::{Duration, SystemTime};

use crate::backend::{BackendResult, Json, QueueMessageRecord, Transaction};

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
