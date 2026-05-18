use crate::backend::{Backend, BackendResult, VersionedRecord};
use crate::json::Json;

#[derive(Debug, Clone, PartialEq)]
pub struct ExternalSystemMetadataKeyValue {
    pub field: String,
    pub value: Json,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ExternalSystemMetadataLookup {
    pub external_system: String,
    pub metadata_entity: String,
    pub tenant_field: Option<String>,
    pub profile_field: String,
    pub key_fields: Vec<String>,
    pub key_values: Vec<ExternalSystemMetadataKeyValue>,
    pub required_fields: Vec<String>,
}

#[derive(Debug, Clone)]
pub struct ExternalSystemMetadataResolution {
    pub record: VersionedRecord,
    pub missing_required_fields: Vec<String>,
}

impl ExternalSystemMetadataResolution {
    pub fn complete(&self) -> bool {
        self.missing_required_fields.is_empty()
    }
}

pub fn missing_required_metadata_fields(
    document: &Json,
    required_fields: &[String],
) -> Vec<String> {
    required_fields
        .iter()
        .filter(|field| document.find(field).is_none())
        .cloned()
        .collect()
}

pub trait ExternalSystemMetadataResolver<B: Backend> {
    fn resolve_metadata(
        &self,
        backend: &B,
        lookup: &ExternalSystemMetadataLookup,
    ) -> BackendResult<Option<ExternalSystemMetadataResolution>>;

    fn resolve_metadata_tx(
        &self,
        tx: &mut B::Tx,
        lookup: &ExternalSystemMetadataLookup,
    ) -> BackendResult<Option<ExternalSystemMetadataResolution>>;
}
