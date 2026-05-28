use std::sync::Arc;

use crate::backend::{Backend, BackendResult, Query, Transaction, VersionedRecord};
use crate::json::Json;
use crate::runtime_entity_gc_repository::{
    EntityGcEligibilityRequest, EntityGcFinalizeRequest, EntityGcRepository,
};
use crate::runtime_entity_gc_types::EntityGcDescriptor;
use crate::runtime_entity_gc_workers::{EntityGcWorker, EntityGcWorkerConfig};

#[derive(Debug, Clone, Default)]
pub struct DefaultEntityGcRepository;

impl<Tx: Transaction> EntityGcRepository<Tx> for DefaultEntityGcRepository {
    fn list_gc_eligible_tx(
        &self,
        tx: &mut Tx,
        request: &EntityGcEligibilityRequest,
    ) -> BackendResult<Vec<VersionedRecord>> {
        let limit = if request.limit == 0 { 100 } else { request.limit };
        let mut eligible = Vec::new();
        for record in tx.query(&request.descriptor.collection, &Query::All)? {
            if eligible.len() >= limit {
                break;
            }
            let Some(Json::String(status)) = record.document.find(&request.descriptor.status_field)
            else {
                continue;
            };
            if is_terminal_gc_state(&request.descriptor, status) {
                eligible.push(record);
            }
        }
        Ok(eligible)
    }

    fn finalize_gc_tx(
        &self,
        tx: &mut Tx,
        request: &EntityGcFinalizeRequest,
    ) -> BackendResult<()> {
        tx.erase(&request.descriptor.collection, &request.key)
    }
}

pub type EntityGcTask = Arc<dyn Fn(String) -> BackendResult<()> + Send + Sync>;

pub fn register_entity_gc_workers<B, F>(
    mut register: F,
    backend: Arc<B>,
    descriptors: Vec<EntityGcDescriptor>,
) -> BackendResult<()>
where
    B: Backend + Send + Sync + 'static,
    F: FnMut(EntityGcTask) -> BackendResult<()>,
{
    for descriptor in descriptors {
        let backend = Arc::clone(&backend);
        let worker = EntityGcWorker::new(
            descriptor,
            DefaultEntityGcRepository,
            EntityGcWorkerConfig::default(),
        );
        register(Arc::new(move |now| {
            worker.run_once(backend.as_ref(), &now).map(|_| ())
        }))?;
    }
    Ok(())
}

fn is_terminal_gc_state(descriptor: &EntityGcDescriptor, status: &str) -> bool {
    descriptor
        .terminal_states
        .iter()
        .any(|terminal| terminal.state == status)
}
