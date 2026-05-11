use std::time::{Duration, SystemTime};

use crate::backend::{Backend, BackendResult, Json, Transaction};

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

pub trait QueueStore<Tx: Transaction> {
    fn create(&self, tx: &mut Tx, request: &CreateQueueRequest) -> BackendResult<QueueCreation>;

    fn inspect_definition(
        &self,
        tx: &mut Tx,
        queue: &str,
        channel: &str,
    ) -> BackendResult<Option<QueueDefinition>>;

    fn enqueue(&self, tx: &mut Tx, request: &EnqueueMessageRequest) -> BackendResult<QueueMessageRecord>;

    fn claim(&self, tx: &mut Tx, request: &ClaimMessageRequest) -> BackendResult<Vec<QueueMessageRecord>>;

    fn acknowledge(&self, tx: &mut Tx, request: &AckMessageRequest) -> BackendResult<()>;

    fn fail(&self, tx: &mut Tx, request: &FailMessageRequest) -> BackendResult<QueueMessageRecord>;

    fn inspect(&self, tx: &mut Tx, message_id: &str) -> BackendResult<Option<QueueMessageRecord>>;
}

pub struct QueueRuntime<'a, B, S>
where
    B: Backend,
    S: QueueStore<B::Tx>,
{
    backend: &'a B,
    store: &'a S,
}

impl<'a, B, S> QueueRuntime<'a, B, S>
where
    B: Backend,
    S: QueueStore<B::Tx>,
{
    pub fn new(backend: &'a B, store: &'a S) -> Self {
        Self { backend, store }
    }

    pub fn create(&self, request: &CreateQueueRequest) -> BackendResult<QueueCreation> {
        let mut tx = self.backend.begin()?;
        match self.store.create(&mut tx, request) {
            Ok(result) => {
                self.backend.commit(tx)?;
                Ok(result)
            }
            Err(err) => {
                let _ = tx.abort();
                Err(err)
            }
        }
    }

    pub fn inspect_definition(&self, queue: &str, channel: &str) -> BackendResult<Option<QueueDefinition>> {
        let mut tx = self.backend.begin()?;
        match self.store.inspect_definition(&mut tx, queue, channel) {
            Ok(result) => {
                self.backend.commit(tx)?;
                Ok(result)
            }
            Err(err) => {
                let _ = tx.abort();
                Err(err)
            }
        }
    }

    pub fn enqueue(&self, request: &EnqueueMessageRequest) -> BackendResult<QueueMessageRecord> {
        let mut tx = self.backend.begin()?;
        match self.store.enqueue(&mut tx, request) {
            Ok(result) => {
                self.backend.commit(tx)?;
                Ok(result)
            }
            Err(err) => {
                let _ = tx.abort();
                Err(err)
            }
        }
    }

    pub fn claim(&self, request: &ClaimMessageRequest) -> BackendResult<Vec<QueueMessageRecord>> {
        let mut tx = self.backend.begin()?;
        match self.store.claim(&mut tx, request) {
            Ok(result) => {
                self.backend.commit(tx)?;
                Ok(result)
            }
            Err(err) => {
                let _ = tx.abort();
                Err(err)
            }
        }
    }

    pub fn acknowledge(&self, request: &AckMessageRequest) -> BackendResult<()> {
        let mut tx = self.backend.begin()?;
        match self.store.acknowledge(&mut tx, request) {
            Ok(()) => self.backend.commit(tx),
            Err(err) => {
                let _ = tx.abort();
                Err(err)
            }
        }
    }

    pub fn fail(&self, request: &FailMessageRequest) -> BackendResult<QueueMessageRecord> {
        let mut tx = self.backend.begin()?;
        match self.store.fail(&mut tx, request) {
            Ok(result) => {
                self.backend.commit(tx)?;
                Ok(result)
            }
            Err(err) => {
                let _ = tx.abort();
                Err(err)
            }
        }
    }

    pub fn inspect(&self, message_id: &str) -> BackendResult<Option<QueueMessageRecord>> {
        let mut tx = self.backend.begin()?;
        match self.store.inspect(&mut tx, message_id) {
            Ok(result) => {
                self.backend.commit(tx)?;
                Ok(result)
            }
            Err(err) => {
                let _ = tx.abort();
                Err(err)
            }
        }
    }
}
