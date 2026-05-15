use std::collections::BTreeMap;

use crate::backend::{Backend, BackendResult};
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

pub trait MetricSink<B: Backend> {
    fn record_metric(&self, backend: &B, sample: &MetricSample) -> BackendResult<()>;

    fn record_metric_tx(&self, tx: &mut B::Tx, sample: &MetricSample) -> BackendResult<()>;
}
