package backend

import (
	"context"
	"time"
)

type LeaseRecord struct {
	Resource     string
	Holder       *string
	ExpiresAt    time.Time
	FencingToken uint64
}

type LeaseAcquireRequest struct {
	Resource string
	Holder   string
	Now      time.Time
	TTL      time.Duration
}

type LeaseRenewRequest struct {
	Resource     string
	Holder       string
	FencingToken uint64
	Now          time.Time
	TTL          time.Duration
}

type LeaseReleaseRequest struct {
	Resource     string
	Holder       string
	FencingToken uint64
}

type LeaseAcquireResult struct {
	Acquired bool
	Lease    *LeaseRecord
}

type LeaseStore interface {
	Acquire(ctx context.Context, backend Backend, request LeaseAcquireRequest) (LeaseAcquireResult, error)

	AcquireTx(ctx context.Context, tx Transaction, request LeaseAcquireRequest) (LeaseAcquireResult, error)

	Renew(ctx context.Context, backend Backend, request LeaseRenewRequest) (LeaseRecord, error)

	RenewTx(ctx context.Context, tx Transaction, request LeaseRenewRequest) (LeaseRecord, error)

	Release(ctx context.Context, backend Backend, request LeaseReleaseRequest) error

	ReleaseTx(ctx context.Context, tx Transaction, request LeaseReleaseRequest) error

	Inspect(ctx context.Context, backend Backend, resource string) (*LeaseRecord, error)

	InspectTx(ctx context.Context, tx Transaction, resource string) (*LeaseRecord, error)
}
