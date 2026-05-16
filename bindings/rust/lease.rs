use std::time::{Duration, SystemTime};

use crate::backend::{Backend, BackendResult};

#[derive(Debug, Clone)]
pub struct LeaseDefinitionId {
    pub name: String,
    pub version: u64,
}

#[derive(Debug, Clone)]
pub struct LeaseDefinition {
    pub id: LeaseDefinitionId,
    pub resource_pattern: String,
    pub ttl: Duration,
    pub renew_every: Duration,
    pub max_ttl: Option<Duration>,
    pub fencing_token: bool,
}

#[derive(Debug, Clone)]
pub struct LeaseRegisterDefinitionResult {
    pub registered_new: bool,
    pub definition: LeaseDefinition,
}

#[derive(Debug, Clone)]
pub struct LeaseRecord {
    pub definition_id: LeaseDefinitionId,
    pub resource: String,
    pub holder: Option<String>,
    pub expires_at: SystemTime,
    pub fencing_token: u64,
}

#[derive(Debug, Clone)]
pub struct LeaseAcquireRequest {
    pub definition_id: LeaseDefinitionId,
    pub resource: String,
    pub holder: String,
    pub now: SystemTime,
}

#[derive(Debug, Clone)]
pub struct LeaseRenewRequest {
    pub definition_id: LeaseDefinitionId,
    pub resource: String,
    pub holder: String,
    pub fencing_token: u64,
    pub now: SystemTime,
}

#[derive(Debug, Clone)]
pub struct LeaseReleaseRequest {
    pub definition_id: LeaseDefinitionId,
    pub resource: String,
    pub holder: String,
    pub fencing_token: u64,
}

#[derive(Debug, Clone)]
pub struct LeaseInspectRequest {
    pub definition_id: LeaseDefinitionId,
    pub resource: String,
}

#[derive(Debug, Clone)]
pub struct LeaseAcquireResult {
    pub acquired: bool,
    pub lease: Option<LeaseRecord>,
}

pub trait LeaseStore<B: Backend> {
    fn register_definition(
        &self,
        backend: &B,
        definition: &LeaseDefinition,
    ) -> BackendResult<LeaseRegisterDefinitionResult>;

    fn register_definition_tx(
        &self,
        tx: &mut B::Tx,
        definition: &LeaseDefinition,
    ) -> BackendResult<LeaseRegisterDefinitionResult>;

    fn inspect_definition(
        &self,
        backend: &B,
        definition_id: &LeaseDefinitionId,
    ) -> BackendResult<Option<LeaseDefinition>>;

    fn inspect_definition_tx(
        &self,
        tx: &mut B::Tx,
        definition_id: &LeaseDefinitionId,
    ) -> BackendResult<Option<LeaseDefinition>>;

    fn acquire(&self, backend: &B, request: &LeaseAcquireRequest) -> BackendResult<LeaseAcquireResult>;

    fn acquire_tx(&self, tx: &mut B::Tx, request: &LeaseAcquireRequest) -> BackendResult<LeaseAcquireResult>;

    fn renew(&self, backend: &B, request: &LeaseRenewRequest) -> BackendResult<LeaseRecord>;

    fn renew_tx(&self, tx: &mut B::Tx, request: &LeaseRenewRequest) -> BackendResult<LeaseRecord>;

    fn release(&self, backend: &B, request: &LeaseReleaseRequest) -> BackendResult<()>;

    fn release_tx(&self, tx: &mut B::Tx, request: &LeaseReleaseRequest) -> BackendResult<()>;

    fn inspect(&self, backend: &B, request: &LeaseInspectRequest) -> BackendResult<Option<LeaseRecord>>;

    fn inspect_tx(
        &self,
        tx: &mut B::Tx,
        request: &LeaseInspectRequest,
    ) -> BackendResult<Option<LeaseRecord>>;
}
