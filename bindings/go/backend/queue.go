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
	Create(ctx context.Context, backend Backend, request CreateQueueRequest) (QueueCreation, error)

	CreateTx(ctx context.Context, tx Transaction, request CreateQueueRequest) (QueueCreation, error)

	InspectDefinition(ctx context.Context, backend Backend, queue string, channel string) (*QueueDefinition, error)

	InspectDefinitionTx(ctx context.Context, tx Transaction, queue string, channel string) (*QueueDefinition, error)

	Enqueue(ctx context.Context, backend Backend, request EnqueueMessageRequest) (QueueMessageRecord, error)

	EnqueueTx(ctx context.Context, tx Transaction, request EnqueueMessageRequest) (QueueMessageRecord, error)

	Claim(ctx context.Context, backend Backend, request ClaimMessageRequest) ([]QueueMessageRecord, error)

	ClaimTx(ctx context.Context, tx Transaction, request ClaimMessageRequest) ([]QueueMessageRecord, error)

	Acknowledge(ctx context.Context, backend Backend, request AckMessageRequest) error

	AcknowledgeTx(ctx context.Context, tx Transaction, request AckMessageRequest) error

	Fail(ctx context.Context, backend Backend, request FailMessageRequest) (QueueMessageRecord, error)

	FailTx(ctx context.Context, tx Transaction, request FailMessageRequest) (QueueMessageRecord, error)

	Inspect(ctx context.Context, backend Backend, messageID string) (*QueueMessageRecord, error)

	InspectTx(ctx context.Context, tx Transaction, messageID string) (*QueueMessageRecord, error)
}
