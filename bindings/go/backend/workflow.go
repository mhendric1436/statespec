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
	RegisterDefinition(ctx context.Context, tx Transaction, request RegisterWorkflowDefinitionRequest) (WorkflowDefinitionRegistration, error)

	InspectDefinition(ctx context.Context, tx Transaction, workflowName string, workflowVersion int64) (*WorkflowDefinition, error)

	Start(ctx context.Context, tx Transaction, request StartWorkflowRequest) (WorkflowExecutionRecord, error)

	ClaimSteps(ctx context.Context, tx Transaction, request ClaimWorkflowStepRequest) ([]WorkflowExecutionRecord, error)

	CompleteStep(ctx context.Context, tx Transaction, request CompleteWorkflowStepRequest) (WorkflowExecutionRecord, error)

	FailStep(ctx context.Context, tx Transaction, request FailWorkflowStepRequest) (WorkflowExecutionRecord, error)

	Cancel(ctx context.Context, tx Transaction, request CancelWorkflowRequest) (WorkflowExecutionRecord, error)

	Inspect(ctx context.Context, tx Transaction, workflowExecutionID string) (*WorkflowExecutionRecord, error)
}

type WorkflowRuntime struct {
	Backend Backend
	Store   WorkflowStore
}

func NewWorkflowRuntime(backend Backend, store WorkflowStore) WorkflowRuntime {
	return WorkflowRuntime{Backend: backend, Store: store}
}

func (r WorkflowRuntime) RegisterDefinition(ctx context.Context, request RegisterWorkflowDefinitionRequest) (WorkflowDefinitionRegistration, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return WorkflowDefinitionRegistration{}, err
	}

	result, err := r.Store.RegisterDefinition(ctx, tx, request)
	if err != nil {
		_ = tx.Abort(ctx)
		return WorkflowDefinitionRegistration{}, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return WorkflowDefinitionRegistration{}, err
	}

	return result, nil
}

func (r WorkflowRuntime) InspectDefinition(ctx context.Context, workflowName string, workflowVersion int64) (*WorkflowDefinition, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return nil, err
	}

	result, err := r.Store.InspectDefinition(ctx, tx, workflowName, workflowVersion)
	if err != nil {
		_ = tx.Abort(ctx)
		return nil, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return nil, err
	}

	return result, nil
}

func (r WorkflowRuntime) Start(ctx context.Context, request StartWorkflowRequest) (WorkflowExecutionRecord, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return WorkflowExecutionRecord{}, err
	}

	result, err := r.Store.Start(ctx, tx, request)
	if err != nil {
		_ = tx.Abort(ctx)
		return WorkflowExecutionRecord{}, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return WorkflowExecutionRecord{}, err
	}

	return result, nil
}

func (r WorkflowRuntime) ClaimSteps(ctx context.Context, request ClaimWorkflowStepRequest) ([]WorkflowExecutionRecord, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return nil, err
	}

	result, err := r.Store.ClaimSteps(ctx, tx, request)
	if err != nil {
		_ = tx.Abort(ctx)
		return nil, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return nil, err
	}

	return result, nil
}

func (r WorkflowRuntime) CompleteStep(ctx context.Context, request CompleteWorkflowStepRequest) (WorkflowExecutionRecord, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return WorkflowExecutionRecord{}, err
	}

	result, err := r.Store.CompleteStep(ctx, tx, request)
	if err != nil {
		_ = tx.Abort(ctx)
		return WorkflowExecutionRecord{}, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return WorkflowExecutionRecord{}, err
	}

	return result, nil
}

func (r WorkflowRuntime) FailStep(ctx context.Context, request FailWorkflowStepRequest) (WorkflowExecutionRecord, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return WorkflowExecutionRecord{}, err
	}

	result, err := r.Store.FailStep(ctx, tx, request)
	if err != nil {
		_ = tx.Abort(ctx)
		return WorkflowExecutionRecord{}, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return WorkflowExecutionRecord{}, err
	}

	return result, nil
}

func (r WorkflowRuntime) Cancel(ctx context.Context, request CancelWorkflowRequest) (WorkflowExecutionRecord, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return WorkflowExecutionRecord{}, err
	}

	result, err := r.Store.Cancel(ctx, tx, request)
	if err != nil {
		_ = tx.Abort(ctx)
		return WorkflowExecutionRecord{}, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return WorkflowExecutionRecord{}, err
	}

	return result, nil
}

func (r WorkflowRuntime) Inspect(ctx context.Context, workflowExecutionID string) (*WorkflowExecutionRecord, error) {
	tx, err := r.Backend.Begin(ctx)
	if err != nil {
		return nil, err
	}

	result, err := r.Store.Inspect(ctx, tx, workflowExecutionID)
	if err != nil {
		_ = tx.Abort(ctx)
		return nil, err
	}

	if err := r.Backend.Commit(ctx, tx); err != nil {
		return nil, err
	}

	return result, nil
}
