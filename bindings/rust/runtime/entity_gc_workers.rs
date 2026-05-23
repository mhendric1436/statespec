use crate::backend::{Backend, BackendResult};
use crate::runtime_entity_gc_descriptors::EntityGcDescriptor;
use crate::runtime_entity_gc_repository::{
    EntityGcEligibilityRequest, EntityGcFinalizeRequest, EntityGcRepository,
};

#[derive(Debug, Clone)]
pub struct EntityGcWorkerConfig {
    pub batch_limit: usize,
}

impl Default for EntityGcWorkerConfig {
    fn default() -> Self {
        Self { batch_limit: 100 }
    }
}

#[derive(Debug, Clone, Default)]
pub struct EntityGcRunResult {
    pub scanned: usize,
    pub finalized: usize,
}

pub struct EntityGcWorker<R> {
    pub descriptor: EntityGcDescriptor,
    pub repository: R,
    pub config: EntityGcWorkerConfig,
}

impl<R> EntityGcWorker<R> {
    pub fn new(
        descriptor: EntityGcDescriptor,
        repository: R,
        config: EntityGcWorkerConfig,
    ) -> Self {
        Self {
            descriptor,
            repository,
            config,
        }
    }
}

impl<R> EntityGcWorker<R> {
    pub fn run_once<B: Backend>(
        &self,
        backend: &B,
        now: &str,
    ) -> BackendResult<EntityGcRunResult>
    where
        R: EntityGcRepository<B::Tx>,
    {
        let mut tx = backend.begin()?;
        let result = self.run_once_tx::<B>(&mut tx, now)?;
        backend.commit(tx)?;
        Ok(result)
    }

    pub fn run_once_tx<B: Backend>(
        &self,
        tx: &mut B::Tx,
        now: &str,
    ) -> BackendResult<EntityGcRunResult>
    where
        R: EntityGcRepository<B::Tx>,
    {
        let records = self.repository.list_gc_eligible_tx(
            tx,
            &EntityGcEligibilityRequest {
                descriptor: self.descriptor.clone(),
                now: now.to_string(),
                limit: self.config.batch_limit,
            },
        )?;
        let scanned = records.len();
        let mut finalized = 0;
        for record in records {
            self.repository.finalize_gc_tx(
                tx,
                &EntityGcFinalizeRequest {
                    descriptor: self.descriptor.clone(),
                    key: record.key,
                    now: now.to_string(),
                },
            )?;
            finalized += 1;
        }
        Ok(EntityGcRunResult { scanned, finalized })
    }
}
