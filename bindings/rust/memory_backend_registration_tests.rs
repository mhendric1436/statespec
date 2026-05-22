#[cfg(test)]
mod tests {
    use crate::backend::{
        Backend, BackendError, CollectionDescriptor, ConflictKind, FieldDescriptor, FieldType,
        IndexDescriptor,
    };
    use crate::memory::backend::InMemoryBackend;

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

    fn compatible_descriptor() -> CollectionDescriptor {
        let mut descriptor = base_descriptor();
        descriptor.schema_version = 2;
        descriptor.fields.push(FieldDescriptor {
            name: "description".to_string(),
            field_type: FieldType::String,
            type_name: "string".to_string(),
            required: false,
        });
        descriptor
    }

    #[test]
    fn enforces_compatible_collection_registration() {
        let backend = InMemoryBackend::new();
        let base = base_descriptor();

        backend.ensure_collection(&base).unwrap();
        backend.ensure_collection(&base).unwrap();
        backend.ensure_collection(&compatible_descriptor()).unwrap();

        let mut incompatible = compatible_descriptor();
        incompatible.schema_version = 3;
        incompatible.key_fields = vec!["tenant_id".to_string(), "order_id".to_string()];

        match backend.ensure_collection(&incompatible) {
            Err(BackendError::Conflict { kind, message }) => {
                assert!(matches!(kind, ConflictKind::SchemaConflict));
                assert!(message.contains("orders"));
                assert!(message.contains("KeyFieldsChanged"));
            }
            other => panic!("expected schema conflict, got {:?}", other),
        }
    }

    fn alternate_compatible_descriptor() -> CollectionDescriptor {
        let mut descriptor = base_descriptor();
        descriptor.schema_version = 2;
        descriptor.fields.push(FieldDescriptor {
            name: "notes".to_string(),
            field_type: FieldType::String,
            type_name: "string".to_string(),
            required: false,
        });
        descriptor
    }

    #[test]
    fn applies_collection_batches_atomically() {
        let backend = InMemoryBackend::new();
        backend.ensure_collection(&base_descriptor()).unwrap();

        let mut incompatible = base_descriptor();
        incompatible.schema_version = 2;
        incompatible.key_fields = vec!["tenant_id".to_string(), "order_id".to_string()];

        assert!(matches!(
            backend.ensure_collections(&[compatible_descriptor(), incompatible]),
            Err(BackendError::Conflict {
                kind: ConflictKind::SchemaConflict,
                ..
            })
        ));

        backend
            .ensure_collection(&alternate_compatible_descriptor())
            .unwrap();
    }
}
