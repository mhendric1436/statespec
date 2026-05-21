use crate::backend::{Backend, BackendError, BackendResult, ConflictKind, Query};
use crate::memory_backend::InMemoryBackend;
use crate::memory_codec;
use crate::memory_transaction::definition_key;
use crate::workflow::{
    CancelWorkflowRequest, ClaimWorkflowStepRequest, CompleteWorkflowStepRequest,
    FailWorkflowStepRequest, KeepAliveWorkflowStepRequest, RegisterWorkflowDefinitionRequest,
    StartWorkflowRequest, WorkflowDefinition, WorkflowDefinitionRegistration,
    WorkflowExecutionRecord, WorkflowStore,
};

const DEFINITIONS: &str = "workflows.definitions";
const EXECUTIONS: &str = "workflows.executions";

#[derive(Debug, Clone, Default)]
pub struct InMemoryWorkflowStore;

impl InMemoryWorkflowStore {
    pub fn new() -> Self {
        Self
    }

    fn require_execution(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        workflow_execution_id: &str,
    ) -> BackendResult<WorkflowExecutionRecord> {
        self.inspect_tx(tx, workflow_execution_id)?
            .ok_or_else(|| BackendError::NotFound {
                message: "unknown workflow execution".to_string(),
            })
    }

    fn all_executions(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
    ) -> BackendResult<Vec<WorkflowExecutionRecord>> {
        Ok(tx
            .query(EXECUTIONS, &Query::All)?
            .iter()
            .map(|record| memory_codec::workflow_execution_from_json(&record.document))
            .collect())
    }
}

impl WorkflowStore<InMemoryBackend> for InMemoryWorkflowStore {
    fn register_definition(
        &self,
        backend: &InMemoryBackend,
        request: &RegisterWorkflowDefinitionRequest,
    ) -> BackendResult<WorkflowDefinitionRegistration> {
        let mut tx = backend.begin()?;
        let result = self.register_definition_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn register_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &RegisterWorkflowDefinitionRequest,
    ) -> BackendResult<WorkflowDefinitionRegistration> {
        let existing = self.inspect_definition_tx(
            tx,
            &request.definition.workflow_name,
            request.definition.workflow_version,
        )?;
        tx.put(
            DEFINITIONS,
            &workflow_definition_key(
                &request.definition.workflow_name,
                request.definition.workflow_version,
            ),
            memory_codec::workflow_definition_to_json(&request.definition),
        )?;
        Ok(WorkflowDefinitionRegistration {
            definition: request.definition.clone(),
            created: existing.is_none(),
        })
    }

    fn inspect_definition(
        &self,
        backend: &InMemoryBackend,
        workflow_name: &str,
        workflow_version: i64,
    ) -> BackendResult<Option<WorkflowDefinition>> {
        let mut tx = backend.begin()?;
        let result = self.inspect_definition_tx(&mut tx, workflow_name, workflow_version)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_definition_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        workflow_name: &str,
        workflow_version: i64,
    ) -> BackendResult<Option<WorkflowDefinition>> {
        let key = workflow_definition_key(workflow_name, workflow_version);
        Ok(tx
            .get(DEFINITIONS, &key)?
            .map(|record| memory_codec::workflow_definition_from_json(&record.document)))
    }

    fn start(
        &self,
        backend: &InMemoryBackend,
        request: &StartWorkflowRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        let mut tx = backend.begin()?;
        let result = self.start_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn start_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &StartWorkflowRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        if let Some(existing) = self.inspect_tx(tx, &request.workflow_execution_id)? {
            return Ok(existing);
        }
        let record = WorkflowExecutionRecord {
            workflow_execution_id: request.workflow_execution_id.clone(),
            workflow_name: request.workflow_name.clone(),
            workflow_version: request.workflow_version,
            current_step: request.start_step.clone(),
            status: "Running".to_string(),
            attempt: 0,
            claimed_by: None,
            claim_expires_at: None,
            state: request.state.clone(),
        };
        tx.put(
            EXECUTIONS,
            &record.workflow_execution_id,
            memory_codec::workflow_execution_to_json(&record),
        )?;
        Ok(record)
    }

    fn claim_steps(
        &self,
        backend: &InMemoryBackend,
        request: &ClaimWorkflowStepRequest,
    ) -> BackendResult<Vec<WorkflowExecutionRecord>> {
        let mut tx = backend.begin()?;
        let result = self.claim_steps_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn claim_steps_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &ClaimWorkflowStepRequest,
    ) -> BackendResult<Vec<WorkflowExecutionRecord>> {
        let mut claimed = Vec::new();
        for mut execution in self.all_executions(tx)? {
            if claimed.len() >= request.max_steps as usize {
                break;
            }
            if execution.workflow_execution_id != request.workflow_execution_id
                || execution.workflow_name != request.workflow_name
                || execution.workflow_version != request.workflow_version
                || execution.status != "Running"
            {
                continue;
            }
            let visible = execution.claimed_by.is_none()
                || execution
                    .claim_expires_at
                    .map(|expires| expires <= request.now)
                    .unwrap_or(false);
            if !visible {
                continue;
            }
            execution.claimed_by = Some(request.worker.clone());
            execution.claim_expires_at = Some(request.now + request.lease_duration);
            execution.attempt += 1;
            tx.put(
                EXECUTIONS,
                &execution.workflow_execution_id,
                memory_codec::workflow_execution_to_json(&execution),
            )?;
            claimed.push(execution);
        }
        Ok(claimed)
    }

    fn keep_alive_step(
        &self,
        backend: &InMemoryBackend,
        request: &KeepAliveWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        let mut tx = backend.begin()?;
        let result = self.keep_alive_step_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn keep_alive_step_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &KeepAliveWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        let mut execution = self.require_execution(tx, &request.workflow_execution_id)?;
        require_claim(&execution, &request.worker, &request.current_step)?;
        execution.claim_expires_at = Some(request.now + request.lease_duration);
        tx.put(
            EXECUTIONS,
            &execution.workflow_execution_id,
            memory_codec::workflow_execution_to_json(&execution),
        )?;
        Ok(execution)
    }

    fn complete_step(
        &self,
        backend: &InMemoryBackend,
        request: &CompleteWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        let mut tx = backend.begin()?;
        let result = self.complete_step_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn complete_step_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &CompleteWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        let mut execution = self.require_execution(tx, &request.workflow_execution_id)?;
        require_claim(&execution, &request.worker, &request.completed_step)?;
        execution.state = request.state.clone();
        execution.claimed_by = None;
        execution.claim_expires_at = None;
        if let Some(next_step) = &request.next_step {
            execution.current_step = next_step.clone();
            execution.status = "Running".to_string();
        } else {
            execution.status = "Completed".to_string();
        }
        tx.put(
            EXECUTIONS,
            &execution.workflow_execution_id,
            memory_codec::workflow_execution_to_json(&execution),
        )?;
        Ok(execution)
    }

    fn fail_step(
        &self,
        backend: &InMemoryBackend,
        request: &FailWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        let mut tx = backend.begin()?;
        let result = self.fail_step_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn fail_step_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &FailWorkflowStepRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        let mut execution = self.require_execution(tx, &request.workflow_execution_id)?;
        require_claim(&execution, &request.worker, &request.failed_step)?;
        execution.claimed_by = None;
        execution.claim_expires_at = None;
        execution.status = if execution.attempt >= request.max_attempts as u64 {
            "Failed".to_string()
        } else {
            "Running".to_string()
        };
        tx.put(
            EXECUTIONS,
            &execution.workflow_execution_id,
            memory_codec::workflow_execution_to_json(&execution),
        )?;
        Ok(execution)
    }

    fn cancel(
        &self,
        backend: &InMemoryBackend,
        request: &CancelWorkflowRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        let mut tx = backend.begin()?;
        let result = self.cancel_tx(&mut tx, request)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn cancel_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        request: &CancelWorkflowRequest,
    ) -> BackendResult<WorkflowExecutionRecord> {
        let mut execution = self.require_execution(tx, &request.workflow_execution_id)?;
        execution.status = "Canceled".to_string();
        execution.claimed_by = None;
        execution.claim_expires_at = None;
        tx.put(
            EXECUTIONS,
            &execution.workflow_execution_id,
            memory_codec::workflow_execution_to_json(&execution),
        )?;
        Ok(execution)
    }

    fn inspect(
        &self,
        backend: &InMemoryBackend,
        workflow_execution_id: &str,
    ) -> BackendResult<Option<WorkflowExecutionRecord>> {
        let mut tx = backend.begin()?;
        let result = self.inspect_tx(&mut tx, workflow_execution_id)?;
        backend.commit(tx)?;
        Ok(result)
    }

    fn inspect_tx(
        &self,
        tx: &mut <InMemoryBackend as Backend>::Tx,
        workflow_execution_id: &str,
    ) -> BackendResult<Option<WorkflowExecutionRecord>> {
        Ok(tx
            .get(EXECUTIONS, workflow_execution_id)?
            .map(|record| memory_codec::workflow_execution_from_json(&record.document)))
    }
}

fn workflow_definition_key(name: &str, version: i64) -> String {
    definition_key(&[name.to_string(), version.to_string()])
}

fn require_claim(
    execution: &WorkflowExecutionRecord,
    worker: &str,
    step: &str,
) -> BackendResult<()> {
    if execution.claimed_by.as_deref() != Some(worker) || execution.current_step != step {
        return Err(BackendError::Conflict {
            kind: ConflictKind::WorkflowClaimConflict,
            message: "workflow step is not claimed by caller".to_string(),
        });
    }
    Ok(())
}
