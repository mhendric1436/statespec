use std::time::SystemTime;

pub type CollectionName = String;
pub type Key = String;
pub type Version = u64;
pub type Json = String;

#[derive(Debug, Clone)]
pub struct FieldDescriptor {
    pub name: String,
    pub field_type: String,
    pub required: bool,
}

#[derive(Debug, Clone)]
pub struct IndexDescriptor {
    pub name: String,
    pub fields: Vec<String>,
    pub unique: bool,
}

#[derive(Debug, Clone)]
pub struct CollectionDescriptor {
    pub name: String,
    pub fields: Vec<FieldDescriptor>,
    pub key_fields: Vec<String>,
    pub indexes: Vec<IndexDescriptor>,
    pub schema_version: u64,
}

#[derive(Debug, Clone, Default)]
pub struct BackendCapabilities {
    pub transactions: bool,
    pub compare_and_swap: bool,
    pub prefix_query: bool,
    pub secondary_indexes: bool,
    pub unique_indexes: bool,
    pub json_path_query: bool,
    pub ordered_scan: bool,
    pub durable_history: bool,
    pub schema_snapshots: bool,
}

#[derive(Debug, Clone)]
pub enum ConflictKind {
    VersionConflict,
    PredicateConflict,
    UniqueIndexConflict,
    SchemaConflict,
    LeaseConflict,
    QueueClaimConflict,
    WorkflowClaimConflict,
}

#[derive(Debug)]
pub enum BackendError {
    Conflict { kind: ConflictKind, message: String },
    NotFound { message: String },
    Unsupported { message: String },
    InvalidSchema { message: String },
    Internal { message: String },
}

pub type BackendResult<T> = Result<T, BackendError>;

#[derive(Debug, Clone)]
pub struct VersionedRecord {
    pub collection: CollectionName,
    pub key: Key,
    pub version: Version,
    pub document: Json,
}

#[derive(Debug, Clone)]
pub enum Query {
    All,
    KeyPrefix { prefix: String },
    JsonEquals { path: String, value: Json },
}

pub trait Transaction {
    fn is_open(&self) -> bool;
    fn abort(&mut self) -> BackendResult<()>;
}

pub trait Backend {
    type Tx: Transaction;

    fn capabilities(&self) -> BackendCapabilities;

    fn ensure_collection(&self, descriptor: &CollectionDescriptor) -> BackendResult<()>;

    fn begin(&self) -> BackendResult<Self::Tx>;

    fn get(
        &self,
        tx: &mut Self::Tx,
        collection: &str,
        key: &str,
    ) -> BackendResult<Option<VersionedRecord>>;

    fn query(
        &self,
        tx: &mut Self::Tx,
        collection: &str,
        query: &Query,
    ) -> BackendResult<Vec<VersionedRecord>>;

    fn put(
        &self,
        tx: &mut Self::Tx,
        collection: &str,
        key: &str,
        document: Json,
    ) -> BackendResult<()>;

    fn erase(
        &self,
        tx: &mut Self::Tx,
        collection: &str,
        key: &str,
    ) -> BackendResult<()>;

    fn commit(&self, tx: Self::Tx) -> BackendResult<()>;
}

#[derive(Debug, Clone)]
pub struct LeaseRecord {
    pub resource: String,
    pub holder: Option<String>,
    pub expires_at: SystemTime,
    pub fencing_token: u64,
}

#[derive(Debug, Clone)]
pub struct QueueMessageRecord {
    pub message_id: String,
    pub queue: String,
    pub channel: String,
    pub status: String,
    pub attempts: u64,
    pub claimed_by: Option<String>,
    pub claim_expires_at: Option<SystemTime>,
    pub payload: Json,
}

#[derive(Debug, Clone)]
pub struct WorkflowExecutionRecord {
    pub workflow_execution_id: String,
    pub workflow_name: String,
    pub current_step: String,
    pub status: String,
    pub attempt: u64,
    pub claimed_by: Option<String>,
    pub claim_expires_at: Option<SystemTime>,
    pub state: Json,
}
