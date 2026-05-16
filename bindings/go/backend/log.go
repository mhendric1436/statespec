package backend

import "context"

type LogLevel string

const (
	LogLevelDebug LogLevel = "debug"
	LogLevelInfo  LogLevel = "info"
	LogLevelWarn  LogLevel = "warn"
	LogLevelError LogLevel = "error"
)

type LogEvent struct {
	Name      string
	Level     LogLevel
	EventName string
	Fields    map[string]JSON
}

type LogSignalDefinition struct {
	Name      string
	Level     LogLevel
	EventName string
	Fields    []FieldDescriptor
}

type LogSignalRegistration struct {
	RegisteredNew bool
	Definition    LogSignalDefinition
}

type LogSink interface {
	// RegisterDefinition is idempotent. Re-registering an identical definition is
	// a no-op; incompatible redefinition should return an error.
	RegisterDefinition(ctx context.Context, backend Backend, definition LogSignalDefinition) (LogSignalRegistration, error)

	RegisterDefinitionTx(ctx context.Context, tx Transaction, definition LogSignalDefinition) (LogSignalRegistration, error)

	InspectDefinition(ctx context.Context, backend Backend, name string) (*LogSignalDefinition, error)

	InspectDefinitionTx(ctx context.Context, tx Transaction, name string) (*LogSignalDefinition, error)

	// EmitLogTx stages the event in the caller's OCC transaction. Commit makes it
	// visible to exporters; rollback drops it.
	EmitLog(ctx context.Context, backend Backend, event LogEvent) error

	EmitLogTx(ctx context.Context, tx Transaction, event LogEvent) error
}
