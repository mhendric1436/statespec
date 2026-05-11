use std::time::{Duration, SystemTime};

use crate::backend::{Backend, BackendResult};

#[derive(Debug, Clone)]
pub struct LeaseRecord {
    pub resource: String,
    pub holder: Option<String>,
    pub expires_at: SystemTime,
    pub fencing_token: u64,
}

#[derive(Debug, Clone)]
pub struct LeaseAcquireRequest {
    pub resource: String,
    pub holder: String,
    pub now: SystemTime,
    pub ttl: Duration,
}

#[derive(Debug, Clone)]
pub struct LeaseRenewRequest {
    pub resource: String,
    pub holder: String,
    pub fencing_token: u64,
    pub now: SystemTime,
    pub ttl: Duration,
}

#[derive(Debug, Clone)]
pub struct LeaseReleaseRequest {
    pub resource: String,
    pub holder: String,
    pub fencing_token: u64,
}

#[derive(Debug, Clone)]
pub struct LeaseAcquireResult {
    pub acquired: bool,
    pub lease: Option<LeaseRecord>,
}

pub trait LeaseStore<B: Backend> {
    fn acquire(&self, backend: &B, request: &LeaseAcquireRequest) -> BackendResult<LeaseAcquireResult>;

    fn renew(&self, backend: &B, request: &LeaseRenewRequest) -> BackendResult<LeaseRecord>;

    fn release(&self, backend: &B, request: &LeaseReleaseRequest) -> BackendResult<()>;

    fn inspect(&self, backend: &B, resource: &str) -> BackendResult<Option<LeaseRecord>>;
}
