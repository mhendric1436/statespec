use crate::backend::{Backend, BackendError, BackendResult, Transaction};
use crate::feature_flag::{
    FeatureFlagDefinition, FeatureFlagEvaluationRequest, FeatureFlagRegisterDefinitionResult,
    FeatureFlagStore, FeatureFlagValue,
};
use crate::runtime_codec;

const DEFINITIONS: &str = "feature_flags.definitions";
const VALUES: &str = "feature_flags.values";

#[derive(Debug, Clone, Default)]
pub struct RuntimeFeatureFlagStore;

impl RuntimeFeatureFlagStore {
    pub fn new() -> Self {
        Self
    }
}

impl<B: Backend> FeatureFlagStore<B> for RuntimeFeatureFlagStore {
    fn register_definition(
        &self,
        backend: &B,
        definition: &FeatureFlagDefinition,
    ) -> BackendResult<FeatureFlagRegisterDefinitionResult> {
        let mut tx = backend.begin()?;
        let result = <RuntimeFeatureFlagStore as FeatureFlagStore<B>>::register_definition_tx(
            self, &mut tx, definition,
        )?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn register_definition_tx(
        &self,
        tx: &mut B::Tx,
        definition: &FeatureFlagDefinition,
    ) -> BackendResult<FeatureFlagRegisterDefinitionResult> {
        let existing = <RuntimeFeatureFlagStore as FeatureFlagStore<B>>::inspect_definition_tx(
            self,
            tx,
            &definition.name,
        )?;
        tx.put(
            DEFINITIONS,
            &definition.name,
            runtime_codec::feature_flag_definition_to_json(definition),
        )?;
        Ok(FeatureFlagRegisterDefinitionResult {
            registered_new: existing.is_none(),
            definition: definition.clone(),
        })
    }

    fn inspect_definition(
        &self,
        backend: &B,
        name: &str,
    ) -> BackendResult<Option<FeatureFlagDefinition>> {
        let mut tx = backend.begin()?;
        let result = <RuntimeFeatureFlagStore as FeatureFlagStore<B>>::inspect_definition_tx(
            self, &mut tx, name,
        )?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_definition_tx(
        &self,
        tx: &mut B::Tx,
        name: &str,
    ) -> BackendResult<Option<FeatureFlagDefinition>> {
        Ok(tx
            .get(DEFINITIONS, name)?
            .map(|record| runtime_codec::feature_flag_definition_from_json(&record.document)))
    }

    fn evaluate(
        &self,
        backend: &B,
        request: &FeatureFlagEvaluationRequest,
    ) -> BackendResult<FeatureFlagValue> {
        let mut tx = backend.begin()?;
        let result =
            <RuntimeFeatureFlagStore as FeatureFlagStore<B>>::evaluate_tx(self, &mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn evaluate_tx(
        &self,
        tx: &mut B::Tx,
        request: &FeatureFlagEvaluationRequest,
    ) -> BackendResult<FeatureFlagValue> {
        if let Some(record) = tx.get(VALUES, &request.name)? {
            return Ok(runtime_codec::feature_flag_value_from_json(
                &record.document,
            ));
        }
        <RuntimeFeatureFlagStore as FeatureFlagStore<B>>::inspect_definition_tx(
            self,
            tx,
            &request.name,
        )?
        .map(|definition| definition.default_value)
        .ok_or_else(|| BackendError::NotFound {
            message: format!("unknown feature flag {}", request.name),
        })
    }
}
