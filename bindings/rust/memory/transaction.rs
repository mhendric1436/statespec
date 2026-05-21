use std::collections::{BTreeMap, BTreeSet};
use std::sync::{Arc, Mutex};

use crate::backend::{
    BackendError, BackendResult, CollectionDescriptor, ConflictKind, Query, Transaction, Version,
    VersionedRecord,
};
use crate::json::Json;

#[derive(Debug, Default)]
pub struct InMemoryState {
    pub records: BTreeMap<String, BTreeMap<String, VersionedRecord>>,
    pub versions: BTreeMap<String, Version>,
    pub collections: BTreeMap<String, CollectionDescriptor>,
}

#[derive(Debug)]
pub struct InMemoryTransaction {
    pub(crate) state: Arc<Mutex<InMemoryState>>,
    pub(crate) open: bool,
    pub(crate) read_versions: BTreeMap<String, Version>,
    pub(crate) puts: BTreeMap<String, VersionedRecord>,
    pub(crate) erases: BTreeSet<String>,
}

impl InMemoryTransaction {
    pub(crate) fn new(state: Arc<Mutex<InMemoryState>>) -> Self {
        Self {
            state,
            open: true,
            read_versions: BTreeMap::new(),
            puts: BTreeMap::new(),
            erases: BTreeSet::new(),
        }
    }

    fn require_open(&self) -> BackendResult<()> {
        if self.open {
            Ok(())
        } else {
            Err(BackendError::Internal {
                message: "expected open in-memory transaction".to_string(),
            })
        }
    }

    pub(crate) fn commit(&mut self) -> BackendResult<()> {
        self.require_open()?;
        let mut state = self.state.lock().map_err(lock_error)?;
        for (version_key, expected_version) in &self.read_versions {
            if version_or_zero(&state.versions, version_key) != *expected_version {
                return Err(BackendError::Conflict {
                    kind: ConflictKind::VersionConflict,
                    message: "in-memory OCC conflict".to_string(),
                });
            }
        }
        for version_key in &self.erases {
            erase_record(&mut state, version_key);
            *state.versions.entry(version_key.clone()).or_insert(0) += 1;
        }
        for (version_key, record) in &self.puts {
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
        drop(state);
        self.abort()
    }
}

impl Transaction for InMemoryTransaction {
    fn is_open(&self) -> bool {
        self.open
    }

    fn abort(&mut self) -> BackendResult<()> {
        self.open = false;
        self.read_versions.clear();
        self.puts.clear();
        self.erases.clear();
        Ok(())
    }

    fn get(&mut self, collection: &str, key: &str) -> BackendResult<Option<VersionedRecord>> {
        self.require_open()?;
        let version_key = record_version_key(collection, key);
        if self.erases.contains(&version_key) {
            return Ok(None);
        }
        if let Some(record) = self.puts.get(&version_key) {
            return Ok(Some(record.clone()));
        }

        let state = self.state.lock().map_err(lock_error)?;
        let record = state
            .records
            .get(collection)
            .and_then(|records| records.get(key))
            .cloned();
        self.read_versions.insert(
            version_key,
            record.as_ref().map(|record| record.version).unwrap_or(0),
        );
        Ok(record)
    }

    fn query(&mut self, collection: &str, query: &Query) -> BackendResult<Vec<VersionedRecord>> {
        self.require_open()?;
        let mut by_key = {
            let state = self.state.lock().map_err(lock_error)?;
            state.records.get(collection).cloned().unwrap_or_default()
        };
        for record in self.puts.values() {
            if record.collection == collection {
                by_key.insert(record.key.clone(), record.clone());
            }
        }
        let prefix = format!("record:{collection}:");
        for version_key in &self.erases {
            if let Some(key) = version_key.strip_prefix(&prefix) {
                by_key.remove(key);
            }
        }

        let mut matched = Vec::new();
        for record in by_key.values() {
            if matches_query(record, query) {
                self.read_versions.insert(
                    record_version_key(&record.collection, &record.key),
                    record.version,
                );
                matched.push(record.clone());
            }
        }
        Ok(matched)
    }

    fn put(&mut self, collection: &str, key: &str, document: Json) -> BackendResult<()> {
        self.require_open()?;
        let _ = self.get(collection, key)?;
        let version_key = record_version_key(collection, key);
        self.puts.insert(
            version_key.clone(),
            VersionedRecord {
                collection: collection.to_string(),
                key: key.to_string(),
                version: 0,
                document,
            },
        );
        self.erases.remove(&version_key);
        Ok(())
    }

    fn erase(&mut self, collection: &str, key: &str) -> BackendResult<()> {
        self.require_open()?;
        let _ = self.get(collection, key)?;
        let version_key = record_version_key(collection, key);
        self.puts.remove(&version_key);
        self.erases.insert(version_key);
        Ok(())
    }
}

pub(crate) fn record_version_key(collection: &str, key: &str) -> String {
    format!("record:{collection}:{key}")
}

pub(crate) fn definition_key(parts: &[String]) -> String {
    parts.join(":")
}

pub(crate) fn version_or_zero(versions: &BTreeMap<String, Version>, key: &str) -> Version {
    versions.get(key).copied().unwrap_or(0)
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
