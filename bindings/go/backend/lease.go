package backend

import (
	"context"
	"time"
)

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
	Acquire(ctx context.Context, tx Transaction, request LeaseAcquireRequest) (LeaseAcquireResult, error)

	Renew(ctx context.Context, tx Transaction, request LeaseRenewRequest) (LeaseRecord, error)

	Release(ctx context.Context, tx Transaction, request LeaseReleaseRequest) error

	Inspect(ctx context.Context, tx Transaction, resource string) (*LeaseRecord, error)
}
