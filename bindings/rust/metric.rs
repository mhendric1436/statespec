use std::collections::BTreeMap;

use crate::backend::{Backend, BackendResult, FieldDescriptor};
use crate::json::Json;

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum MetricKind {
    Counter,
    Gauge,
    Histogram,
}

#[derive(Debug, Clone, PartialEq)]
pub struct MetricSample {
    pub name: String,
    pub kind: MetricKind,
    pub backend_name: String,
    pub value: f64,
    pub unit: String,
    pub labels: BTreeMap<String, Json>,
}

#[derive(Debug, Clone)]
pub struct MetricDefinition {
    pub name: String,
    pub kind: MetricKind,
    pub backend_name: String,
    pub unit: String,
    pub labels: Vec<FieldDescriptor>,
}

#[derive(Debug, Clone)]
pub struct MetricDefinitionRegistration {
    pub registered_new: bool,
    pub definition: MetricDefinition,
}

pub trait MetricSink<B: Backend> {
    fn register_definition(
        &self,
        backend: &B,
        definition: &MetricDefinition,
    ) -> BackendResult<MetricDefinitionRegistration>;

    fn register_definition_tx(
        &self,
        tx: &mut B::Tx,
        definition: &MetricDefinition,
    ) -> BackendResult<MetricDefinitionRegistration>;

    /// Transactional records are staged in the caller's OCC transaction. Commit
    /// makes the sample visible to exporters; rollback drops it. Implementations
    /// should validate labels against the registered metric definition.
    fn record_metric(&self, backend: &B, sample: &MetricSample) -> BackendResult<()>;

    fn record_metric_tx(&self, tx: &mut B::Tx, sample: &MetricSample) -> BackendResult<()>;
}
