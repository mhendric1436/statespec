use std::sync::{Arc, Mutex};

use crate::backend::{
    Backend, BackendCapabilities, BackendError, BackendResult, CollectionDescriptor, ConflictKind,
    Query, Transaction, VersionedRecord,
};
use crate::json::Json;
use crate::memory_transaction::{
    record_version_key, version_or_zero, InMemoryState, InMemoryTransaction,
};

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
        self.state
            .lock()
            .map_err(lock_error)?
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
        let version_key = record_version_key(collection, key);
        if tx.erases.contains(&version_key) {
            return Ok(None);
        }
        if let Some(record) = tx.puts.get(&version_key) {
            return Ok(Some(record.clone()));
        }

        let state = tx.state.lock().map_err(lock_error)?;
        let record = state
            .records
            .get(collection)
            .and_then(|records| records.get(key))
            .cloned();
        tx.read_versions.insert(
            version_key,
            record.as_ref().map(|record| record.version).unwrap_or(0),
        );
        Ok(record)
    }

    fn query(
        &self,
        tx: &mut Self::Tx,
        collection: &str,
        query: &Query,
    ) -> BackendResult<Vec<VersionedRecord>> {
        let mut by_key = {
            let state = tx.state.lock().map_err(lock_error)?;
            state.records.get(collection).cloned().unwrap_or_default()
        };
        for record in tx.puts.values() {
            if record.collection == collection {
                by_key.insert(record.key.clone(), record.clone());
            }
        }
        let prefix = format!("record:{collection}:");
        for version_key in &tx.erases {
            if let Some(key) = version_key.strip_prefix(&prefix) {
                by_key.remove(key);
            }
        }

        let mut matched = Vec::new();
        for record in by_key.values() {
            if matches_query(record, query) {
                tx.read_versions.insert(
                    record_version_key(&record.collection, &record.key),
                    record.version,
                );
                matched.push(record.clone());
            }
        }
        Ok(matched)
    }

    fn put(
        &self,
        tx: &mut Self::Tx,
        collection: &str,
        key: &str,
        document: Json,
    ) -> BackendResult<()> {
        let _ = self.get(tx, collection, key)?;
        let version_key = record_version_key(collection, key);
        tx.puts.insert(
            version_key.clone(),
            VersionedRecord {
                collection: collection.to_string(),
                key: key.to_string(),
                version: 0,
                document,
            },
        );
        tx.erases.remove(&version_key);
        Ok(())
    }

    fn erase(&self, tx: &mut Self::Tx, collection: &str, key: &str) -> BackendResult<()> {
        let _ = self.get(tx, collection, key)?;
        let version_key = record_version_key(collection, key);
        tx.puts.remove(&version_key);
        tx.erases.insert(version_key);
        Ok(())
    }

    fn commit(&self, mut tx: Self::Tx) -> BackendResult<()> {
        let mut state = tx.state.lock().map_err(lock_error)?;
        for (version_key, expected_version) in &tx.read_versions {
            if version_or_zero(&state.versions, version_key) != *expected_version {
                return Err(BackendError::Conflict {
                    kind: ConflictKind::VersionConflict,
                    message: "in-memory OCC conflict".to_string(),
                });
            }
        }
        for version_key in &tx.erases {
            erase_record(&mut state, version_key);
            *state.versions.entry(version_key.clone()).or_insert(0) += 1;
        }
        for (version_key, record) in &tx.puts {
            let version = state.versions.entry(version_key.clone()).or_insert(0);
            *version += 1;
            let mut committed = record.clone();
            committed.version = *version;
            state
                .records
                .entry(committed.collection.clone())
                .or_default()
                .insert(committed.key.clone(), committed);
        }

        state
            .feature_flag_definitions
            .append(&mut tx.feature_flag_definition_puts);
        state
            .feature_flag_values
            .append(&mut tx.feature_flag_value_puts);
        state
            .queue_definitions
            .append(&mut tx.queue_definition_puts);
        state.queue_messages.append(&mut tx.queue_message_puts);
        state
            .queue_idempotency_keys
            .append(&mut tx.queue_idempotency_puts);
        state
            .lease_definitions
            .append(&mut tx.lease_definition_puts);
        for key in &tx.lease_erases {
            state.leases.remove(key);
        }
        state.leases.append(&mut tx.lease_puts);
        state
            .workflow_definitions
            .append(&mut tx.workflow_definition_puts);
        state
            .workflow_executions
            .append(&mut tx.workflow_execution_puts);
        state.log_definitions.append(&mut tx.log_definition_puts);
        state.log_events.append(&mut tx.log_event_appends);
        state
            .metric_definitions
            .append(&mut tx.metric_definition_puts);
        state.metric_samples.append(&mut tx.metric_sample_appends);
        drop(state);
        tx.abort()
    }
}

fn matches_query(record: &VersionedRecord, query: &Query) -> bool {
    match query {
        Query::All => true,
        Query::KeyPrefix { prefix } => record.key.starts_with(prefix),
        Query::JsonEquals { path, value } => find_json_path(&record.document, path)
            .map(|found| found == value)
            .unwrap_or(false),
        _ => true,
    }
}

fn find_json_path<'a>(document: &'a Json, path: &str) -> Option<&'a Json> {
    let mut current = document;
    for segment in path.split('.') {
        current = current.find(segment)?;
    }
    Some(current)
}

fn erase_record(state: &mut InMemoryState, version_key: &str) {
    let rest = match version_key.strip_prefix("record:") {
        Some(rest) => rest,
        None => return,
    };
    let Some((collection, key)) = rest.split_once(':') else {
        return;
    };
    if let Some(records) = state.records.get_mut(collection) {
        records.remove(key);
    }
}

pub(crate) fn lock_error<T>(_: std::sync::PoisonError<T>) -> BackendError {
    BackendError::Internal {
        message: "in-memory backend lock poisoned".to_string(),
    }
}
