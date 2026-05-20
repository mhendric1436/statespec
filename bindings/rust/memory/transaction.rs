use std::collections::{BTreeMap, BTreeSet};
use std::sync::{Arc, Mutex};

use crate::backend::{BackendResult, CollectionDescriptor, Transaction, Version, VersionedRecord};
use crate::feature_flag::{FeatureFlagDefinition, FeatureFlagValue};
use crate::lease::{LeaseDefinition, LeaseRecord};
use crate::log::{LogDefinition, LogEvent};
use crate::metric::{MetricDefinition, MetricSample};
use crate::queue::{QueueDefinition, QueueMessageRecord};
use crate::workflow::{WorkflowDefinition, WorkflowExecutionRecord};

#[derive(Debug, Default)]
pub struct InMemoryState {
    pub records: BTreeMap<String, BTreeMap<String, VersionedRecord>>,
    pub versions: BTreeMap<String, Version>,
    pub collections: BTreeMap<String, CollectionDescriptor>,
    pub feature_flag_definitions: BTreeMap<String, FeatureFlagDefinition>,
    pub feature_flag_values: BTreeMap<String, FeatureFlagValue>,
    pub queue_definitions: BTreeMap<String, QueueDefinition>,
    pub queue_messages: BTreeMap<String, QueueMessageRecord>,
    pub queue_idempotency_keys: BTreeMap<String, String>,
    pub lease_definitions: BTreeMap<String, LeaseDefinition>,
    pub leases: BTreeMap<String, LeaseRecord>,
    pub workflow_definitions: BTreeMap<String, WorkflowDefinition>,
    pub workflow_executions: BTreeMap<String, WorkflowExecutionRecord>,
    pub log_definitions: BTreeMap<String, LogDefinition>,
    pub log_events: Vec<LogEvent>,
    pub metric_definitions: BTreeMap<String, MetricDefinition>,
    pub metric_samples: Vec<MetricSample>,
}

#[derive(Debug)]
pub struct InMemoryTransaction {
    pub(crate) state: Arc<Mutex<InMemoryState>>,
    pub(crate) open: bool,
    pub(crate) read_versions: BTreeMap<String, Version>,
    pub(crate) puts: BTreeMap<String, VersionedRecord>,
    pub(crate) erases: BTreeSet<String>,
    pub(crate) feature_flag_definition_puts: BTreeMap<String, FeatureFlagDefinition>,
    pub(crate) feature_flag_value_puts: BTreeMap<String, FeatureFlagValue>,
    pub(crate) queue_definition_puts: BTreeMap<String, QueueDefinition>,
    pub(crate) queue_message_puts: BTreeMap<String, QueueMessageRecord>,
    pub(crate) queue_idempotency_puts: BTreeMap<String, String>,
    pub(crate) lease_definition_puts: BTreeMap<String, LeaseDefinition>,
    pub(crate) lease_puts: BTreeMap<String, LeaseRecord>,
    pub(crate) lease_erases: BTreeSet<String>,
    pub(crate) workflow_definition_puts: BTreeMap<String, WorkflowDefinition>,
    pub(crate) workflow_execution_puts: BTreeMap<String, WorkflowExecutionRecord>,
    pub(crate) log_definition_puts: BTreeMap<String, LogDefinition>,
    pub(crate) log_event_appends: Vec<LogEvent>,
    pub(crate) metric_definition_puts: BTreeMap<String, MetricDefinition>,
    pub(crate) metric_sample_appends: Vec<MetricSample>,
}

impl InMemoryTransaction {
    pub(crate) fn new(state: Arc<Mutex<InMemoryState>>) -> Self {
        Self {
            state,
            open: true,
            read_versions: BTreeMap::new(),
            puts: BTreeMap::new(),
            erases: BTreeSet::new(),
            feature_flag_definition_puts: BTreeMap::new(),
            feature_flag_value_puts: BTreeMap::new(),
            queue_definition_puts: BTreeMap::new(),
            queue_message_puts: BTreeMap::new(),
            queue_idempotency_puts: BTreeMap::new(),
            lease_definition_puts: BTreeMap::new(),
            lease_puts: BTreeMap::new(),
            lease_erases: BTreeSet::new(),
            workflow_definition_puts: BTreeMap::new(),
            workflow_execution_puts: BTreeMap::new(),
            log_definition_puts: BTreeMap::new(),
            log_event_appends: Vec::new(),
            metric_definition_puts: BTreeMap::new(),
            metric_sample_appends: Vec::new(),
        }
    }
}

impl Transaction for InMemoryTransaction {
    fn is_open(&self) -> bool {
        self.open
    }

    fn abort(&mut self) -> BackendResult<()> {
        self.open = false;
        self.read_versions.clear();
        self.puts.clear();
        self.erases.clear();
        self.feature_flag_definition_puts.clear();
        self.feature_flag_value_puts.clear();
        self.queue_definition_puts.clear();
        self.queue_message_puts.clear();
        self.queue_idempotency_puts.clear();
        self.lease_definition_puts.clear();
        self.lease_puts.clear();
        self.lease_erases.clear();
        self.workflow_definition_puts.clear();
        self.workflow_execution_puts.clear();
        self.log_definition_puts.clear();
        self.log_event_appends.clear();
        self.metric_definition_puts.clear();
        self.metric_sample_appends.clear();
        Ok(())
    }
}

pub(crate) fn record_version_key(collection: &str, key: &str) -> String {
    format!("record:{collection}:{key}")
}

pub(crate) fn definition_key(parts: &[String]) -> String {
    parts.join(":")
}

pub(crate) fn version_or_zero(versions: &BTreeMap<String, Version>, key: &str) -> Version {
    versions.get(key).copied().unwrap_or(0)
}
