use crate::backend::{Backend, BackendError, BackendResult};
use crate::feature_flag::{
    FeatureFlagDefinition, FeatureFlagEvaluationRequest, FeatureFlagRegisterDefinitionResult,
    FeatureFlagStore, FeatureFlagValue,
};
use crate::memory_backend::InMemoryBackend;
use crate::memory_codec;

const DEFINITIONS: &str = "feature_flags.definitions";
const VALUES: &str = "feature_flags.values";

#[derive(Debug, Clone, Default)]
pub struct InMemoryFeatureFlagStore;

impl InMemoryFeatureFlagStore {
    pub fn new() -> Self {
        Self
    }
}

impl FeatureFlagStore<InMemoryBackend> for InMemoryFeatureFlagStore {
    fn register_definition(
        &self,
        backend: &InMemoryBackend,
        definition: &FeatureFlagDefinition,
    ) -> BackendResult<FeatureFlagRegisterDefinitionResult> {
        let mut tx = backend.begin()?;
        let result = self.register_definition_tx(&mut tx, definition)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn register_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        definition: &FeatureFlagDefinition,
    ) -> BackendResult<FeatureFlagRegisterDefinitionResult> {
        let existing = self.inspect_definition_tx(tx, &definition.name)?;
        tx.put(
            DEFINITIONS,
            &definition.name,
            memory_codec::feature_flag_definition_to_json(definition),
        )?;
        Ok(FeatureFlagRegisterDefinitionResult {
            registered_new: existing.is_none(),
            definition: definition.clone(),
        })
    }

    fn inspect_definition(
        &self,
        backend: &InMemoryBackend,
        name: &str,
    ) -> BackendResult<Option<FeatureFlagDefinition>> {
        let mut tx = backend.begin()?;
        let result = self.inspect_definition_tx(&mut tx, name)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        name: &str,
    ) -> BackendResult<Option<FeatureFlagDefinition>> {
        Ok(tx
            .get(DEFINITIONS, name)?
            .map(|record| memory_codec::feature_flag_definition_from_json(&record.document)))
    }

    fn evaluate(
        &self,
        backend: &InMemoryBackend,
        request: &FeatureFlagEvaluationRequest,
    ) -> BackendResult<FeatureFlagValue> {
        let mut tx = backend.begin()?;
        let result = self.evaluate_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn evaluate_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &FeatureFlagEvaluationRequest,
    ) -> BackendResult<FeatureFlagValue> {
        if let Some(record) = tx.get(VALUES, &request.name)? {
            return Ok(memory_codec::feature_flag_value_from_json(&record.document));
        }
        self.inspect_definition_tx(tx, &request.name)?
            .map(|definition| definition.default_value)
            .ok_or_else(|| BackendError::NotFound {
                message: format!("unknown feature flag {}", request.name),
            })
    }
}
