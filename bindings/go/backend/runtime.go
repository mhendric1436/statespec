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

type StartWorkflowRequest struct {
	WorkflowExecutionID string
	WorkflowName        string
	StartStep           string
	State               JSON
}

type ClaimWorkflowStepRequest struct {
	WorkflowName  string
	Worker        string
	Now           time.Time
	LeaseDuration time.Duration
	MaxSteps      uint32
}

type CompleteWorkflowStepRequest struct {
	WorkflowExecutionID string
	Worker              string
	CompletedStep       string
	NextStep            *string
	State               JSON
}

type FailWorkflowStepRequest struct {
	WorkflowExecutionID string
	Worker              string
	FailedStep          string
	Reason              string
	Now                 time.Time
	MaxAttempts         uint32
}

type CancelWorkflowRequest struct {
	WorkflowExecutionID string
	Reason              string
}

type WorkflowStore interface {
	Start(ctx context.Context, tx Transaction, request StartWorkflowRequest) (WorkflowExecutionRecord, error)

	ClaimSteps(ctx context.Context, tx Transaction, request ClaimWorkflowStepRequest) ([]WorkflowExecutionRecord, error)

	CompleteStep(ctx context.Context, tx Transaction, request CompleteWorkflowStepRequest) (WorkflowExecutionRecord, error)

	FailStep(ctx context.Context, tx Transaction, request FailWorkflowStepRequest) (WorkflowExecutionRecord, error)

	Cancel(ctx context.Context, tx Transaction, request CancelWorkflowRequest) (WorkflowExecutionRecord, error)

	Inspect(ctx context.Context, tx Transaction, workflowExecutionID string) (*WorkflowExecutionRecord, error)
}
