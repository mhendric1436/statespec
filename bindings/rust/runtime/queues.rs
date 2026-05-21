use crate::backend::{Backend, BackendError, BackendResult, ConflictKind, Query, Transaction};
use crate::json::Json;
use crate::queue::{
    AckMessageRequest, ClaimMessageRequest, EnqueueMessageRequest, FailMessageRequest,
    QueueDefinition, QueueDefinitionRegistration, QueueMessageRecord, QueueStore,
    RegisterQueueDefinitionRequest,
};
use crate::runtime_codec;
use crate::runtime_codec::definition_key;

const DEFINITIONS: &str = "queues.definitions";
const MESSAGES: &str = "queues.messages";
const IDEMPOTENCY: &str = "queues.idempotency";

#[derive(Debug, Clone, Default)]
pub struct RuntimeQueueStore;

impl RuntimeQueueStore {
    pub fn new() -> Self {
        Self
    }

    fn require_message<B: Backend>(
        &self,
        tx: &mut B::Tx,
        message_id: &str,
    ) -> BackendResult<QueueMessageRecord> {
        tx.get(MESSAGES, message_id)?
            .map(|record| runtime_codec::queue_message_from_json(&record.document))
            .ok_or_else(|| BackendError::NotFound {
                message: format!("unknown queue message {message_id}"),
            })
    }

    fn all_messages<B: Backend>(&self, tx: &mut B::Tx) -> BackendResult<Vec<QueueMessageRecord>> {
        Ok(tx
            .query(MESSAGES, &Query::All)?
            .iter()
            .map(|record| runtime_codec::queue_message_from_json(&record.document))
            .collect())
    }
}

impl<B: Backend> QueueStore<B> for RuntimeQueueStore {
    fn register_definition(
        &self,
        backend: &B,
        request: &RegisterQueueDefinitionRequest,
    ) -> BackendResult<QueueDefinitionRegistration> {
        let mut tx = backend.begin()?;
        let result =
            <RuntimeQueueStore as QueueStore<B>>::register_definition_tx(self, &mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn register_definition_tx(
        &self,
        tx: &mut B::Tx,
        request: &RegisterQueueDefinitionRequest,
    ) -> BackendResult<QueueDefinitionRegistration> {
        let existing = <RuntimeQueueStore as QueueStore<B>>::inspect_definition_tx(
            self,
            tx,
            &request.definition.queue,
            &request.definition.channel,
        )?;
        tx.put(
            DEFINITIONS,
            &queue_definition_key(&request.definition.queue, &request.definition.channel),
            runtime_codec::queue_definition_to_json(&request.definition),
        )?;
        Ok(QueueDefinitionRegistration {
            definition: request.definition.clone(),
            created: existing.is_none(),
        })
    }

    fn inspect_definition(
        &self,
        backend: &B,
        queue: &str,
        channel: &str,
    ) -> BackendResult<Option<QueueDefinition>> {
        let mut tx = backend.begin()?;
        let result = <RuntimeQueueStore as QueueStore<B>>::inspect_definition_tx(
            self, &mut tx, queue, channel,
        )?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_definition_tx(
        &self,
        tx: &mut B::Tx,
        queue: &str,
        channel: &str,
    ) -> BackendResult<Option<QueueDefinition>> {
        let key = queue_definition_key(queue, channel);
        Ok(tx
            .get(DEFINITIONS, &key)?
            .map(|record| runtime_codec::queue_definition_from_json(&record.document)))
    }

    fn enqueue(
        &self,
        backend: &B,
        request: &EnqueueMessageRequest,
    ) -> BackendResult<QueueMessageRecord> {
        let mut tx = backend.begin()?;
        let result = <RuntimeQueueStore as QueueStore<B>>::enqueue_tx(self, &mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn enqueue_tx(
        &self,
        tx: &mut B::Tx,
        request: &EnqueueMessageRequest,
    ) -> BackendResult<QueueMessageRecord> {
        if let Some(idempotency) = &request.idempotency_key {
            let key = format!(
                "{}|{}",
                queue_definition_key(&request.queue, &request.channel),
                idempotency
            );
            if let Some(record) = tx.get(IDEMPOTENCY, &key)? {
                let message_id = runtime_codec::string_from_json(&record.document);
                return self.require_message::<B>(tx, &message_id);
            }
            tx.put(IDEMPOTENCY, &key, Json::String(request.message_id.clone()))?;
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
        tx.put(
            MESSAGES,
            &record.message_id,
            runtime_codec::queue_message_to_json(&record),
        )?;
        Ok(record)
    }

    fn claim(
        &self,
        backend: &B,
        request: &ClaimMessageRequest,
    ) -> BackendResult<Vec<QueueMessageRecord>> {
        let mut tx = backend.begin()?;
        let result = <RuntimeQueueStore as QueueStore<B>>::claim_tx(self, &mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn claim_tx(
        &self,
        tx: &mut B::Tx,
        request: &ClaimMessageRequest,
    ) -> BackendResult<Vec<QueueMessageRecord>> {
        let mut claimed = Vec::new();
        for mut message in self.all_messages::<B>(tx)? {
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
            tx.put(
                MESSAGES,
                &message.message_id,
                runtime_codec::queue_message_to_json(&message),
            )?;
            claimed.push(message);
        }
        Ok(claimed)
    }

    fn acknowledge(&self, backend: &B, request: &AckMessageRequest) -> BackendResult<()> {
        let mut tx = backend.begin()?;
        <RuntimeQueueStore as QueueStore<B>>::acknowledge_tx(self, &mut tx, request)?;
        backend.commit(tx)
    }

    fn acknowledge_tx(&self, tx: &mut B::Tx, request: &AckMessageRequest) -> BackendResult<()> {
        let mut message = self.require_message::<B>(tx, &request.message_id)?;
        require_claimant(&message, &request.claimant)?;
        message.status = "Acked".to_string();
        tx.put(
            MESSAGES,
            &message.message_id,
            runtime_codec::queue_message_to_json(&message),
        )?;
        Ok(())
    }

    fn fail(&self, backend: &B, request: &FailMessageRequest) -> BackendResult<QueueMessageRecord> {
        let mut tx = backend.begin()?;
        let result = <RuntimeQueueStore as QueueStore<B>>::fail_tx(self, &mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn fail_tx(
        &self,
        tx: &mut B::Tx,
        request: &FailMessageRequest,
    ) -> BackendResult<QueueMessageRecord> {
        let mut message = self.require_message::<B>(tx, &request.message_id)?;
        require_claimant(&message, &request.claimant)?;
        message.claimed_by = None;
        message.claim_expires_at = None;
        message.status = if message.attempts >= request.max_attempts as u64 {
            "DeadLettered".to_string()
        } else {
            "Pending".to_string()
        };
        tx.put(
            MESSAGES,
            &message.message_id,
            runtime_codec::queue_message_to_json(&message),
        )?;
        Ok(message)
    }

    fn inspect(&self, backend: &B, message_id: &str) -> BackendResult<Option<QueueMessageRecord>> {
        let mut tx = backend.begin()?;
        let result = <RuntimeQueueStore as QueueStore<B>>::inspect_tx(self, &mut tx, message_id)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_tx(
        &self,
        tx: &mut B::Tx,
        message_id: &str,
    ) -> BackendResult<Option<QueueMessageRecord>> {
        Ok(tx
            .get(MESSAGES, message_id)?
            .map(|record| runtime_codec::queue_message_from_json(&record.document)))
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
