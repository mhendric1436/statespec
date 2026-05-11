use std::time::{Duration, SystemTime};

use crate::backend::{Backend, BackendResult, Transaction};

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

pub trait LeaseStore<Tx: Transaction> {
    fn acquire(&self, tx: &mut Tx, request: &LeaseAcquireRequest) -> BackendResult<LeaseAcquireResult>;

    fn renew(&self, tx: &mut Tx, request: &LeaseRenewRequest) -> BackendResult<LeaseRecord>;

    fn release(&self, tx: &mut Tx, request: &LeaseReleaseRequest) -> BackendResult<()>;

    fn inspect(&self, tx: &mut Tx, resource: &str) -> BackendResult<Option<LeaseRecord>>;
}

pub struct LeaseRuntime<'a, B, S>
where
    B: Backend,
    S: LeaseStore<B::Tx>,
{
    backend: &'a B,
    store: &'a S,
}

impl<'a, B, S> LeaseRuntime<'a, B, S>
where
    B: Backend,
    S: LeaseStore<B::Tx>,
{
    pub fn new(backend: &'a B, store: &'a S) -> Self {
        Self { backend, store }
    }

    pub fn acquire(&self, request: &LeaseAcquireRequest) -> BackendResult<LeaseAcquireResult> {
        let mut tx = self.backend.begin()?;
        match self.store.acquire(&mut tx, request) {
            Ok(result) => {
                self.backend.commit(tx)?;
                Ok(result)
            }
            Err(err) => {
                let _ = tx.abort();
                Err(err)
            }
        }
    }

    pub fn renew(&self, request: &LeaseRenewRequest) -> BackendResult<LeaseRecord> {
        let mut tx = self.backend.begin()?;
        match self.store.renew(&mut tx, request) {
            Ok(result) => {
                self.backend.commit(tx)?;
                Ok(result)
            }
            Err(err) => {
                let _ = tx.abort();
                Err(err)
            }
        }
    }

    pub fn release(&self, request: &LeaseReleaseRequest) -> BackendResult<()> {
        let mut tx = self.backend.begin()?;
        match self.store.release(&mut tx, request) {
            Ok(()) => self.backend.commit(tx),
            Err(err) => {
                let _ = tx.abort();
                Err(err)
            }
        }
    }

    pub fn inspect(&self, resource: &str) -> BackendResult<Option<LeaseRecord>> {
        let mut tx = self.backend.begin()?;
        match self.store.inspect(&mut tx, resource) {
            Ok(result) => {
                self.backend.commit(tx)?;
                Ok(result)
            }
            Err(err) => {
                let _ = tx.abort();
                Err(err)
            }
        }
    }
}
