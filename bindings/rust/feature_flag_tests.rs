#[cfg(test)]
mod tests {
    use crate::feature_flag::{
        FeatureFlagDefinition, FeatureFlagEvaluationContext, FeatureFlagEvaluationRequest,
        FeatureFlagRegisterDefinitionResult, FeatureFlagScopeKind, FeatureFlagType,
        FeatureFlagValue,
    };

    #[test]
    fn feature_flag_values_expose_typed_accessors() {
        let bool_value = FeatureFlagValue::Bool(true);
        assert_eq!(bool_value.flag_type(), FeatureFlagType::Bool);
        assert_eq!(bool_value.as_bool(), Some(true));
        assert_eq!(bool_value.as_string(), None);

        let string_value = FeatureFlagValue::String("new".to_string());
        assert_eq!(string_value.as_string(), Some("new"));

        let integer_value = FeatureFlagValue::Integer(100);
        assert_eq!(integer_value.as_integer(), Some(100));

        let decimal_value = FeatureFlagValue::Decimal(1.5);
        assert_eq!(decimal_value.as_decimal(), Some(1.5));
    }

    #[test]
    fn feature_flag_definition_and_context_hold_metadata() {
        let definition = FeatureFlagDefinition {
            name: "NewScheduler".to_string(),
            flag_type: FeatureFlagType::Bool,
            default_value: FeatureFlagValue::Bool(false),
            scope: FeatureFlagScopeKind::Tenant,
            owner: Some("platform".to_string()),
            description: Some("Route through the new scheduler".to_string()),
            expires: Some("2026-12-31".to_string()),
        };

        let context = FeatureFlagEvaluationContext {
            tenant_id: Some("tenant-a".to_string()),
            ..FeatureFlagEvaluationContext::default()
        };

        assert_eq!(definition.name, "NewScheduler");
        assert_eq!(definition.default_value.as_bool(), Some(false));

        let registration = FeatureFlagRegisterDefinitionResult {
            registered_new: true,
            definition,
        };
        assert!(registration.registered_new);
        assert_eq!(registration.definition.name, "NewScheduler");

        let request = FeatureFlagEvaluationRequest {
            name: "NewScheduler".to_string(),
            context,
        };

        assert_eq!(request.name, "NewScheduler");
        assert_eq!(request.context.tenant_id.as_deref(), Some("tenant-a"));
    }
}
