use crate::backend::{Backend, BackendError, BackendResult, ConflictKind};
use crate::lease::{
    LeaseAcquireRequest, LeaseAcquireResult, LeaseDefinition, LeaseDefinitionId,
    LeaseInspectRequest, LeaseRecord, LeaseRegisterDefinitionResult, LeaseReleaseRequest,
    LeaseRenewRequest, LeaseStore,
};
use crate::memory_backend::InMemoryBackend;
use crate::memory_codec;
use crate::memory_transaction::definition_key;

const DEFINITIONS: &str = "leases.definitions";
const LEASES: &str = "leases.records";

#[derive(Debug, Clone, Default)]
pub struct InMemoryLeaseStore;

impl InMemoryLeaseStore {
    pub fn new() -> Self {
        Self
    }

    fn require_lease(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        definition_id: &LeaseDefinitionId,
        resource: &str,
    ) -> BackendResult<LeaseRecord> {
        self.inspect_tx(
            tx,
            &LeaseInspectRequest {
                definition_id: definition_id.clone(),
                resource: resource.to_string(),
            },
        )?
        .ok_or_else(|| BackendError::NotFound {
            message: "unknown lease".to_string(),
        })
    }
}

impl LeaseStore<InMemoryBackend> for InMemoryLeaseStore {
    fn register_definition(
        &self,
        backend: &InMemoryBackend,
        definition: &LeaseDefinition,
    ) -> BackendResult<LeaseRegisterDefinitionResult> {
        let mut tx = backend.begin()?;
        let result = self.register_definition_tx(&mut tx, definition)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn register_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        definition: &LeaseDefinition,
    ) -> BackendResult<LeaseRegisterDefinitionResult> {
        let existing = self.inspect_definition_tx(tx, &definition.id)?;
        tx.put(
            DEFINITIONS,
            &lease_definition_key(&definition.id),
            memory_codec::lease_definition_to_json(definition),
        )?;
        Ok(LeaseRegisterDefinitionResult {
            registered_new: existing.is_none(),
            definition: definition.clone(),
        })
    }

    fn inspect_definition(
        &self,
        backend: &InMemoryBackend,
        definition_id: &LeaseDefinitionId,
    ) -> BackendResult<Option<LeaseDefinition>> {
        let mut tx = backend.begin()?;
        let result = self.inspect_definition_tx(&mut tx, definition_id)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        definition_id: &LeaseDefinitionId,
    ) -> BackendResult<Option<LeaseDefinition>> {
        let key = lease_definition_key(definition_id);
        Ok(tx
            .get(DEFINITIONS, &key)?
            .map(|record| memory_codec::lease_definition_from_json(&record.document)))
    }

    fn acquire(
        &self,
        backend: &InMemoryBackend,
        request: &LeaseAcquireRequest,
    ) -> BackendResult<LeaseAcquireResult> {
        let mut tx = backend.begin()?;
        let result = self.acquire_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn acquire_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &LeaseAcquireRequest,
    ) -> BackendResult<LeaseAcquireResult> {
        let definition = self
            .inspect_definition_tx(tx, &request.definition_id)?
            .ok_or_else(|| BackendError::NotFound {
                message: "unknown lease definition".to_string(),
            })?;
        let existing = self.inspect_tx(
            tx,
            &LeaseInspectRequest {
                definition_id: request.definition_id.clone(),
                resource: request.resource.clone(),
            },
        )?;
        if let Some(existing) = &existing {
            if existing.holder.as_deref() != Some(&request.holder)
                && existing.expires_at > request.now
            {
                return Ok(LeaseAcquireResult {
                    acquired: false,
                    lease: Some(existing.clone()),
                });
            }
        }
        let mut token = existing
            .as_ref()
            .map(|lease| lease.fencing_token)
            .unwrap_or(0);
        if definition.fencing_token {
            token += 1;
        }
        let record = LeaseRecord {
            definition_id: request.definition_id.clone(),
            resource: request.resource.clone(),
            holder: Some(request.holder.clone()),
            expires_at: request.now + definition.ttl,
            fencing_token: token,
        };
        let key = lease_key(&request.definition_id, &request.resource);
        tx.put(LEASES, &key, memory_codec::lease_record_to_json(&record))?;
        Ok(LeaseAcquireResult {
            acquired: true,
            lease: Some(record),
        })
    }

    fn renew(
        &self,
        backend: &InMemoryBackend,
        request: &LeaseRenewRequest,
    ) -> BackendResult<LeaseRecord> {
        let mut tx = backend.begin()?;
        let result = self.renew_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn renew_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &LeaseRenewRequest,
    ) -> BackendResult<LeaseRecord> {
        let definition = self
            .inspect_definition_tx(tx, &request.definition_id)?
            .ok_or_else(|| BackendError::NotFound {
                message: "unknown lease definition".to_string(),
            })?;
        let mut record = self.require_lease(tx, &request.definition_id, &request.resource)?;
        require_holder(&record, &request.holder, request.fencing_token)?;
        record.expires_at = request.now + definition.ttl;
        tx.put(
            LEASES,
            &lease_key(&request.definition_id, &request.resource),
            memory_codec::lease_record_to_json(&record),
        )?;
        Ok(record)
    }

    fn release(
        &self,
        backend: &InMemoryBackend,
        request: &LeaseReleaseRequest,
    ) -> BackendResult<()> {
        let mut tx = backend.begin()?;
        self.release_tx(&mut tx, request)?;
        backend.commit(tx)
    }

    fn release_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &LeaseReleaseRequest,
    ) -> BackendResult<()> {
        let record = self.require_lease(tx, &request.definition_id, &request.resource)?;
        require_holder(&record, &request.holder, request.fencing_token)?;
        let key = lease_key(&request.definition_id, &request.resource);
        tx.erase(LEASES, &key)?;
        Ok(())
    }

    fn inspect(
        &self,
        backend: &InMemoryBackend,
        request: &LeaseInspectRequest,
    ) -> BackendResult<Option<LeaseRecord>> {
        let mut tx = backend.begin()?;
        let result = self.inspect_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &LeaseInspectRequest,
    ) -> BackendResult<Option<LeaseRecord>> {
        let key = lease_key(&request.definition_id, &request.resource);
        Ok(tx
            .get(LEASES, &key)?
            .map(|record| memory_codec::lease_record_from_json(&record.document)))
    }
}

fn lease_definition_key(id: &LeaseDefinitionId) -> String {
    definition_key(&[id.name.clone(), id.version.to_string()])
}

fn lease_key(id: &LeaseDefinitionId, resource: &str) -> String {
    definition_key(&[
        id.name.clone(),
        id.version.to_string(),
        resource.to_string(),
    ])
}

fn require_holder(record: &LeaseRecord, holder: &str, fencing_token: u64) -> BackendResult<()> {
    if record.holder.as_deref() != Some(holder) || record.fencing_token != fencing_token {
        return Err(BackendError::Conflict {
            kind: ConflictKind::LeaseConflict,
            message: "lease is not held by caller".to_string(),
        });
    }
    Ok(())
}
