use std::collections::{BTreeMap, BTreeSet};
use std::sync::{Arc, Mutex};

use crate::backend::{
    BackendError, BackendResult, CollectionDescriptor, ConflictKind, IndexBound, IndexDescriptor,
    IndexValue, Query, Transaction, Version, VersionedRecord,
};
use crate::json::Json;

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct InMemoryIndexKey(pub Vec<String>);

#[derive(Debug, Clone)]
pub struct InMemoryIndexState {
    pub descriptor: crate::backend::IndexDescriptor,
    pub entries: BTreeMap<InMemoryIndexKey, BTreeSet<String>>,
}

#[derive(Debug, Default)]
pub struct InMemoryState {
    pub records: BTreeMap<String, BTreeMap<String, VersionedRecord>>,
    pub versions: BTreeMap<String, Version>,
    pub collections: BTreeMap<String, CollectionDescriptor>,
    pub indexes: BTreeMap<String, BTreeMap<String, InMemoryIndexState>>,
}

pub(crate) fn empty_index_states(
    descriptor: &CollectionDescriptor,
) -> BTreeMap<String, InMemoryIndexState> {
    descriptor
        .indexes
        .iter()
        .map(|index| {
            (
                index.name.clone(),
                InMemoryIndexState {
                    descriptor: index.clone(),
                    entries: BTreeMap::new(),
                },
            )
        })
        .collect()
}

pub(crate) fn extract_index_key(
    document: &Json,
    descriptor: &IndexDescriptor,
) -> BackendResult<InMemoryIndexKey> {
    descriptor
        .fields
        .iter()
        .map(|field| encode_index_value(document.find(field)))
        .collect::<BackendResult<Vec<_>>>()
        .map(InMemoryIndexKey)
}

pub(crate) fn encode_index_value(value: Option<&Json>) -> BackendResult<String> {
    match value {
        None => Ok("missing:".to_string()),
        Some(Json::Null) => Ok("null:".to_string()),
        Some(Json::Bool(value)) => Ok(format!("bool:{value}")),
        Some(Json::Integer(value)) => Ok(format!("int:{value}")),
        Some(Json::Decimal(value)) => Ok(format!(
            "decimal:{}",
            Json::Decimal(*value).canonical_string()
        )),
        Some(Json::String(value)) => Ok(format!("string:{value}")),
        Some(Json::Array(_)) | Some(Json::Object(_)) => Err(BackendError::InvalidSchema {
            message: "in-memory index fields must resolve to scalar JSON values".to_string(),
        }),
    }
}

pub(crate) fn extract_index_key_from_values(values: &[IndexValue]) -> InMemoryIndexKey {
    InMemoryIndexKey(values.iter().map(encode_query_index_value).collect())
}

fn encode_query_index_value(value: &IndexValue) -> String {
    match value {
        IndexValue::Null => "null:".to_string(),
        IndexValue::String(value) | IndexValue::Timestamp(value) => format!("string:{value}"),
        IndexValue::Integer(value) => format!("int:{value}"),
        IndexValue::Decimal(value) => {
            format!("decimal:{}", Json::Decimal(*value).canonical_string())
        }
        IndexValue::Boolean(value) => format!("bool:{value}"),
    }
}

fn index_key_starts_with(key: &InMemoryIndexKey, prefix: &InMemoryIndexKey) -> bool {
    key.0.len() >= prefix.0.len() && key.0[..prefix.0.len()] == prefix.0
}

fn index_key_in_range(
    key: &InMemoryIndexKey,
    lower: &Option<IndexBound>,
    upper: &Option<IndexBound>,
) -> bool {
    if let Some(lower) = lower {
        let lower_key = extract_index_key_from_values(&lower.values);
        if if lower.inclusive {
            key < &lower_key
        } else {
            key <= &lower_key
        } {
            return false;
        }
    }
    if let Some(upper) = upper {
        let upper_key = extract_index_key_from_values(&upper.values);
        if if upper.inclusive {
            key > &upper_key
        } else {
            key >= &upper_key
        } {
            return false;
        }
    }
    true
}

fn remove_record_from_indexes(
    indexes: &mut BTreeMap<String, BTreeMap<String, InMemoryIndexState>>,
    record: &VersionedRecord,
) -> BackendResult<()> {
    let Some(collection_indexes) = indexes.get_mut(&record.collection) else {
        return Ok(());
    };
    for index in collection_indexes.values_mut() {
        let key = extract_index_key(&record.document, &index.descriptor)?;
        let remove_entry = match index.entries.get_mut(&key) {
            Some(keys) => {
                keys.remove(&record.key);
                keys.is_empty()
            }
            None => false,
        };
        if remove_entry {
            index.entries.remove(&key);
        }
    }
    Ok(())
}

fn add_record_to_indexes(
    indexes: &mut BTreeMap<String, BTreeMap<String, InMemoryIndexState>>,
    record: &VersionedRecord,
) -> BackendResult<()> {
    let Some(collection_indexes) = indexes.get_mut(&record.collection) else {
        return Ok(());
    };
    for index in collection_indexes.values_mut() {
        let key = extract_index_key(&record.document, &index.descriptor)?;
        let keys = index.entries.entry(key).or_default();
        if index.descriptor.unique {
            for existing_key in keys.iter() {
                if existing_key != &record.key {
                    return Err(BackendError::Conflict {
                        kind: ConflictKind::UniqueIndexConflict,
                        message: format!(
                            "unique index conflict on collection '{}' index '{}'",
                            record.collection, index.descriptor.name
                        ),
                    });
                }
            }
        }
        keys.insert(record.key.clone());
    }
    Ok(())
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

        let mut staged_indexes = state.indexes.clone();
        for version_key in &self.erases {
            let Some((collection, key)) = parse_record_version_key(version_key) else {
                continue;
            };
            if let Some(record) = state
                .records
                .get(&collection)
                .and_then(|records| records.get(&key))
            {
                remove_record_from_indexes(&mut staged_indexes, record)?;
            }
        }
        for record in self.puts.values() {
            if let Some(existing) = state
                .records
                .get(&record.collection)
                .and_then(|records| records.get(&record.key))
            {
                remove_record_from_indexes(&mut staged_indexes, existing)?;
            }
            add_record_to_indexes(&mut staged_indexes, record)?;
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
        state.indexes = staged_indexes;
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
            if is_index_query(query) {
                self.committed_index_query_locked(&state, collection, query)?
                    .into_iter()
                    .map(|record| (record.key.clone(), record))
                    .collect()
            } else {
                state.records.get(collection).cloned().unwrap_or_default()
            }
        };
        for record in self.puts.values() {
            let matches = if is_index_query(query) {
                self.staged_record_matches_index_query(record, query)?
            } else {
                true
            };
            if record.collection == collection && matches {
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

impl InMemoryTransaction {
    fn committed_index_query_locked(
        &self,
        state: &InMemoryState,
        collection: &str,
        query: &Query,
    ) -> BackendResult<Vec<VersionedRecord>> {
        let Some(index_name) = query_index_name(query) else {
            return Ok(Vec::new());
        };
        let Some(index) = state
            .indexes
            .get(collection)
            .and_then(|indexes| indexes.get(index_name))
        else {
            return Ok(Vec::new());
        };
        let Some(collection_records) = state.records.get(collection) else {
            return Ok(Vec::new());
        };
        let mut records = Vec::new();
        for (index_key, keys) in &index.entries {
            if !matches_index_key(index_key, query) {
                continue;
            }
            for key in keys {
                if let Some(record) = collection_records.get(key) {
                    records.push(record.clone());
                }
            }
        }
        Ok(records)
    }

    fn staged_record_matches_index_query(
        &self,
        record: &VersionedRecord,
        query: &Query,
    ) -> BackendResult<bool> {
        let Some(index_name) = query_index_name(query) else {
            return Ok(false);
        };
        let state = self.state.lock().map_err(lock_error)?;
        let Some(index) = state
            .indexes
            .get(&record.collection)
            .and_then(|indexes| indexes.get(index_name))
        else {
            return Ok(false);
        };
        Ok(matches_index_key(
            &extract_index_key(&record.document, &index.descriptor)?,
            query,
        ))
    }
}

pub(crate) fn record_version_key(collection: &str, key: &str) -> String {
    format!("record:{collection}:{key}")
}

pub(crate) fn version_or_zero(versions: &BTreeMap<String, Version>, key: &str) -> Version {
    versions.get(key).copied().unwrap_or(0)
}

fn parse_record_version_key(version_key: &str) -> Option<(String, String)> {
    let rest = version_key.strip_prefix("record:")?;
    let (collection, key) = rest.split_once(':')?;
    Some((collection.to_string(), key.to_string()))
}

fn matches_query(record: &VersionedRecord, query: &Query) -> bool {
    match query {
        Query::All => true,
        Query::KeyPrefix { prefix } => record.key.starts_with(prefix),
        Query::JsonEquals { path, value } => find_json_path(&record.document, path)
            .map(|found| found == value)
            .unwrap_or(false),
        Query::IndexEquals { .. } | Query::IndexPrefix { .. } | Query::IndexRange { .. } => true,
    }
}

fn is_index_query(query: &Query) -> bool {
    matches!(
        query,
        Query::IndexEquals { .. } | Query::IndexPrefix { .. } | Query::IndexRange { .. }
    )
}

fn query_index_name(query: &Query) -> Option<&str> {
    match query {
        Query::IndexEquals { index_name, .. }
        | Query::IndexPrefix { index_name, .. }
        | Query::IndexRange { index_name, .. } => Some(index_name),
        _ => None,
    }
}

fn matches_index_key(key: &InMemoryIndexKey, query: &Query) -> bool {
    match query {
        Query::IndexEquals { values, .. } => {
            index_key_starts_with(key, &extract_index_key_from_values(values))
        }
        Query::IndexPrefix { prefix_values, .. } => {
            index_key_starts_with(key, &extract_index_key_from_values(prefix_values))
        }
        Query::IndexRange {
            lower_bound,
            upper_bound,
            ..
        } => index_key_in_range(key, lower_bound, upper_bound),
        _ => false,
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
    let Some((collection, key)) = parse_record_version_key(version_key) else {
        return;
    };
    if let Some(records) = state.records.get_mut(&collection) {
        records.remove(&key);
    }
}

pub(crate) fn lock_error<T>(_: std::sync::PoisonError<T>) -> BackendError {
    BackendError::Internal {
        message: "in-memory backend lock poisoned".to_string(),
    }
}
