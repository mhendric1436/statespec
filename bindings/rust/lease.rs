use std::time::{Duration, SystemTime};

use crate::backend::{BackendResult, LeaseRecord, Transaction};

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

pub trait LeaseStore<Tx: Transaction> {
    fn acquire(&self, tx: &mut Tx, request: &LeaseAcquireRequest) -> BackendResult<LeaseAcquireResult>;

    fn renew(&self, tx: &mut Tx, request: &LeaseRenewRequest) -> BackendResult<LeaseRecord>;

    fn release(&self, tx: &mut Tx, request: &LeaseReleaseRequest) -> BackendResult<()>;

    fn inspect(&self, tx: &mut Tx, resource: &str) -> BackendResult<Option<LeaseRecord>>;
}
