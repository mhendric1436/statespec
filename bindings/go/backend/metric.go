package backend

import "context"

type MetricKind string

const (
	MetricCounter   MetricKind = "counter"
	MetricGauge     MetricKind = "gauge"
	MetricHistogram MetricKind = "histogram"
)

type MetricSample struct {
	Name        string
	Kind        MetricKind
	BackendName string
	Value       float64
	Unit        string
	Labels      map[string]JSON
}

type MetricInstrumentDefinition struct {
	Name        string
	Kind        MetricKind
	BackendName string
	Unit        string
	Labels      []FieldDescriptor
}

type MetricInstrumentRegistration struct {
	RegisteredNew bool
	Definition    MetricInstrumentDefinition
}

type MetricSink interface {
	// RegisterDefinition is idempotent. Re-registering an identical definition is
	// a no-op; incompatible redefinition should return an error.
	RegisterDefinition(ctx context.Context, backend Backend, definition MetricInstrumentDefinition) (MetricInstrumentRegistration, error)

	RegisterDefinitionTx(ctx context.Context, tx Transaction, definition MetricInstrumentDefinition) (MetricInstrumentRegistration, error)

	// RecordMetricTx stages the sample in the caller's OCC transaction. Commit
	// makes it visible to exporters; rollback drops it. Implementations should
	// validate labels against the registered metric definition.
	RecordMetric(ctx context.Context, backend Backend, sample MetricSample) error

	RecordMetricTx(ctx context.Context, tx Transaction, sample MetricSample) error
}
