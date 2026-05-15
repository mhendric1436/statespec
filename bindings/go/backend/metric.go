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

type MetricSink interface {
	RecordMetric(ctx context.Context, backend Backend, sample MetricSample) error

	RecordMetricTx(ctx context.Context, tx Transaction, sample MetricSample) error
}
