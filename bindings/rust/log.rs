use std::collections::BTreeMap;

use crate::backend::{Backend, BackendResult};
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

pub trait LogSink<B: Backend> {
    fn emit_log(&self, backend: &B, event: &LogEvent) -> BackendResult<()>;

    fn emit_log_tx(&self, tx: &mut B::Tx, event: &LogEvent) -> BackendResult<()>;
}
