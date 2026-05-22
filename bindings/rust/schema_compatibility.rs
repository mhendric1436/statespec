use crate::backend::{
    BackendError, BackendResult, CollectionDescriptor, ConflictKind, FieldDescriptor,
    IndexDescriptor,
};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SchemaCompatibilityStatus {
    Identical,
    Compatible,
    Incompatible,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SchemaCompatibilityReason {
    SchemaVersionDecreased,
    SchemaVersionNotIncremented,
    CollectionNameChanged,
    KeyFieldsChanged,
    FieldRemoved,
    FieldRenamed,
    FieldTypeChanged,
    FieldTypeNameChanged,
    RequiredFieldAdded,
    FieldBecameRequired,
    IndexRemoved,
    IndexRenamed,
    IndexFieldsChanged,
    IndexUniquenessChanged,
    UniqueIndexAdded,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SchemaCompatibilityDifference {
    pub reason: SchemaCompatibilityReason,
    pub path: String,
    pub message: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SchemaCompatibilityResult {
    pub status: SchemaCompatibilityStatus,
    pub differences: Vec<SchemaCompatibilityDifference>,
}

impl SchemaCompatibilityResult {
    pub fn compatible(&self) -> bool {
        self.status != SchemaCompatibilityStatus::Incompatible
    }
}

pub fn compare_collection_descriptors(
    existing: &CollectionDescriptor,
    requested: &CollectionDescriptor,
) -> SchemaCompatibilityResult {
    if collection_descriptors_equal(existing, requested) {
        return SchemaCompatibilityResult {
            status: SchemaCompatibilityStatus::Identical,
            differences: Vec::new(),
        };
    }

    SchemaCompatibilityResult {
        status: SchemaCompatibilityStatus::Compatible,
        differences: Vec::new(),
    }
}

pub fn validate_collection_descriptor_upgrade(
    existing: &CollectionDescriptor,
    requested: &CollectionDescriptor,
) -> BackendResult<()> {
    let result = compare_collection_descriptors(existing, requested);
    if result.compatible() {
        return Ok(());
    }

    let mut message = "collection schema upgrade is incompatible".to_string();
    if let Some(difference) = result.differences.first() {
        message.push_str(": ");
        message.push_str(&difference.message);
    }
    Err(BackendError::Conflict {
        kind: ConflictKind::SchemaConflict,
        message,
    })
}

fn collection_descriptors_equal(left: &CollectionDescriptor, right: &CollectionDescriptor) -> bool {
    left.name == right.name
        && left.schema_version == right.schema_version
        && left.key_fields == right.key_fields
        && field_descriptors_equal(&left.fields, &right.fields)
        && index_descriptors_equal(&left.indexes, &right.indexes)
}

fn field_descriptors_equal(left: &[FieldDescriptor], right: &[FieldDescriptor]) -> bool {
    left.len() == right.len()
        && left.iter().zip(right).all(|(left, right)| {
            left.name == right.name
                && left.field_type == right.field_type
                && left.type_name == right.type_name
                && left.required == right.required
        })
}

fn index_descriptors_equal(left: &[IndexDescriptor], right: &[IndexDescriptor]) -> bool {
    left.len() == right.len()
        && left.iter().zip(right).all(|(left, right)| {
            left.name == right.name && left.fields == right.fields && left.unique == right.unique
        })
}
