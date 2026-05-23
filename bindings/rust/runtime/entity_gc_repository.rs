use crate::backend::{BackendResult, Key, Transaction, VersionedRecord};
use crate::runtime_entity_gc_descriptors::EntityGcDescriptor;

pub struct EntityGcEligibilityRequest {
    pub descriptor: EntityGcDescriptor,
    pub now: String,
    pub limit: usize,
}

pub struct EntityGcFinalizeRequest {
    pub descriptor: EntityGcDescriptor,
    pub key: Key,
    pub now: String,
}

pub trait EntityGcRepository<Tx: Transaction> {
    fn list_gc_eligible_tx(
        &self,
        tx: &mut Tx,
        request: &EntityGcEligibilityRequest,
    ) -> BackendResult<Vec<VersionedRecord>>;

    fn finalize_gc_tx(
        &self,
        tx: &mut Tx,
        request: &EntityGcFinalizeRequest,
    ) -> BackendResult<()>;
}
