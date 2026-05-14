package backend

import "testing"

func TestFeatureFlagValuesExposeTypedAccessors(t *testing.T) {
	boolValue := BoolFeatureFlagValue(true)
	if value, ok := boolValue.AsBool(); !ok || !value {
		t.Fatalf("expected bool feature flag value")
	}
	if _, ok := boolValue.AsString(); ok {
		t.Fatalf("bool value should not expose string accessor")
	}

	stringValue := StringFeatureFlagValue("new")
	if value, ok := stringValue.AsString(); !ok || value != "new" {
		t.Fatalf("expected string feature flag value")
	}

	intValue := IntegerFeatureFlagValue(100)
	if value, ok := intValue.AsInteger(); !ok || value != 100 {
		t.Fatalf("expected integer feature flag value")
	}

	decimalValue := DecimalFeatureFlagValue(1.5)
	if value, ok := decimalValue.AsDecimal(); !ok || value != 1.5 {
		t.Fatalf("expected decimal feature flag value")
	}
}

func TestFeatureFlagDefinitionAndContext(t *testing.T) {
	owner := "platform"
	tenantID := "tenant-a"
	definition := FeatureFlagDefinition{
		Name:         "NewScheduler",
		Type:         FeatureFlagBool,
		DefaultValue: BoolFeatureFlagValue(false),
		Scope:        FeatureFlagScopeTenant,
		Owner:        &owner,
	}
	context := FeatureFlagEvaluationContext{TenantID: &tenantID}

	if definition.Name != "NewScheduler" || definition.Owner == nil || *definition.Owner != owner {
		t.Fatalf("unexpected feature flag definition")
	}
	if value, ok := definition.DefaultValue.AsBool(); !ok || value {
		t.Fatalf("expected false default value")
	}

	registration := FeatureFlagRegisterDefinitionResult{RegisteredNew: true, Definition: definition}
	if !registration.RegisteredNew || registration.Definition.Name != "NewScheduler" {
		t.Fatalf("unexpected feature flag registration")
	}

	request := FeatureFlagEvaluationRequest{Name: "NewScheduler", Context: context}
	if context.TenantID == nil || *context.TenantID != tenantID {
		t.Fatalf("expected tenant context")
	}
	if request.Name != "NewScheduler" || request.Context.TenantID == nil || *request.Context.TenantID != tenantID {
		t.Fatalf("expected evaluation request context")
	}
}
