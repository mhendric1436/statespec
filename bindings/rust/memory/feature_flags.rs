use crate::backend::{Backend, BackendError, BackendResult};
use crate::feature_flag::{
    FeatureFlagDefinition, FeatureFlagEvaluationRequest, FeatureFlagRegisterDefinitionResult,
    FeatureFlagStore, FeatureFlagValue,
};
use crate::memory_backend::{lock_error, InMemoryBackend};

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
        tx.feature_flag_definition_puts
            .insert(definition.name.clone(), definition.clone());
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
        if let Some(definition) = tx.feature_flag_definition_puts.get(name) {
            return Ok(Some(definition.clone()));
        }
        Ok(tx
            .state
            .lock()
            .map_err(lock_error)?
            .feature_flag_definitions
            .get(name)
            .cloned())
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
        if let Some(value) = tx.feature_flag_value_puts.get(&request.name) {
            return Ok(value.clone());
        }
        if let Some(value) = tx
            .state
            .lock()
            .map_err(lock_error)?
            .feature_flag_values
            .get(&request.name)
            .cloned()
        {
            return Ok(value);
        }
        self.inspect_definition_tx(tx, &request.name)?
            .map(|definition| definition.default_value)
            .ok_or_else(|| BackendError::NotFound {
                message: format!("unknown feature flag {}", request.name),
            })
    }
}
