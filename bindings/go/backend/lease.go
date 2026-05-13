package backend

import (
	"context"
	"time"
)

type LeaseDefinitionID struct {
	Name    string
	Version uint64
}

type LeaseDefinition struct {
	ID              LeaseDefinitionID
	ResourcePattern string
	TTL             time.Duration
	RenewEvery      time.Duration
	MaxTTL          *time.Duration
	FencingToken    bool
}

type LeaseRegisterDefinitionResult struct {
	RegisteredNew bool
	Definition    LeaseDefinition
}

type LeaseRecord struct {
	DefinitionID LeaseDefinitionID
	Resource     string
	Holder       *string
	ExpiresAt    time.Time
	FencingToken uint64
}

type LeaseAcquireRequest struct {
	DefinitionID LeaseDefinitionID
	Resource     string
	Holder       string
	Now          time.Time
}

type LeaseRenewRequest struct {
	DefinitionID LeaseDefinitionID
	Resource     string
	Holder       string
	FencingToken uint64
	Now          time.Time
}

type LeaseReleaseRequest struct {
	DefinitionID LeaseDefinitionID
	Resource     string
	Holder       string
	FencingToken uint64
}

type LeaseInspectRequest struct {
	DefinitionID LeaseDefinitionID
	Resource     string
}

type LeaseAcquireResult struct {
	Acquired bool
	Lease    *LeaseRecord
}

type LeaseStore interface {
	RegisterDefinition(ctx context.Context, backend Backend, definition LeaseDefinition) (LeaseRegisterDefinitionResult, error)

	RegisterDefinitionTx(ctx context.Context, tx Transaction, definition LeaseDefinition) (LeaseRegisterDefinitionResult, error)

	InspectDefinition(ctx context.Context, backend Backend, definitionID LeaseDefinitionID) (*LeaseDefinition, error)

	InspectDefinitionTx(ctx context.Context, tx Transaction, definitionID LeaseDefinitionID) (*LeaseDefinition, error)

	Acquire(ctx context.Context, backend Backend, request LeaseAcquireRequest) (LeaseAcquireResult, error)

	AcquireTx(ctx context.Context, tx Transaction, request LeaseAcquireRequest) (LeaseAcquireResult, error)

	Renew(ctx context.Context, backend Backend, request LeaseRenewRequest) (LeaseRecord, error)

	RenewTx(ctx context.Context, tx Transaction, request LeaseRenewRequest) (LeaseRecord, error)

	Release(ctx context.Context, backend Backend, request LeaseReleaseRequest) error

	ReleaseTx(ctx context.Context, tx Transaction, request LeaseReleaseRequest) error

	Inspect(ctx context.Context, backend Backend, request LeaseInspectRequest) (*LeaseRecord, error)

	InspectTx(ctx context.Context, tx Transaction, request LeaseInspectRequest) (*LeaseRecord, error)
}
