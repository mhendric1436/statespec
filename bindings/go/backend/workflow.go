package backend

import (
	"context"
	"time"
)

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
