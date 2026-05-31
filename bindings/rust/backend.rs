use crate::json::Json;

pub type CollectionName = String;
pub type Key = String;
pub type Version = u64;

pub mod runtime_collections {
    pub const FEATURE_FLAG_DEFINITIONS: &str = "statespec_feature_flag_definitions";
    pub const FEATURE_FLAG_VALUES: &str = "statespec_feature_flag_values";
    pub const QUEUE_DEFINITIONS: &str = "statespec_queue_definitions";
    pub const QUEUE_MESSAGES: &str = "statespec_queue_messages";
    pub const QUEUE_IDEMPOTENCY: &str = "statespec_queue_idempotency";
    pub const LEASE_DEFINITIONS: &str = "statespec_lease_definitions";
    pub const LEASES: &str = "statespec_leases";
    pub const WORKFLOW_DEFINITIONS: &str = "statespec_workflow_definitions";
    pub const WORKFLOW_EXECUTIONS: &str = "statespec_workflow_executions";
    pub const WORKFLOW_HEARTBEATS: &str = "statespec_workflow_heartbeats";
    pub const LOG_DEFINITIONS: &str = "statespec_log_definitions";
    pub const LOG_EVENTS: &str = "statespec_log_events";
    pub const METRIC_DEFINITIONS: &str = "statespec_metric_definitions";
    pub const METRIC_SAMPLES: &str = "statespec_metric_samples";
}

pub mod runtime_key_fields {
    pub const NAME: &str = "name";
    pub const SCOPE_KEY: &str = "scope_key";
    pub const QUEUE: &str = "queue";
    pub const MESSAGE_ID: &str = "message_id";
    pub const IDEMPOTENCY_KEY: &str = "idempotency_key";
    pub const LEASE_DEFINITION: &str = "lease_definition";
    pub const LEASE: &str = "lease";
    pub const WORKFLOW_DEFINITION: &str = "workflow_definition";
    pub const WORKFLOW_EXECUTION_ID: &str = "workflow_execution_id";
    pub const WORKFLOW_HEARTBEAT: &str = "workflow_heartbeat";
    pub const EVENT_ID: &str = "event_id";
    pub const SAMPLE_ID: &str = "sample_id";
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FieldType {
    String,
    Bool,
    Int,
    Int32,
    Int64,
    Long,
    Double,
    Decimal,
    Json,
    Timestamp,
    Duration,
    Uuid,
    Named,
    List,
    Set,
    Map,
    Optional,
    Reference,
}

#[derive(Debug, Clone)]
pub struct FieldDescriptor {
    pub name: String,
    pub field_type: FieldType,
    pub type_name: String,
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
pub enum IndexValue {
    Null,
    String(String),
    Integer(i64),
    Decimal(f64),
    Boolean(bool),
    Timestamp(String),
}

impl IndexValue {
    pub fn json_value(&self) -> Json {
        match self {
            IndexValue::Null => Json::Null,
            IndexValue::String(value) => Json::String(value.clone()),
            IndexValue::Integer(value) => Json::Integer(*value),
            IndexValue::Decimal(value) => Json::Decimal(*value),
            IndexValue::Boolean(value) => Json::Bool(*value),
            IndexValue::Timestamp(value) => Json::String(value.clone()),
        }
    }
}

#[derive(Debug, Clone)]
pub struct IndexBound {
    pub values: Vec<IndexValue>,
    pub inclusive: bool,
}

#[derive(Debug, Clone)]
pub enum Query {
    All,
    KeyPrefix {
        prefix: String,
    },
    JsonEquals {
        path: String,
        value: Json,
    },
    IndexEquals {
        index_name: String,
        values: Vec<IndexValue>,
    },
    IndexPrefix {
        index_name: String,
        prefix_values: Vec<IndexValue>,
    },
    IndexRange {
        index_name: String,
        lower_bound: Option<IndexBound>,
        upper_bound: Option<IndexBound>,
    },
}

pub trait Transaction {
    fn is_open(&self) -> bool;
    fn abort(&mut self) -> BackendResult<()>;
    fn get(&mut self, collection: &str, key: &str) -> BackendResult<Option<VersionedRecord>>;
    fn query(&mut self, collection: &str, query: &Query) -> BackendResult<Vec<VersionedRecord>>;
    fn put(&mut self, collection: &str, key: &str, document: Json) -> BackendResult<()>;
    fn erase(&mut self, collection: &str, key: &str) -> BackendResult<()>;
}

pub trait Backend {
    type Tx: Transaction;

    fn capabilities(&self) -> BackendCapabilities;

    fn ensure_collection(&self, descriptor: &CollectionDescriptor) -> BackendResult<()>;

    fn ensure_collections(&self, descriptors: &[CollectionDescriptor]) -> BackendResult<()>;

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

    fn erase(&self, tx: &mut Self::Tx, collection: &str, key: &str) -> BackendResult<()>;

    fn commit(&self, tx: Self::Tx) -> BackendResult<()>;
}
