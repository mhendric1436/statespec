package backend

import "context"

type FeatureFlagType string

const (
	FeatureFlagBool    FeatureFlagType = "bool"
	FeatureFlagString  FeatureFlagType = "string"
	FeatureFlagInteger FeatureFlagType = "int"
	FeatureFlagDecimal FeatureFlagType = "decimal"
)

type FeatureFlagScopeKind string

const (
	FeatureFlagScopeSystem FeatureFlagScopeKind = "system"
	FeatureFlagScopeTenant FeatureFlagScopeKind = "tenant"
	FeatureFlagScopeUser   FeatureFlagScopeKind = "user"
	FeatureFlagScopeEntity FeatureFlagScopeKind = "entity"
)

type FeatureFlagValue struct {
	Type         FeatureFlagType
	BoolValue    bool
	StringValue  string
	IntValue     int64
	DecimalValue float64
}

func BoolFeatureFlagValue(value bool) FeatureFlagValue {
	return FeatureFlagValue{Type: FeatureFlagBool, BoolValue: value}
}

func StringFeatureFlagValue(value string) FeatureFlagValue {
	return FeatureFlagValue{Type: FeatureFlagString, StringValue: value}
}

func IntegerFeatureFlagValue(value int64) FeatureFlagValue {
	return FeatureFlagValue{Type: FeatureFlagInteger, IntValue: value}
}

func DecimalFeatureFlagValue(value float64) FeatureFlagValue {
	return FeatureFlagValue{Type: FeatureFlagDecimal, DecimalValue: value}
}

func (v FeatureFlagValue) AsBool() (bool, bool) {
	if v.Type != FeatureFlagBool {
		return false, false
	}
	return v.BoolValue, true
}

func (v FeatureFlagValue) AsString() (string, bool) {
	if v.Type != FeatureFlagString {
		return "", false
	}
	return v.StringValue, true
}

func (v FeatureFlagValue) AsInteger() (int64, bool) {
	if v.Type != FeatureFlagInteger {
		return 0, false
	}
	return v.IntValue, true
}

func (v FeatureFlagValue) AsDecimal() (float64, bool) {
	if v.Type != FeatureFlagDecimal {
		return 0, false
	}
	return v.DecimalValue, true
}

type FeatureFlagDefinition struct {
	Name         string
	Type         FeatureFlagType
	DefaultValue FeatureFlagValue
	Scope        FeatureFlagScopeKind
	Owner        *string
	Description  *string
	Expires      *string
}

type FeatureFlagRegisterDefinitionResult struct {
	RegisteredNew bool
	Definition    FeatureFlagDefinition
}

type FeatureFlagEvaluationContext struct {
	TenantID   *string
	UserID     *string
	EntityType *string
	EntityID   *string
}

type FeatureFlagEvaluationRequest struct {
	Name    string
	Context FeatureFlagEvaluationContext
}

type FeatureFlagStore interface {
	RegisterDefinition(ctx context.Context, backend Backend, definition FeatureFlagDefinition) (FeatureFlagRegisterDefinitionResult, error)

	RegisterDefinitionTx(ctx context.Context, tx Transaction, definition FeatureFlagDefinition) (FeatureFlagRegisterDefinitionResult, error)

	InspectDefinition(ctx context.Context, backend Backend, name string) (*FeatureFlagDefinition, error)

	InspectDefinitionTx(ctx context.Context, tx Transaction, name string) (*FeatureFlagDefinition, error)

	Evaluate(ctx context.Context, backend Backend, request FeatureFlagEvaluationRequest) (FeatureFlagValue, error)

	EvaluateTx(ctx context.Context, tx Transaction, request FeatureFlagEvaluationRequest) (FeatureFlagValue, error)
}
