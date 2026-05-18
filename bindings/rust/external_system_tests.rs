#[cfg(test)]
mod tests {
    use std::collections::BTreeMap;

    use crate::backend::VersionedRecord;
    use crate::external_system::{
        missing_required_metadata_fields, ExternalSystemMetadataKeyValue,
        ExternalSystemMetadataLookup, ExternalSystemMetadataResolution,
    };
    use crate::json::Json;

    #[test]
    fn external_system_metadata_lookup_contracts() {
        let lookup = ExternalSystemMetadataLookup {
            external_system: "Billing.Stripe".to_string(),
            metadata_entity: "ExternalSystemEndpoint".to_string(),
            tenant_field: Some("tenant_id".to_string()),
            profile_field: "profile".to_string(),
            key_fields: vec![
                "tenant_id".to_string(),
                "external_system_id".to_string(),
                "profile".to_string(),
            ],
            key_values: vec![
                ExternalSystemMetadataKeyValue {
                    field: "tenant_id".to_string(),
                    value: Json::String("tenant-a".to_string()),
                },
                ExternalSystemMetadataKeyValue {
                    field: "external_system_id".to_string(),
                    value: Json::String("stripe".to_string()),
                },
                ExternalSystemMetadataKeyValue {
                    field: "profile".to_string(),
                    value: Json::String("default".to_string()),
                },
            ],
            required_fields: vec![
                "base_url".to_string(),
                "auth_ref".to_string(),
                "timeout_ms".to_string(),
            ],
        };

        let document = Json::Object(BTreeMap::from([
            (
                "tenant_id".to_string(),
                Json::String("tenant-a".to_string()),
            ),
            (
                "base_url".to_string(),
                Json::String("https://api.stripe.test".to_string()),
            ),
        ]));
        let missing = missing_required_metadata_fields(&document, &lookup.required_fields);

        assert_eq!(lookup.tenant_field.as_deref(), Some("tenant_id"));
        assert_eq!(lookup.key_fields.len(), 3);
        assert_eq!(
            missing,
            vec!["auth_ref".to_string(), "timeout_ms".to_string()]
        );

        let resolution = ExternalSystemMetadataResolution {
            record: VersionedRecord {
                collection: lookup.metadata_entity.clone(),
                key: "tenant-a/stripe/default".to_string(),
                version: 1,
                document,
            },
            missing_required_fields: missing,
        };

        assert!(!resolution.complete());
    }
}
