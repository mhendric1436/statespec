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

type KeepAliveWorkflowStepRequest struct {
	WorkflowExecutionID string
	Worker              string
	CurrentStep         string
	Now                 time.Time
	LeaseDuration       time.Duration
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

	RegisterDefinitionTx(ctx context.Context, tx Transaction, request RegisterWorkflowDefinitionRequest) (WorkflowDefinitionRegistration, error)

	InspectDefinition(ctx context.Context, backend Backend, workflowName string, workflowVersion int64) (*WorkflowDefinition, error)

	InspectDefinitionTx(ctx context.Context, tx Transaction, workflowName string, workflowVersion int64) (*WorkflowDefinition, error)

	Start(ctx context.Context, backend Backend, request StartWorkflowRequest) (WorkflowExecutionRecord, error)

	StartTx(ctx context.Context, tx Transaction, request StartWorkflowRequest) (WorkflowExecutionRecord, error)

	ClaimSteps(ctx context.Context, backend Backend, request ClaimWorkflowStepRequest) ([]WorkflowExecutionRecord, error)

	ClaimStepsTx(ctx context.Context, tx Transaction, request ClaimWorkflowStepRequest) ([]WorkflowExecutionRecord, error)

	KeepAliveStep(ctx context.Context, backend Backend, request KeepAliveWorkflowStepRequest) (WorkflowExecutionRecord, error)

	KeepAliveStepTx(ctx context.Context, tx Transaction, request KeepAliveWorkflowStepRequest) (WorkflowExecutionRecord, error)

	CompleteStep(ctx context.Context, backend Backend, request CompleteWorkflowStepRequest) (WorkflowExecutionRecord, error)

	CompleteStepTx(ctx context.Context, tx Transaction, request CompleteWorkflowStepRequest) (WorkflowExecutionRecord, error)

	FailStep(ctx context.Context, backend Backend, request FailWorkflowStepRequest) (WorkflowExecutionRecord, error)

	FailStepTx(ctx context.Context, tx Transaction, request FailWorkflowStepRequest) (WorkflowExecutionRecord, error)

	Cancel(ctx context.Context, backend Backend, request CancelWorkflowRequest) (WorkflowExecutionRecord, error)

	CancelTx(ctx context.Context, tx Transaction, request CancelWorkflowRequest) (WorkflowExecutionRecord, error)

	Inspect(ctx context.Context, backend Backend, workflowExecutionID string) (*WorkflowExecutionRecord, error)

	InspectTx(ctx context.Context, tx Transaction, workflowExecutionID string) (*WorkflowExecutionRecord, error)
}
