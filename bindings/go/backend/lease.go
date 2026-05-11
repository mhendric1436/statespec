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
	Acquire(ctx context.Context, tx Transaction, request LeaseAcquireRequest) (LeaseAcquireResult, error)

	Renew(ctx context.Context, tx Transaction, request LeaseRenewRequest) (LeaseRecord, error)

	Release(ctx context.Context, tx Transaction, request LeaseReleaseRequest) error

	Inspect(ctx context.Context, tx Transaction, resource string) (*LeaseRecord, error)
}

type LeaseRuntime struct {
	Backend Backend
	Store   LeaseStore
}

func NewLeaseRuntime(backend Backend, store LeaseStore) LeaseRuntime {
	return LeaseRuntime{Backend: backend, Store: store}
}

func (r LeaseRuntime) Acquire(ctx context.Context, request LeaseAcquireRequest) (LeaseAcquireResult, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return LeaseAcquireResult{}, err
	}

	result, err := r.Store.Acquire(ctx, tx, request)
	if err != nil {
		_ = tx.Abort(ctx)
		return LeaseAcquireResult{}, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return LeaseAcquireResult{}, err
	}

	return result, nil
}

func (r LeaseRuntime) Renew(ctx context.Context, request LeaseRenewRequest) (LeaseRecord, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return LeaseRecord{}, err
	}

	result, err := r.Store.Renew(ctx, tx, request)
	if err != nil {
		_ = tx.Abort(ctx)
		return LeaseRecord{}, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return LeaseRecord{}, err
	}

	return result, nil
}

func (r LeaseRuntime) Release(ctx context.Context, request LeaseReleaseRequest) error {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return err
	}

	if err := r.Store.Release(ctx, tx, request); err != nil {
		_ = tx.Abort(ctx)
		return err
	}

	return r.Backend.Commit(ctx, tx)
}

func (r LeaseRuntime) Inspect(ctx context.Context, resource string) (*LeaseRecord, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return nil, err
	}

	result, err := r.Store.Inspect(ctx, tx, resource)
	if err != nil {
		_ = tx.Abort(ctx)
		return nil, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return nil, err
	}

	return result, nil
}
