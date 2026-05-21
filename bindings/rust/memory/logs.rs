use crate::backend::{Backend, BackendResult, Query, Transaction};
use crate::log::{LogDefinition, LogDefinitionRegistration, LogEvent, LogSink};
use crate::memory_codec;

const DEFINITIONS: &str = "logs.definitions";
const EVENTS: &str = "logs.events";

#[derive(Debug, Clone, Default)]
pub struct InMemoryLogSink;

impl InMemoryLogSink {
    pub fn new() -> Self {
        Self
    }

    pub fn inspect_events<B: Backend>(&self, backend: &B) -> BackendResult<Vec<LogEvent>> {
        let mut tx = backend.begin()?;
        let events = self.inspect_events_tx::<B>(&mut tx)?;
        backend.commit(tx)?;
        Ok(events)
    }

    pub fn inspect_events_tx<B: Backend>(&self, tx: &mut B::Tx) -> BackendResult<Vec<LogEvent>> {
        Ok(tx
            .query(EVENTS, &Query::All)?
            .iter()
            .map(|record| memory_codec::log_event_from_json(&record.document))
            .collect())
    }
}

impl<B: Backend> LogSink<B> for InMemoryLogSink {
    fn register_definition(
        &self,
        backend: &B,
        definition: &LogDefinition,
    ) -> BackendResult<LogDefinitionRegistration> {
        let mut tx = backend.begin()?;
        let result =
            <InMemoryLogSink as LogSink<B>>::register_definition_tx(self, &mut tx, definition)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn register_definition_tx(
        &self,
        tx: &mut B::Tx,
        definition: &LogDefinition,
    ) -> BackendResult<LogDefinitionRegistration> {
        let existing =
            <InMemoryLogSink as LogSink<B>>::inspect_definition_tx(self, tx, &definition.name)?;
        tx.put(
            DEFINITIONS,
            &definition.name,
            memory_codec::log_definition_to_json(definition),
        )?;
        Ok(LogDefinitionRegistration {
            registered_new: existing.is_none(),
            definition: definition.clone(),
        })
    }

    fn inspect_definition(&self, backend: &B, name: &str) -> BackendResult<Option<LogDefinition>> {
        let mut tx = backend.begin()?;
        let result = <InMemoryLogSink as LogSink<B>>::inspect_definition_tx(self, &mut tx, name)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_definition_tx(
        &self,
        tx: &mut B::Tx,
        name: &str,
    ) -> BackendResult<Option<LogDefinition>> {
        Ok(tx
            .get(DEFINITIONS, name)?
            .map(|record| memory_codec::log_definition_from_json(&record.document)))
    }

    fn emit_log(&self, backend: &B, event: &LogEvent) -> BackendResult<()> {
        let mut tx = backend.begin()?;
        <InMemoryLogSink as LogSink<B>>::emit_log_tx(self, &mut tx, event)?;
        backend.commit(tx)
    }

    fn emit_log_tx(&self, tx: &mut B::Tx, event: &LogEvent) -> BackendResult<()> {
        let key = format!("event:{:020}", tx.query(EVENTS, &Query::All)?.len() + 1);
        tx.put(EVENTS, &key, memory_codec::log_event_to_json(event))?;
        Ok(())
    }
}
