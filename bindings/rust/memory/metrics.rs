use crate::backend::{Backend, BackendResult, Query, Transaction};
use crate::memory_codec;
use crate::metric::{MetricDefinition, MetricDefinitionRegistration, MetricSample, MetricSink};

const DEFINITIONS: &str = "metrics.definitions";
const SAMPLES: &str = "metrics.samples";

#[derive(Debug, Clone, Default)]
pub struct InMemoryMetricSink;

impl InMemoryMetricSink {
    pub fn new() -> Self {
        Self
    }

    pub fn inspect_samples<B: Backend>(&self, backend: &B) -> BackendResult<Vec<MetricSample>> {
        let mut tx = backend.begin()?;
        let samples = self.inspect_samples_tx::<B>(&mut tx)?;
        backend.commit(tx)?;
        Ok(samples)
    }

    pub fn inspect_samples_tx<B: Backend>(
        &self,
        tx: &mut B::Tx,
    ) -> BackendResult<Vec<MetricSample>> {
        Ok(tx
            .query(SAMPLES, &Query::All)?
            .iter()
            .map(|record| memory_codec::metric_sample_from_json(&record.document))
            .collect())
    }
}

impl<B: Backend> MetricSink<B> for InMemoryMetricSink {
    fn register_definition(
        &self,
        backend: &B,
        definition: &MetricDefinition,
    ) -> BackendResult<MetricDefinitionRegistration> {
        let mut tx = backend.begin()?;
        let result = <InMemoryMetricSink as MetricSink<B>>::register_definition_tx(
            self, &mut tx, definition,
        )?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn register_definition_tx(
        &self,
        tx: &mut B::Tx,
        definition: &MetricDefinition,
    ) -> BackendResult<MetricDefinitionRegistration> {
        let existing = <InMemoryMetricSink as MetricSink<B>>::inspect_definition_tx(
            self,
            tx,
            &definition.name,
        )?;
        tx.put(
            DEFINITIONS,
            &definition.name,
            memory_codec::metric_definition_to_json(definition),
        )?;
        Ok(MetricDefinitionRegistration {
            registered_new: existing.is_none(),
            definition: definition.clone(),
        })
    }

    fn inspect_definition(
        &self,
        backend: &B,
        name: &str,
    ) -> BackendResult<Option<MetricDefinition>> {
        let mut tx = backend.begin()?;
        let result =
            <InMemoryMetricSink as MetricSink<B>>::inspect_definition_tx(self, &mut tx, name)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_definition_tx(
        &self,
        tx: &mut B::Tx,
        name: &str,
    ) -> BackendResult<Option<MetricDefinition>> {
        Ok(tx
            .get(DEFINITIONS, name)?
            .map(|record| memory_codec::metric_definition_from_json(&record.document)))
    }

    fn record_metric(&self, backend: &B, sample: &MetricSample) -> BackendResult<()> {
        let mut tx = backend.begin()?;
        <InMemoryMetricSink as MetricSink<B>>::record_metric_tx(self, &mut tx, sample)?;
        backend.commit(tx)
    }

    fn record_metric_tx(&self, tx: &mut B::Tx, sample: &MetricSample) -> BackendResult<()> {
        let key = format!("sample:{:020}", tx.query(SAMPLES, &Query::All)?.len() + 1);
        tx.put(SAMPLES, &key, memory_codec::metric_sample_to_json(sample))?;
        Ok(())
    }
}
