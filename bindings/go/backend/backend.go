package backend

import (
	"context"
	"errors"
)

type CollectionName string
type Key string
type Version uint64

type JSON []byte

type FieldDescriptor struct {
	Name     string
	Type     string
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
	Transactions      bool
	CompareAndSwap    bool
	PrefixQuery       bool
	SecondaryIndexes  bool
	UniqueIndexes     bool
	JSONPathQuery     bool
	OrderedScan       bool
	DurableHistory    bool
	SchemaSnapshots   bool
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

type QueryKind string

const (
	QueryAll        QueryKind = "All"
	QueryKeyPrefix  QueryKind = "KeyPrefix"
	QueryJSONEquals QueryKind = "JSONEquals"
)

type Query struct {
	Kind      QueryKind
	KeyPrefix *string
	JSONPath  *string
	JSONValue *JSON
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

type Transaction interface {
	IsOpen() bool
	Abort(ctx context.Context) error
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
