use std::sync::{Arc, Mutex};

use crate::backend::{
    Backend, BackendCapabilities, BackendResult, CollectionDescriptor, Query, Transaction,
    VersionedRecord,
};
use crate::json::Json;
use crate::memory_transaction::{lock_error, InMemoryState, InMemoryTransaction};
use crate::schema_compatibility::validate_collection_descriptor_upgrade;

#[derive(Debug, Clone, Default)]
pub struct InMemoryBackend {
    state: Arc<Mutex<InMemoryState>>,
}

impl InMemoryBackend {
    pub fn new() -> Self {
        Self::default()
    }
}

impl Backend for InMemoryBackend {
    type Tx = InMemoryTransaction;

    fn capabilities(&self) -> BackendCapabilities {
        BackendCapabilities {
            transactions: true,
            compare_and_swap: true,
            prefix_query: true,
            ..BackendCapabilities::default()
        }
    }

    fn ensure_collection(&self, descriptor: &CollectionDescriptor) -> BackendResult<()> {
        let mut state = self.state.lock().map_err(lock_error)?;
        if let Some(existing) = state.collections.get(&descriptor.name) {
            validate_collection_descriptor_upgrade(existing, descriptor)?;
        }
        state
            .collections
            .insert(descriptor.name.clone(), descriptor.clone());
        Ok(())
    }

    fn ensure_collections(&self, descriptors: &[CollectionDescriptor]) -> BackendResult<()> {
        for descriptor in descriptors {
            self.ensure_collection(descriptor)?;
        }
        Ok(())
    }

    fn begin(&self) -> BackendResult<Self::Tx> {
        Ok(InMemoryTransaction::new(self.state.clone()))
    }

    fn get(
        &self,
        tx: &mut Self::Tx,
        collection: &str,
        key: &str,
    ) -> BackendResult<Option<VersionedRecord>> {
        tx.get(collection, key)
    }

    fn query(
        &self,
        tx: &mut Self::Tx,
        collection: &str,
        query: &Query,
    ) -> BackendResult<Vec<VersionedRecord>> {
        tx.query(collection, query)
    }

    fn put(
        &self,
        tx: &mut Self::Tx,
        collection: &str,
        key: &str,
        document: Json,
    ) -> BackendResult<()> {
        tx.put(collection, key, document)
    }

    fn erase(&self, tx: &mut Self::Tx, collection: &str, key: &str) -> BackendResult<()> {
        tx.erase(collection, key)
    }

    fn commit(&self, mut tx: Self::Tx) -> BackendResult<()> {
        tx.commit()
    }
}
