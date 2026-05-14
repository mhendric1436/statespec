use crate::backend::{Backend, BackendResult};

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum FeatureFlagType {
    Bool,
    String,
    Integer,
    Decimal,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum FeatureFlagScopeKind {
    System,
    Tenant,
    User,
    Entity,
}

#[derive(Debug, Clone, PartialEq)]
pub enum FeatureFlagValue {
    Bool(bool),
    String(String),
    Integer(i64),
    Decimal(f64),
}

impl FeatureFlagValue {
    pub fn flag_type(&self) -> FeatureFlagType {
        match self {
            FeatureFlagValue::Bool(_) => FeatureFlagType::Bool,
            FeatureFlagValue::String(_) => FeatureFlagType::String,
            FeatureFlagValue::Integer(_) => FeatureFlagType::Integer,
            FeatureFlagValue::Decimal(_) => FeatureFlagType::Decimal,
        }
    }

    pub fn as_bool(&self) -> Option<bool> {
        match self {
            FeatureFlagValue::Bool(value) => Some(*value),
            _ => None,
        }
    }

    pub fn as_string(&self) -> Option<&str> {
        match self {
            FeatureFlagValue::String(value) => Some(value),
            _ => None,
        }
    }

    pub fn as_integer(&self) -> Option<i64> {
        match self {
            FeatureFlagValue::Integer(value) => Some(*value),
            _ => None,
        }
    }

    pub fn as_decimal(&self) -> Option<f64> {
        match self {
            FeatureFlagValue::Decimal(value) => Some(*value),
            _ => None,
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct FeatureFlagDefinition {
    pub name: String,
    pub flag_type: FeatureFlagType,
    pub default_value: FeatureFlagValue,
    pub scope: FeatureFlagScopeKind,
    pub owner: Option<String>,
    pub description: Option<String>,
    pub expires: Option<String>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct FeatureFlagRegisterDefinitionResult {
    pub registered_new: bool,
    pub definition: FeatureFlagDefinition,
}

#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct FeatureFlagEvaluationContext {
    pub tenant_id: Option<String>,
    pub user_id: Option<String>,
    pub entity_type: Option<String>,
    pub entity_id: Option<String>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FeatureFlagEvaluationRequest {
    pub name: String,
    pub context: FeatureFlagEvaluationContext,
}

pub trait FeatureFlagStore<B: Backend> {
    fn register_definition(
        &self,
        backend: &B,
        definition: &FeatureFlagDefinition,
    ) -> BackendResult<FeatureFlagRegisterDefinitionResult>;

    fn register_definition_tx(
        &self,
        tx: &mut B::Tx,
        definition: &FeatureFlagDefinition,
    ) -> BackendResult<FeatureFlagRegisterDefinitionResult>;

    fn inspect_definition(
        &self,
        backend: &B,
        name: &str,
    ) -> BackendResult<Option<FeatureFlagDefinition>>;

    fn inspect_definition_tx(
        &self,
        tx: &mut B::Tx,
        name: &str,
    ) -> BackendResult<Option<FeatureFlagDefinition>>;

    fn evaluate(
        &self,
        backend: &B,
        request: &FeatureFlagEvaluationRequest,
    ) -> BackendResult<FeatureFlagValue>;

    fn evaluate_tx(
        &self,
        tx: &mut B::Tx,
        request: &FeatureFlagEvaluationRequest,
    ) -> BackendResult<FeatureFlagValue>;
}
