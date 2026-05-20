use crate::backend::{Backend, BackendError, BackendResult, ConflictKind};
use crate::memory_backend::{lock_error, InMemoryBackend};
use crate::memory_transaction::definition_key;
use crate::queue::{
    AckMessageRequest, ClaimMessageRequest, EnqueueMessageRequest, FailMessageRequest,
    QueueDefinition, QueueDefinitionRegistration, QueueMessageRecord, QueueStore,
    RegisterQueueDefinitionRequest,
};

#[derive(Debug, Clone, Default)]
pub struct InMemoryQueueStore;

impl InMemoryQueueStore {
    pub fn new() -> Self {
        Self
    }

    fn require_message(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        message_id: &str,
    ) -> BackendResult<QueueMessageRecord> {
        if let Some(message) = tx.queue_message_puts.get(message_id) {
            return Ok(message.clone());
        }
        tx.state
            .lock()
            .map_err(lock_error)?
            .queue_messages
            .get(message_id)
            .cloned()
            .ok_or_else(|| BackendError::NotFound {
                message: format!("unknown queue message {message_id}"),
            })
    }

    fn all_messages(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
    ) -> BackendResult<Vec<QueueMessageRecord>> {
        let mut by_id = tx.state.lock().map_err(lock_error)?.queue_messages.clone();
        by_id.append(&mut tx.queue_message_puts.clone());
        Ok(by_id.into_values().collect())
    }
}

impl QueueStore<InMemoryBackend> for InMemoryQueueStore {
    fn register_definition(
        &self,
        backend: &InMemoryBackend,
        request: &RegisterQueueDefinitionRequest,
    ) -> BackendResult<QueueDefinitionRegistration> {
        let mut tx = backend.begin()?;
        let result = self.register_definition_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn register_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &RegisterQueueDefinitionRequest,
    ) -> BackendResult<QueueDefinitionRegistration> {
        let existing =
            self.inspect_definition_tx(tx, &request.definition.queue, &request.definition.channel)?;
        tx.queue_definition_puts.insert(
            queue_definition_key(&request.definition.queue, &request.definition.channel),
            request.definition.clone(),
        );
        Ok(QueueDefinitionRegistration {
            definition: request.definition.clone(),
            created: existing.is_none(),
        })
    }

    fn inspect_definition(
        &self,
        backend: &InMemoryBackend,
        queue: &str,
        channel: &str,
    ) -> BackendResult<Option<QueueDefinition>> {
        let mut tx = backend.begin()?;
        let result = self.inspect_definition_tx(&mut tx, queue, channel)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        queue: &str,
        channel: &str,
    ) -> BackendResult<Option<QueueDefinition>> {
        let key = queue_definition_key(queue, channel);
        if let Some(definition) = tx.queue_definition_puts.get(&key) {
            return Ok(Some(definition.clone()));
        }
        Ok(tx
            .state
            .lock()
            .map_err(lock_error)?
            .queue_definitions
            .get(&key)
            .cloned())
    }

    fn enqueue(
        &self,
        backend: &InMemoryBackend,
        request: &EnqueueMessageRequest,
    ) -> BackendResult<QueueMessageRecord> {
        let mut tx = backend.begin()?;
        let result = self.enqueue_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn enqueue_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &EnqueueMessageRequest,
    ) -> BackendResult<QueueMessageRecord> {
        if let Some(idempotency) = &request.idempotency_key {
            let key = format!(
                "{}|{}",
                queue_definition_key(&request.queue, &request.channel),
                idempotency
            );
            if let Some(message_id) = tx.queue_idempotency_puts.get(&key).cloned() {
                return self.require_message(tx, &message_id);
            }
            let existing_message_id = tx
                .state
                .lock()
                .map_err(lock_error)?
                .queue_idempotency_keys
                .get(&key)
                .cloned();
            if let Some(message_id) = existing_message_id {
                return self.require_message(tx, &message_id);
            }
            tx.queue_idempotency_puts
                .insert(key, request.message_id.clone());
        }
        let record = QueueMessageRecord {
            message_id: request.message_id.clone(),
            queue: request.queue.clone(),
            channel: request.channel.clone(),
            status: "Pending".to_string(),
            attempts: 0,
            claimed_by: None,
            claim_expires_at: None,
            payload: request.payload.clone(),
        };
        tx.queue_message_puts
            .insert(record.message_id.clone(), record.clone());
        Ok(record)
    }

    fn claim(
        &self,
        backend: &InMemoryBackend,
        request: &ClaimMessageRequest,
    ) -> BackendResult<Vec<QueueMessageRecord>> {
        let mut tx = backend.begin()?;
        let result = self.claim_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn claim_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &ClaimMessageRequest,
    ) -> BackendResult<Vec<QueueMessageRecord>> {
        let mut claimed = Vec::new();
        for mut message in self.all_messages(tx)? {
            if claimed.len() >= request.max_messages as usize {
                break;
            }
            if message.queue != request.queue
                || message.channel != request.channel
                || message.status == "Acked"
                || message.status == "DeadLettered"
            {
                continue;
            }
            let visible = message.status == "Pending"
                || message
                    .claim_expires_at
                    .map(|expires| expires <= request.now)
                    .unwrap_or(false);
            if !visible {
                continue;
            }
            message.status = "Claimed".to_string();
            message.claimed_by = Some(request.claimant.clone());
            message.claim_expires_at = Some(request.now + request.visibility_timeout);
            message.attempts += 1;
            tx.queue_message_puts
                .insert(message.message_id.clone(), message.clone());
            claimed.push(message);
        }
        Ok(claimed)
    }

    fn acknowledge(
        &self,
        backend: &InMemoryBackend,
        request: &AckMessageRequest,
    ) -> BackendResult<()> {
        let mut tx = backend.begin()?;
        self.acknowledge_tx(&mut tx, request)?;
        backend.commit(tx)
    }

    fn acknowledge_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &AckMessageRequest,
    ) -> BackendResult<()> {
        let mut message = self.require_message(tx, &request.message_id)?;
        require_claimant(&message, &request.claimant)?;
        message.status = "Acked".to_string();
        tx.queue_message_puts
            .insert(message.message_id.clone(), message);
        Ok(())
    }

    fn fail(
        &self,
        backend: &InMemoryBackend,
        request: &FailMessageRequest,
    ) -> BackendResult<QueueMessageRecord> {
        let mut tx = backend.begin()?;
        let result = self.fail_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn fail_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &FailMessageRequest,
    ) -> BackendResult<QueueMessageRecord> {
        let mut message = self.require_message(tx, &request.message_id)?;
        require_claimant(&message, &request.claimant)?;
        message.claimed_by = None;
        message.claim_expires_at = None;
        message.status = if message.attempts >= request.max_attempts as u64 {
            "DeadLettered".to_string()
        } else {
            "Pending".to_string()
        };
        tx.queue_message_puts
            .insert(message.message_id.clone(), message.clone());
        Ok(message)
    }

    fn inspect(
        &self,
        backend: &InMemoryBackend,
        message_id: &str,
    ) -> BackendResult<Option<QueueMessageRecord>> {
        let mut tx = backend.begin()?;
        let result = self.inspect_tx(&mut tx, message_id)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        message_id: &str,
    ) -> BackendResult<Option<QueueMessageRecord>> {
        if let Some(message) = tx.queue_message_puts.get(message_id) {
            return Ok(Some(message.clone()));
        }
        Ok(tx
            .state
            .lock()
            .map_err(lock_error)?
            .queue_messages
            .get(message_id)
            .cloned())
    }
}

fn queue_definition_key(queue: &str, channel: &str) -> String {
    definition_key(&[queue.to_string(), channel.to_string()])
}

fn require_claimant(message: &QueueMessageRecord, claimant: &str) -> BackendResult<()> {
    if message.claimed_by.as_deref() != Some(claimant) {
        return Err(BackendError::Conflict {
            kind: ConflictKind::QueueClaimConflict,
            message: "queue message is not claimed by caller".to_string(),
        });
    }
    Ok(())
}
