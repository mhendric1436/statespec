package backend

import (
	"context"
	"time"
)

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
	Enqueue(ctx context.Context, tx Transaction, request EnqueueMessageRequest) (QueueMessageRecord, error)

	Claim(ctx context.Context, tx Transaction, request ClaimMessageRequest) ([]QueueMessageRecord, error)

	Acknowledge(ctx context.Context, tx Transaction, request AckMessageRequest) error

	Fail(ctx context.Context, tx Transaction, request FailMessageRequest) (QueueMessageRecord, error)

	Inspect(ctx context.Context, tx Transaction, messageID string) (*QueueMessageRecord, error)
}
