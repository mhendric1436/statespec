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

    let mut differences = Vec::new();
    if existing.name != requested.name {
        differences.push(schema_difference(
            SchemaCompatibilityReason::CollectionNameChanged,
            "name",
            format!(
                "collection name changed from '{}' to '{}'",
                existing.name, requested.name
            ),
        ));
    }
    if requested.schema_version < existing.schema_version {
        differences.push(schema_difference(
            SchemaCompatibilityReason::SchemaVersionDecreased,
            "schema_version",
            "schema_version decreased",
        ));
    } else if requested.schema_version == existing.schema_version {
        differences.push(schema_difference(
            SchemaCompatibilityReason::SchemaVersionNotIncremented,
            "schema_version",
            "schema_version must increase when descriptor shape changes",
        ));
    }
    if existing.key_fields != requested.key_fields {
        differences.push(schema_difference(
            SchemaCompatibilityReason::KeyFieldsChanged,
            "key_fields",
            "key_fields changed",
        ));
    }

    for existing_field in &existing.fields {
        let Some(requested_field) = find_field_descriptor(&requested.fields, &existing_field.name)
        else {
            differences.push(schema_difference(
                SchemaCompatibilityReason::FieldRemoved,
                format!("fields.{}", existing_field.name),
                format!("field '{}' was removed", existing_field.name),
            ));
            continue;
        };
        if existing_field.field_type != requested_field.field_type {
            differences.push(schema_difference(
                SchemaCompatibilityReason::FieldTypeChanged,
                format!("fields.{}", existing_field.name),
                format!("field '{}' type changed", existing_field.name),
            ));
        }
        if existing_field.type_name != requested_field.type_name {
            differences.push(schema_difference(
                SchemaCompatibilityReason::FieldTypeNameChanged,
                format!("fields.{}", existing_field.name),
                format!("field '{}' type_name changed", existing_field.name),
            ));
        }
        if !existing_field.required && requested_field.required {
            differences.push(schema_difference(
                SchemaCompatibilityReason::FieldBecameRequired,
                format!("fields.{}", existing_field.name),
                format!("field '{}' became required", existing_field.name),
            ));
        }
    }

    for requested_field in &requested.fields {
        if find_field_descriptor(&existing.fields, &requested_field.name).is_none()
            && requested_field.required
        {
            differences.push(schema_difference(
                SchemaCompatibilityReason::RequiredFieldAdded,
                format!("fields.{}", requested_field.name),
                format!("required field '{}' was added", requested_field.name),
            ));
        }
    }

    for existing_index in &existing.indexes {
        let Some(requested_index) = find_index_descriptor(&requested.indexes, &existing_index.name)
        else {
            differences.push(schema_difference(
                SchemaCompatibilityReason::IndexRemoved,
                format!("indexes.{}", existing_index.name),
                format!("index '{}' was removed", existing_index.name),
            ));
            continue;
        };
        if existing_index.fields != requested_index.fields {
            differences.push(schema_difference(
                SchemaCompatibilityReason::IndexFieldsChanged,
                format!("indexes.{}", existing_index.name),
                format!("index '{}' fields changed", existing_index.name),
            ));
        }
        if existing_index.unique != requested_index.unique {
            differences.push(schema_difference(
                SchemaCompatibilityReason::IndexUniquenessChanged,
                format!("indexes.{}", existing_index.name),
                format!("index '{}' uniqueness changed", existing_index.name),
            ));
        }
    }

    for requested_index in &requested.indexes {
        if find_index_descriptor(&existing.indexes, &requested_index.name).is_none()
            && requested_index.unique
        {
            differences.push(schema_difference(
                SchemaCompatibilityReason::UniqueIndexAdded,
                format!("indexes.{}", requested_index.name),
                format!("unique index '{}' was added", requested_index.name),
            ));
        }
    }

    if !differences.is_empty() {
        return SchemaCompatibilityResult {
            status: SchemaCompatibilityStatus::Incompatible,
            differences,
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

fn schema_difference(
    reason: SchemaCompatibilityReason,
    path: impl Into<String>,
    message: impl Into<String>,
) -> SchemaCompatibilityDifference {
    SchemaCompatibilityDifference {
        reason,
        path: path.into(),
        message: message.into(),
    }
}

fn find_field_descriptor<'a>(
    fields: &'a [FieldDescriptor],
    name: &str,
) -> Option<&'a FieldDescriptor> {
    fields.iter().find(|field| field.name == name)
}

fn find_index_descriptor<'a>(
    indexes: &'a [IndexDescriptor],
    name: &str,
) -> Option<&'a IndexDescriptor> {
    indexes.iter().find(|index| index.name == name)
}
