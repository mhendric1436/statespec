use crate::backend::{Backend, BackendResult};
use crate::log::{LogDefinition, LogDefinitionRegistration, LogEvent, LogSink};
use crate::memory_backend::{lock_error, InMemoryBackend};

#[derive(Debug, Clone, Default)]
pub struct InMemoryLogSink;

impl InMemoryLogSink {
    pub fn new() -> Self {
        Self
    }

    pub fn inspect_events(&self, backend: &InMemoryBackend) -> BackendResult<Vec<LogEvent>> {
        let mut tx = backend.begin()?;
        let events = self.inspect_events_tx(&mut tx)?;
        backend.commit(tx)?;
        Ok(events)
    }

    pub fn inspect_events_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
    ) -> BackendResult<Vec<LogEvent>> {
        let mut events = tx.state.lock().map_err(lock_error)?.log_events.clone();
        events.extend(tx.log_event_appends.clone());
        Ok(events)
    }
}

impl LogSink<InMemoryBackend> for InMemoryLogSink {
    fn register_definition(
        &self,
        backend: &InMemoryBackend,
        definition: &LogDefinition,
    ) -> BackendResult<LogDefinitionRegistration> {
        let mut tx = backend.begin()?;
        let result = self.register_definition_tx(&mut tx, definition)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn register_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        definition: &LogDefinition,
    ) -> BackendResult<LogDefinitionRegistration> {
        let existing = self.inspect_definition_tx(tx, &definition.name)?;
        tx.log_definition_puts
            .insert(definition.name.clone(), definition.clone());
        Ok(LogDefinitionRegistration {
            registered_new: existing.is_none(),
            definition: definition.clone(),
        })
    }

    fn inspect_definition(
        &self,
        backend: &InMemoryBackend,
        name: &str,
    ) -> BackendResult<Option<LogDefinition>> {
        let mut tx = backend.begin()?;
        let result = self.inspect_definition_tx(&mut tx, name)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        name: &str,
    ) -> BackendResult<Option<LogDefinition>> {
        if let Some(definition) = tx.log_definition_puts.get(name) {
            return Ok(Some(definition.clone()));
        }
        Ok(tx
            .state
            .lock()
            .map_err(lock_error)?
            .log_definitions
            .get(name)
            .cloned())
    }

    fn emit_log(&self, backend: &InMemoryBackend, event: &LogEvent) -> BackendResult<()> {
        let mut tx = backend.begin()?;
        self.emit_log_tx(&mut tx, event)?;
        backend.commit(tx)
    }

    fn emit_log_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        event: &LogEvent,
    ) -> BackendResult<()> {
        tx.log_event_appends.push(event.clone());
        Ok(())
    }
}
