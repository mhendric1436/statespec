use crate::backend::{Backend, BackendResult};
use crate::memory_backend::{lock_error, InMemoryBackend};
use crate::metric::{MetricDefinition, MetricDefinitionRegistration, MetricSample, MetricSink};

#[derive(Debug, Clone, Default)]
pub struct InMemoryMetricSink;

impl InMemoryMetricSink {
    pub fn new() -> Self {
        Self
    }

    pub fn inspect_samples(&self, backend: &InMemoryBackend) -> BackendResult<Vec<MetricSample>> {
        let mut tx = backend.begin()?;
        let samples = self.inspect_samples_tx(&mut tx)?;
        backend.commit(tx)?;
        Ok(samples)
    }

    pub fn inspect_samples_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
    ) -> BackendResult<Vec<MetricSample>> {
        let mut samples = tx.state.lock().map_err(lock_error)?.metric_samples.clone();
        samples.extend(tx.metric_sample_appends.clone());
        Ok(samples)
    }
}

impl MetricSink<InMemoryBackend> for InMemoryMetricSink {
    fn register_definition(
        &self,
        backend: &InMemoryBackend,
        definition: &MetricDefinition,
    ) -> BackendResult<MetricDefinitionRegistration> {
        let mut tx = backend.begin()?;
        let result = self.register_definition_tx(&mut tx, definition)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn register_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        definition: &MetricDefinition,
    ) -> BackendResult<MetricDefinitionRegistration> {
        let existing = self.inspect_definition_tx(tx, &definition.name)?;
        tx.metric_definition_puts
            .insert(definition.name.clone(), definition.clone());
        Ok(MetricDefinitionRegistration {
            registered_new: existing.is_none(),
            definition: definition.clone(),
        })
    }

    fn inspect_definition(
        &self,
        backend: &InMemoryBackend,
        name: &str,
    ) -> BackendResult<Option<MetricDefinition>> {
        let mut tx = backend.begin()?;
        let result = self.inspect_definition_tx(&mut tx, name)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        name: &str,
    ) -> BackendResult<Option<MetricDefinition>> {
        if let Some(definition) = tx.metric_definition_puts.get(name) {
            return Ok(Some(definition.clone()));
        }
        Ok(tx
            .state
            .lock()
            .map_err(lock_error)?
            .metric_definitions
            .get(name)
            .cloned())
    }

    fn record_metric(&self, backend: &InMemoryBackend, sample: &MetricSample) -> BackendResult<()> {
        let mut tx = backend.begin()?;
        self.record_metric_tx(&mut tx, sample)?;
        backend.commit(tx)
    }

    fn record_metric_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        sample: &MetricSample,
    ) -> BackendResult<()> {
        tx.metric_sample_appends.push(sample.clone());
        Ok(())
    }
}
