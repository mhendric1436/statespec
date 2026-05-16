use std::collections::BTreeMap;

use crate::backend::{Backend, BackendResult, FieldDescriptor};
use crate::json::Json;

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum LogLevel {
    Debug,
    Info,
    Warn,
    Error,
}

#[derive(Debug, Clone, PartialEq)]
pub struct LogEvent {
    pub name: String,
    pub level: LogLevel,
    pub event_name: String,
    pub fields: BTreeMap<String, Json>,
}

#[derive(Debug, Clone)]
pub struct LogDefinition {
    pub name: String,
    pub level: LogLevel,
    pub event_name: String,
    pub fields: Vec<FieldDescriptor>,
}

#[derive(Debug, Clone)]
pub struct LogDefinitionRegistration {
    pub registered_new: bool,
    pub definition: LogDefinition,
}

pub trait LogSink<B: Backend> {
    fn register_definition(
        &self,
        backend: &B,
        definition: &LogDefinition,
    ) -> BackendResult<LogDefinitionRegistration>;

    fn register_definition_tx(
        &self,
        tx: &mut B::Tx,
        definition: &LogDefinition,
    ) -> BackendResult<LogDefinitionRegistration>;

    fn inspect_definition(&self, backend: &B, name: &str) -> BackendResult<Option<LogDefinition>>;

    fn inspect_definition_tx(
        &self,
        tx: &mut B::Tx,
        name: &str,
    ) -> BackendResult<Option<LogDefinition>>;

    /// Transactional emits are staged in the caller's OCC transaction. Commit
    /// makes the log visible to exporters; rollback drops it.
    fn emit_log(&self, backend: &B, event: &LogEvent) -> BackendResult<()>;

    fn emit_log_tx(&self, tx: &mut B::Tx, event: &LogEvent) -> BackendResult<()>;
}
