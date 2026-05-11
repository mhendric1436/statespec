package backend

import (
	"context"
	"time"
)

type WorkflowStepDefinition struct {
	Name                  string
	ExpectedExecutionTime time.Duration
	MaxRetries            uint32
}

type WorkflowDefinition struct {
	WorkflowName          string
	WorkflowVersion       int64
	StartStep             string
	ExpectedExecutionTime time.Duration
	Singleton             bool
	Steps                 []WorkflowStepDefinition
	Metadata              JSON
}

type RegisterWorkflowDefinitionRequest struct {
	Definition WorkflowDefinition
}

type WorkflowDefinitionRegistration struct {
	Definition WorkflowDefinition
	Created    bool
}

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
	WorkflowExecutionID string
	WorkflowName        string
	WorkflowVersion     int64
	Worker              string
	Now                 time.Time
	LeaseDuration       time.Duration
	MaxSteps            uint32
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
	RegisterDefinition(ctx context.Context, backend Backend, request RegisterWorkflowDefinitionRequest) (WorkflowDefinitionRegistration, error)

	InspectDefinition(ctx context.Context, backend Backend, workflowName string, workflowVersion int64) (*WorkflowDefinition, error)

	Start(ctx context.Context, backend Backend, request StartWorkflowRequest) (WorkflowExecutionRecord, error)

	ClaimSteps(ctx context.Context, backend Backend, request ClaimWorkflowStepRequest) ([]WorkflowExecutionRecord, error)

	CompleteStep(ctx context.Context, backend Backend, request CompleteWorkflowStepRequest) (WorkflowExecutionRecord, error)

	FailStep(ctx context.Context, backend Backend, request FailWorkflowStepRequest) (WorkflowExecutionRecord, error)

	Cancel(ctx context.Context, backend Backend, request CancelWorkflowRequest) (WorkflowExecutionRecord, error)

	Inspect(ctx context.Context, backend Backend, workflowExecutionID string) (*WorkflowExecutionRecord, error)
}
