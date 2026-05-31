package backend

import (
	"context"
	"errors"
)

type CollectionName string
type Key string
type Version uint64

const (
	RuntimeCollectionFeatureFlagDefinitions CollectionName = "statespec_feature_flag_definitions"
	RuntimeCollectionFeatureFlagValues      CollectionName = "statespec_feature_flag_values"
	RuntimeCollectionQueueDefinitions       CollectionName = "statespec_queue_definitions"
	RuntimeCollectionQueueMessages          CollectionName = "statespec_queue_messages"
	RuntimeCollectionQueueIdempotency       CollectionName = "statespec_queue_idempotency"
	RuntimeCollectionLeaseDefinitions       CollectionName = "statespec_lease_definitions"
	RuntimeCollectionLeases                 CollectionName = "statespec_leases"
	RuntimeCollectionWorkflowDefinitions    CollectionName = "statespec_workflow_definitions"
	RuntimeCollectionWorkflowExecutions     CollectionName = "statespec_workflow_executions"
	RuntimeCollectionWorkflowHeartbeats     CollectionName = "statespec_workflow_heartbeats"
	RuntimeCollectionLogDefinitions         CollectionName = "statespec_log_definitions"
	RuntimeCollectionLogEvents              CollectionName = "statespec_log_events"
	RuntimeCollectionMetricDefinitions      CollectionName = "statespec_metric_definitions"
	RuntimeCollectionMetricSamples          CollectionName = "statespec_metric_samples"
)

const (
	RuntimeKeyFieldName                = "name"
	RuntimeKeyFieldScopeKey            = "scope_key"
	RuntimeKeyFieldQueue               = "queue"
	RuntimeKeyFieldMessageID           = "message_id"
	RuntimeKeyFieldIdempotencyKey      = "idempotency_key"
	RuntimeKeyFieldLeaseDefinition     = "lease_definition"
	RuntimeKeyFieldLease               = "lease"
	RuntimeKeyFieldWorkflowDefinition  = "workflow_definition"
	RuntimeKeyFieldWorkflowExecutionID = "workflow_execution_id"
	RuntimeKeyFieldWorkflowHeartbeat   = "workflow_heartbeat"
	RuntimeKeyFieldEventID             = "event_id"
	RuntimeKeyFieldSampleID            = "sample_id"
)

type FieldType string

const (
	FieldTypeString    FieldType = "string"
	FieldTypeBool      FieldType = "bool"
	FieldTypeInt       FieldType = "int"
	FieldTypeInt32     FieldType = "int32"
	FieldTypeInt64     FieldType = "int64"
	FieldTypeLong      FieldType = "long"
	FieldTypeDouble    FieldType = "double"
	FieldTypeDecimal   FieldType = "decimal"
	FieldTypeJSON      FieldType = "json"
	FieldTypeTimestamp FieldType = "timestamp"
	FieldTypeDuration  FieldType = "duration"
	FieldTypeUUID      FieldType = "uuid"
	FieldTypeNamed     FieldType = "named"
	FieldTypeList      FieldType = "list"
	FieldTypeSet       FieldType = "set"
	FieldTypeMap       FieldType = "map"
	FieldTypeOptional  FieldType = "optional"
	FieldTypeReference FieldType = "reference"
)

type FieldDescriptor struct {
	Name     string
	Type     FieldType
	TypeName string
	Required bool
}

type IndexDescriptor struct {
	Name   string
	Fields []string
	Unique bool
}

type CollectionDescriptor struct {
	Name          string
	Fields        []FieldDescriptor
	KeyFields     []string
	Indexes       []IndexDescriptor
	SchemaVersion uint64
}

type BackendCapabilities struct {
	Transactions     bool
	CompareAndSwap   bool
	PrefixQuery      bool
	SecondaryIndexes bool
	UniqueIndexes    bool
	JSONPathQuery    bool
	OrderedScan      bool
	DurableHistory   bool
	SchemaSnapshots  bool
}

type ConflictKind string

const (
	VersionConflict       ConflictKind = "VersionConflict"
	PredicateConflict     ConflictKind = "PredicateConflict"
	UniqueIndexConflict   ConflictKind = "UniqueIndexConflict"
	SchemaConflict        ConflictKind = "SchemaConflict"
	LeaseConflict         ConflictKind = "LeaseConflict"
	QueueClaimConflict    ConflictKind = "QueueClaimConflict"
	WorkflowClaimConflict ConflictKind = "WorkflowClaimConflict"
)

type ConflictError struct {
	Kind    ConflictKind
	Message string
}

func (e ConflictError) Error() string {
	return string(e.Kind) + ": " + e.Message
}

var (
	ErrNotFound      = errors.New("not found")
	ErrUnsupported   = errors.New("unsupported backend feature")
	ErrInvalidSchema = errors.New("invalid schema")
)

type VersionedRecord struct {
	Collection CollectionName
	Key        Key
	Version    Version
	Document   JSON
}

type IndexValueKind string

const (
	IndexNull      IndexValueKind = "Null"
	IndexString    IndexValueKind = "String"
	IndexInteger   IndexValueKind = "Integer"
	IndexDecimal   IndexValueKind = "Decimal"
	IndexBoolean   IndexValueKind = "Boolean"
	IndexTimestamp IndexValueKind = "Timestamp"
)

type IndexValue struct {
	Kind  IndexValueKind
	Value JSON
}

func NullIndexValue() IndexValue {
	return IndexValue{Kind: IndexNull, Value: JSONNull()}
}

func StringIndexValue(value string) IndexValue {
	return IndexValue{Kind: IndexString, Value: JSONString(value)}
}

func IntegerIndexValue(value int64) IndexValue {
	return IndexValue{Kind: IndexInteger, Value: JSONInt(value)}
}

func DecimalIndexValue(value float64) (IndexValue, error) {
	jsonValue, err := JSONFloat(value)
	if err != nil {
		return IndexValue{}, err
	}
	return IndexValue{Kind: IndexDecimal, Value: jsonValue}, nil
}

func BooleanIndexValue(value bool) IndexValue {
	return IndexValue{Kind: IndexBoolean, Value: JSONBool(value)}
}

func TimestampIndexValue(value string) IndexValue {
	return IndexValue{Kind: IndexTimestamp, Value: JSONString(value)}
}

type IndexBound struct {
	Values    []IndexValue
	Inclusive bool
}

type QueryKind string

const (
	QueryAll         QueryKind = "All"
	QueryKeyPrefix   QueryKind = "KeyPrefix"
	QueryJSONEquals  QueryKind = "JSONEquals"
	QueryIndexEquals QueryKind = "IndexEquals"
	QueryIndexPrefix QueryKind = "IndexPrefix"
	QueryIndexRange  QueryKind = "IndexRange"
)

type Query struct {
	Kind        QueryKind
	KeyPrefix   *string
	JSONPath    *string
	JSONValue   *JSON
	IndexName   *string
	IndexValues []IndexValue
	LowerBound  *IndexBound
	UpperBound  *IndexBound
}

func AllQuery() Query {
	return Query{Kind: QueryAll}
}

func KeyPrefixQuery(prefix string) Query {
	return Query{Kind: QueryKeyPrefix, KeyPrefix: &prefix}
}

func JSONEqualsQuery(path string, value JSON) Query {
	return Query{Kind: QueryJSONEquals, JSONPath: &path, JSONValue: &value}
}

func IndexEqualsQuery(indexName string, values []IndexValue) Query {
	return Query{Kind: QueryIndexEquals, IndexName: &indexName, IndexValues: values}
}

func IndexPrefixQuery(indexName string, prefixValues []IndexValue) Query {
	return Query{Kind: QueryIndexPrefix, IndexName: &indexName, IndexValues: prefixValues}
}

func IndexRangeQuery(indexName string, lowerBound *IndexBound, upperBound *IndexBound) Query {
	return Query{Kind: QueryIndexRange, IndexName: &indexName, LowerBound: lowerBound, UpperBound: upperBound}
}

type Transaction interface {
	IsOpen() bool
	Abort(ctx context.Context) error
	Get(ctx context.Context, collection CollectionName, key Key) (*VersionedRecord, error)
	Query(ctx context.Context, collection CollectionName, query Query) ([]VersionedRecord, error)
	Put(ctx context.Context, collection CollectionName, key Key, document JSON) error
	Erase(ctx context.Context, collection CollectionName, key Key) error
}

type Backend interface {
	Capabilities(ctx context.Context) BackendCapabilities

	EnsureCollection(ctx context.Context, descriptor CollectionDescriptor) error

	EnsureCollections(ctx context.Context, descriptors []CollectionDescriptor) error

	Begin(ctx context.Context) (Transaction, error)

	Get(ctx context.Context, tx Transaction, collection CollectionName, key Key) (*VersionedRecord, error)

	Query(ctx context.Context, tx Transaction, collection CollectionName, query Query) ([]VersionedRecord, error)

	Put(ctx context.Context, tx Transaction, collection CollectionName, key Key, document JSON) error

	Erase(ctx context.Context, tx Transaction, collection CollectionName, key Key) error

	Commit(ctx context.Context, tx Transaction) error
}
