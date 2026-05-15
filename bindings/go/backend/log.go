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

type LogSink interface {
	EmitLog(ctx context.Context, backend Backend, event LogEvent) error

	EmitLogTx(ctx context.Context, tx Transaction, event LogEvent) error
}
