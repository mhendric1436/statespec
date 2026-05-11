package backend

import (
	"context"
	"time"
)

type QueueDefinition struct {
	Queue             string
	Channel           string
	VisibilityTimeout time.Duration
	MaxAttempts       uint32
	DeadLetterQueue   *string
	Metadata          JSON
}

type CreateQueueRequest struct {
	Definition QueueDefinition
}

type QueueCreation struct {
	Definition QueueDefinition
	Created    bool
}

type QueueMessageRecord struct {
	MessageID      string
	Queue          string
	Channel        string
	Status         string
	Attempts       uint64
	ClaimedBy      *string
	ClaimExpiresAt *time.Time
	Payload         JSON
}

type EnqueueMessageRequest struct {
	MessageID      string
	Queue          string
	Channel        string
	IdempotencyKey *string
	Payload         JSON
}

type ClaimMessageRequest struct {
	Queue             string
	Channel           string
	Claimant          string
	Now               time.Time
	VisibilityTimeout time.Duration
	MaxMessages       uint32
}

type AckMessageRequest struct {
	MessageID string
	Claimant  string
}

type FailMessageRequest struct {
	MessageID   string
	Claimant    string
	Reason      string
	Now         time.Time
	MaxAttempts uint32
}

type QueueStore interface {
	Create(ctx context.Context, tx Transaction, request CreateQueueRequest) (QueueCreation, error)

	InspectDefinition(ctx context.Context, tx Transaction, queue string, channel string) (*QueueDefinition, error)

	Enqueue(ctx context.Context, tx Transaction, request EnqueueMessageRequest) (QueueMessageRecord, error)

	Claim(ctx context.Context, tx Transaction, request ClaimMessageRequest) ([]QueueMessageRecord, error)

	Acknowledge(ctx context.Context, tx Transaction, request AckMessageRequest) error

	Fail(ctx context.Context, tx Transaction, request FailMessageRequest) (QueueMessageRecord, error)

	Inspect(ctx context.Context, tx Transaction, messageID string) (*QueueMessageRecord, error)
}

type QueueRuntime struct {
	Backend Backend
	Store   QueueStore
}

func NewQueueRuntime(backend Backend, store QueueStore) QueueRuntime {
	return QueueRuntime{Backend: backend, Store: store}
}

func (r QueueRuntime) Create(ctx context.Context, request CreateQueueRequest) (QueueCreation, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return QueueCreation{}, err
	}

	result, err := r.Store.Create(ctx, tx, request)
	if err != nil {
		_ = tx.Abort(ctx)
		return QueueCreation{}, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return QueueCreation{}, err
	}

	return result, nil
}

func (r QueueRuntime) InspectDefinition(ctx context.Context, queue string, channel string) (*QueueDefinition, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return nil, err
	}

	result, err := r.Store.InspectDefinition(ctx, tx, queue, channel)
	if err != nil {
		_ = tx.Abort(ctx)
		return nil, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return nil, err
	}

	return result, nil
}

func (r QueueRuntime) Enqueue(ctx context.Context, request EnqueueMessageRequest) (QueueMessageRecord, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return QueueMessageRecord{}, err
	}

	result, err := r.Store.Enqueue(ctx, tx, request)
	if err != nil {
		_ = tx.Abort(ctx)
		return QueueMessageRecord{}, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return QueueMessageRecord{}, err
	}

	return result, nil
}

func (r QueueRuntime) Claim(ctx context.Context, request ClaimMessageRequest) ([]QueueMessageRecord, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return nil, err
	}

	result, err := r.Store.Claim(ctx, tx, request)
	if err != nil {
		_ = tx.Abort(ctx)
		return nil, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return nil, err
	}

	return result, nil
}

func (r QueueRuntime) Acknowledge(ctx context.Context, request AckMessageRequest) error {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return err
	}

	if err := r.Store.Acknowledge(ctx, tx, request); err != nil {
		_ = tx.Abort(ctx)
		return err
	}

	return r.Backend.Commit(ctx, tx)
}

func (r QueueRuntime) Fail(ctx context.Context, request FailMessageRequest) (QueueMessageRecord, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return QueueMessageRecord{}, err
	}

	result, err := r.Store.Fail(ctx, tx, request)
	if err != nil {
		_ = tx.Abort(ctx)
		return QueueMessageRecord{}, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return QueueMessageRecord{}, err
	}

	return result, nil
}

func (r QueueRuntime) Inspect(ctx context.Context, messageID string) (*QueueMessageRecord, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return nil, err
	}

	result, err := r.Store.Inspect(ctx, tx, messageID)
	if err != nil {
		_ = tx.Abort(ctx)
		return nil, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return nil, err
	}

	return result, nil
}
