#[cfg(test)]
mod tests {
    use crate::backend::{CollectionDescriptor, FieldDescriptor, FieldType, IndexDescriptor};
    use crate::schema_compatibility::{
        compare_collection_descriptors, SchemaCompatibilityReason, SchemaCompatibilityStatus,
    };

    fn base_descriptor() -> CollectionDescriptor {
        CollectionDescriptor {
            name: "orders".to_string(),
            fields: vec![
                FieldDescriptor {
                    name: "tenant_id".to_string(),
                    field_type: FieldType::String,
                    type_name: "string".to_string(),
                    required: true,
                },
                FieldDescriptor {
                    name: "status".to_string(),
                    field_type: FieldType::String,
                    type_name: "string".to_string(),
                    required: false,
                },
            ],
            key_fields: vec!["tenant_id".to_string()],
            indexes: vec![IndexDescriptor {
                name: "by_status".to_string(),
                fields: vec!["status".to_string()],
                unique: false,
            }],
            schema_version: 1,
        }
    }

    fn require_reason(requested: CollectionDescriptor, reason: SchemaCompatibilityReason) {
        let result = compare_collection_descriptors(&base_descriptor(), &requested);
        assert_eq!(result.status, SchemaCompatibilityStatus::Incompatible);
        assert!(
            result
                .differences
                .iter()
                .any(|difference| difference.reason == reason),
            "expected reason {:?} in {:?}",
            reason,
            result.differences
        );
    }

    #[test]
    fn allows_identical_and_compatible_changes() {
        let base = base_descriptor();
        let identical = compare_collection_descriptors(&base, &base);
        assert_eq!(identical.status, SchemaCompatibilityStatus::Identical);
        assert!(identical.compatible());

        let mut optional_field = base.clone();
        optional_field.schema_version = 2;
        optional_field.fields.push(FieldDescriptor {
            name: "description".to_string(),
            field_type: FieldType::String,
            type_name: "string".to_string(),
            required: false,
        });
        let result = compare_collection_descriptors(&base, &optional_field);
        assert_eq!(result.status, SchemaCompatibilityStatus::Compatible);

        let mut non_unique_index = base.clone();
        non_unique_index.schema_version = 2;
        non_unique_index.indexes.push(IndexDescriptor {
            name: "by_tenant_status".to_string(),
            fields: vec!["tenant_id".to_string(), "status".to_string()],
            unique: false,
        });
        assert!(compare_collection_descriptors(&base, &non_unique_index).compatible());
    }

    #[test]
    fn rejects_incompatible_changes() {
        let base = base_descriptor();

        let mut same_version = base.clone();
        same_version.fields.push(FieldDescriptor {
            name: "description".to_string(),
            field_type: FieldType::String,
            type_name: "string".to_string(),
            required: false,
        });
        require_reason(
            same_version.clone(),
            SchemaCompatibilityReason::SchemaVersionNotIncremented,
        );

        same_version.schema_version = 0;
        require_reason(
            same_version,
            SchemaCompatibilityReason::SchemaVersionDecreased,
        );

        let mut key_changed = base.clone();
        key_changed.schema_version = 2;
        key_changed.key_fields = vec!["tenant_id".to_string(), "order_id".to_string()];
        require_reason(key_changed, SchemaCompatibilityReason::KeyFieldsChanged);

        let mut field_removed = base.clone();
        field_removed.schema_version = 2;
        field_removed.fields.pop();
        require_reason(field_removed, SchemaCompatibilityReason::FieldRemoved);

        let mut field_type_changed = base.clone();
        field_type_changed.schema_version = 2;
        field_type_changed.fields[1].field_type = FieldType::Int;
        require_reason(
            field_type_changed,
            SchemaCompatibilityReason::FieldTypeChanged,
        );

        let mut field_type_name_changed = base.clone();
        field_type_name_changed.schema_version = 2;
        field_type_name_changed.fields[1].type_name = "Status".to_string();
        require_reason(
            field_type_name_changed,
            SchemaCompatibilityReason::FieldTypeNameChanged,
        );

        let mut field_became_required = base.clone();
        field_became_required.schema_version = 2;
        field_became_required.fields[1].required = true;
        require_reason(
            field_became_required,
            SchemaCompatibilityReason::FieldBecameRequired,
        );

        let mut required_field_added = base.clone();
        required_field_added.schema_version = 2;
        required_field_added.fields.push(FieldDescriptor {
            name: "priority".to_string(),
            field_type: FieldType::Int,
            type_name: "int".to_string(),
            required: true,
        });
        require_reason(
            required_field_added,
            SchemaCompatibilityReason::RequiredFieldAdded,
        );

        let mut index_removed = base.clone();
        index_removed.schema_version = 2;
        index_removed.indexes.clear();
        require_reason(index_removed, SchemaCompatibilityReason::IndexRemoved);

        let mut index_fields_changed = base.clone();
        index_fields_changed.schema_version = 2;
        index_fields_changed.indexes[0].fields =
            vec!["tenant_id".to_string(), "status".to_string()];
        require_reason(
            index_fields_changed,
            SchemaCompatibilityReason::IndexFieldsChanged,
        );

        let mut index_unique_changed = base.clone();
        index_unique_changed.schema_version = 2;
        index_unique_changed.indexes[0].unique = true;
        require_reason(
            index_unique_changed,
            SchemaCompatibilityReason::IndexUniquenessChanged,
        );

        let mut unique_index_added = base.clone();
        unique_index_added.schema_version = 2;
        unique_index_added.indexes.push(IndexDescriptor {
            name: "unique_status".to_string(),
            fields: vec!["status".to_string()],
            unique: true,
        });
        require_reason(
            unique_index_added,
            SchemaCompatibilityReason::UniqueIndexAdded,
        );
    }
}
