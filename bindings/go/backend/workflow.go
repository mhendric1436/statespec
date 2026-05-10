package backend

import (
	"context"
	"time"
)

type WorkflowExecutionRecord struct {
	WorkflowExecutionID string
	WorkflowName        string
	WorkflowVersion     int64
	CurrentStep         string
	Status              string
	Attempt             uint64
	ClaimedBy           *string
	ClaimExpiresAt      *time.Time
	State               JSON
}

type StartWorkflowRequest struct {
	WorkflowExecutionID string
	WorkflowName        string
	WorkflowVersion     int64
	StartStep           string
	State               JSON
}

type ClaimWorkflowStepRequest struct {
	WorkflowName    string
	WorkflowVersion int64
	Worker          string
	Now             time.Time
	LeaseDuration   time.Duration
	MaxSteps        uint32
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
