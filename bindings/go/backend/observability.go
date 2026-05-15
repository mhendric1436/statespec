package backend

import "context"

type LogLevel string

const (
	LogLevelDebug LogLevel = "debug"
	LogLevelInfo  LogLevel = "info"
	LogLevelWarn  LogLevel = "warn"
	LogLevelError LogLevel = "error"
)

type MetricKind string

const (
	MetricCounter   MetricKind = "counter"
	MetricGauge     MetricKind = "gauge"
	MetricHistogram MetricKind = "histogram"
)

type LogEvent struct {
	Name      string
	Level     LogLevel
	EventName string
	Fields    map[string]JSON
}

type MetricSample struct {
	Name        string
	Kind        MetricKind
	BackendName string
	Value       float64
	Unit        string
	Labels      map[string]JSON
}

type ObservabilitySink interface {
	EmitLog(ctx context.Context, backend Backend, event LogEvent) error

	EmitLogTx(ctx context.Context, tx Transaction, event LogEvent) error

	RecordMetric(ctx context.Context, backend Backend, sample MetricSample) error

	RecordMetricTx(ctx context.Context, tx Transaction, sample MetricSample) error
}
