#[cfg(test)]
mod tests {
    use crate::backend::{
        Backend, BackendError, CollectionDescriptor, ConflictKind, FieldDescriptor, FieldType,
        IndexDescriptor,
    };
    use crate::json::Json;
    use crate::memory::backend::InMemoryBackend;
    use crate::memory_transaction::{extract_index_key, InMemoryIndexKey};
    use std::collections::BTreeMap;

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

    fn unique_status_descriptor() -> CollectionDescriptor {
        let mut descriptor = base_descriptor();
        descriptor.indexes[0].unique = true;
        descriptor
    }

    fn order_document(status: &str) -> Json {
        Json::Object(BTreeMap::from([(
            "status".to_string(),
            Json::String(status.to_string()),
        )]))
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

    #[test]
    fn extracts_index_keys_deterministically() {
        let index = IndexDescriptor {
            name: "by_status".to_string(),
            fields: vec![
                "status".to_string(),
                "priority".to_string(),
                "archived".to_string(),
                "missing".to_string(),
                "cleared".to_string(),
            ],
            unique: false,
        };
        let document = Json::Object(BTreeMap::from([
            ("status".to_string(), Json::String("Open".to_string())),
            ("priority".to_string(), Json::Integer(3)),
            ("archived".to_string(), Json::Bool(false)),
            ("cleared".to_string(), Json::Null),
        ]));

        let key = extract_index_key(&document, &index).unwrap();
        assert_eq!(
            key,
            InMemoryIndexKey(vec![
                "string:Open".to_string(),
                "int:3".to_string(),
                "bool:false".to_string(),
                "missing:".to_string(),
                "null:".to_string(),
            ])
        );

        let invalid = Json::Object(BTreeMap::from([(
            "status".to_string(),
            Json::Array(vec![Json::String("Open".to_string())]),
        )]));
        assert!(matches!(
            extract_index_key(&invalid, &index),
            Err(BackendError::InvalidSchema { .. })
        ));
    }

    #[test]
    fn enforces_unique_indexes_on_commit() {
        let backend = InMemoryBackend::new();
        backend
            .ensure_collection(&unique_status_descriptor())
            .unwrap();

        let mut tx1 = backend.begin().unwrap();
        backend
            .put(&mut tx1, "orders", "order-1", order_document("Open"))
            .unwrap();
        backend.commit(tx1).unwrap();

        let mut duplicate_tx = backend.begin().unwrap();
        backend
            .put(
                &mut duplicate_tx,
                "orders",
                "order-2",
                order_document("Open"),
            )
            .unwrap();
        assert!(matches!(
            backend.commit(duplicate_tx),
            Err(BackendError::Conflict {
                kind: ConflictKind::UniqueIndexConflict,
                ..
            })
        ));

        let mut update_tx = backend.begin().unwrap();
        backend
            .put(
                &mut update_tx,
                "orders",
                "order-1",
                order_document("Closed"),
            )
            .unwrap();
        backend.commit(update_tx).unwrap();

        let mut insert_tx = backend.begin().unwrap();
        backend
            .put(&mut insert_tx, "orders", "order-2", order_document("Open"))
            .unwrap();
        backend.commit(insert_tx).unwrap();
    }
}
