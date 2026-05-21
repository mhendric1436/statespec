use std::collections::BTreeMap;
use std::time::{Duration, SystemTime};

use statespec_generated::backend::{Backend, Query};
use statespec_generated::feature_flag::{
    FeatureFlagDefinition, FeatureFlagEvaluationContext, FeatureFlagEvaluationRequest,
    FeatureFlagScopeKind, FeatureFlagStore, FeatureFlagType, FeatureFlagValue,
};
use statespec_generated::json::Json;
use statespec_generated::lease::{
    LeaseAcquireRequest, LeaseDefinition, LeaseDefinitionId, LeaseStore,
};
use statespec_generated::log::{LogDefinition, LogEvent, LogLevel, LogSink};
use statespec_generated::memory_backend::InMemoryBackend;
use statespec_generated::metric::{MetricDefinition, MetricKind, MetricSample, MetricSink};
use statespec_generated::queue::{
    ClaimMessageRequest, EnqueueMessageRequest, QueueDefinition, QueueStore,
    RegisterQueueDefinitionRequest,
};
use statespec_generated::runtime_feature_flags::RuntimeFeatureFlagStore;
use statespec_generated::runtime_leases::RuntimeLeaseStore;
use statespec_generated::runtime_logs::RuntimeLogSink;
use statespec_generated::runtime_metrics::RuntimeMetricSink;
use statespec_generated::runtime_queues::RuntimeQueueStore;
use statespec_generated::runtime_workflows::RuntimeWorkflowStore;
use statespec_generated::workflow::{
    ClaimWorkflowStepRequest, KeepAliveWorkflowStepRequest, RegisterWorkflowDefinitionRequest,
    StartWorkflowRequest, WorkflowDefinition, WorkflowStepDefinition, WorkflowStore,
};

#[test]
fn memory_backend_stores_compose_in_transaction() {
    let backend = InMemoryBackend::new();
    let flags = RuntimeFeatureFlagStore::new();
    let queues = RuntimeQueueStore::new();
    let leases = RuntimeLeaseStore::new();
    let workflows = RuntimeWorkflowStore::new();
    let logs = RuntimeLogSink::new();
    let metrics = RuntimeMetricSink::new();

    let mut tx = backend.begin().unwrap();
    <RuntimeFeatureFlagStore as FeatureFlagStore<InMemoryBackend>>::register_definition_tx(
        &flags,
        &mut tx,
        &FeatureFlagDefinition {
            name: "new_scheduler".to_string(),
            flag_type: FeatureFlagType::Bool,
            default_value: FeatureFlagValue::Bool(true),
            scope: FeatureFlagScopeKind::System,
            owner: None,
            description: None,
            expires: None,
        },
    )
    .unwrap();
    <RuntimeQueueStore as QueueStore<InMemoryBackend>>::register_definition_tx(
        &queues,
        &mut tx,
        &RegisterQueueDefinitionRequest {
            definition: QueueDefinition {
                queue: "workflow".to_string(),
                channel: "launch".to_string(),
                visibility_timeout: Duration::from_secs(60),
                max_attempts: 3,
                dead_letter_queue: None,
                metadata: Json::Object(BTreeMap::new()),
            },
        },
    )
    .unwrap();
    let lease_id = LeaseDefinitionId {
        name: "workflow_step".to_string(),
        version: 1,
    };
    <RuntimeLeaseStore as LeaseStore<InMemoryBackend>>::register_definition_tx(
        &leases,
        &mut tx,
        &LeaseDefinition {
            id: lease_id.clone(),
            resource_pattern: "workflow/*".to_string(),
            ttl: Duration::from_secs(60),
            renew_every: Duration::from_secs(30),
            max_ttl: None,
            fencing_token: true,
        },
    )
    .unwrap();
    <RuntimeWorkflowStore as WorkflowStore<InMemoryBackend>>::register_definition_tx(
        &workflows,
        &mut tx,
        &RegisterWorkflowDefinitionRequest {
            definition: WorkflowDefinition {
                workflow_name: "ProvisionService".to_string(),
                workflow_version: 1,
                start_step: "validate_request".to_string(),
                expected_execution_time: Duration::from_secs(60),
                singleton: false,
                steps: vec![WorkflowStepDefinition {
                    name: "validate_request".to_string(),
                    expected_execution_time: Duration::from_secs(1),
                    max_retries: 3,
                }],
                metadata: Json::Object(BTreeMap::new()),
            },
        },
    )
    .unwrap();
    <RuntimeLogSink as LogSink<InMemoryBackend>>::register_definition_tx(
        &logs,
        &mut tx,
        &LogDefinition {
            name: "launch_decision".to_string(),
            level: LogLevel::Info,
            event_name: "workflow.launch.decision".to_string(),
            fields: vec![],
        },
    )
    .unwrap();
    <RuntimeMetricSink as MetricSink<InMemoryBackend>>::register_definition_tx(
        &metrics,
        &mut tx,
        &MetricDefinition {
            name: "launch_attempts".to_string(),
            kind: MetricKind::Counter,
            backend_name: "workflow_launch_attempts_total".to_string(),
            unit: "1".to_string(),
            labels: vec![],
        },
    )
    .unwrap();
    backend.commit(tx).unwrap();

    let flag = flags
        .evaluate(
            &backend,
            &FeatureFlagEvaluationRequest {
                name: "new_scheduler".to_string(),
                context: FeatureFlagEvaluationContext::default(),
            },
        )
        .unwrap();
    assert_eq!(flag.as_bool(), Some(true));

    let now = SystemTime::UNIX_EPOCH + Duration::from_secs(100);
    let message = queues
        .enqueue(
            &backend,
            &EnqueueMessageRequest {
                message_id: "msg-1".to_string(),
                queue: "workflow".to_string(),
                channel: "launch".to_string(),
                idempotency_key: None,
                payload: Json::Object(BTreeMap::new()),
            },
        )
        .unwrap();
    assert_eq!(message.status, "Pending");
    let claimed = queues
        .claim(
            &backend,
            &ClaimMessageRequest {
                queue: "workflow".to_string(),
                channel: "launch".to_string(),
                claimant: "worker-1".to_string(),
                now,
                visibility_timeout: Duration::from_secs(60),
                max_messages: 1,
            },
        )
        .unwrap();
    assert_eq!(claimed.len(), 1);

    let lease = leases
        .acquire(
            &backend,
            &LeaseAcquireRequest {
                definition_id: lease_id,
                resource: "workflow/msg-1".to_string(),
                holder: "worker-1".to_string(),
                now,
            },
        )
        .unwrap();
    assert!(lease.acquired);

    workflows
        .start(
            &backend,
            &StartWorkflowRequest {
                workflow_execution_id: "wf-1".to_string(),
                workflow_name: "ProvisionService".to_string(),
                workflow_version: 1,
                start_step: "validate_request".to_string(),
                state: Json::Object(BTreeMap::new()),
            },
        )
        .unwrap();
    let steps = workflows
        .claim_steps(
            &backend,
            &ClaimWorkflowStepRequest {
                workflow_execution_id: "wf-1".to_string(),
                workflow_name: "ProvisionService".to_string(),
                workflow_version: 1,
                worker: "worker-1".to_string(),
                now,
                lease_duration: Duration::from_secs(60),
                max_steps: 1,
            },
        )
        .unwrap();
    assert_eq!(steps.len(), 1);
    workflows
        .keep_alive_step(
            &backend,
            &KeepAliveWorkflowStepRequest {
                workflow_execution_id: "wf-1".to_string(),
                worker: "worker-1".to_string(),
                current_step: "validate_request".to_string(),
                now,
                lease_duration: Duration::from_secs(60),
            },
        )
        .unwrap();

    logs.emit_log(
        &backend,
        &LogEvent {
            name: "launch_decision".to_string(),
            level: LogLevel::Info,
            event_name: "workflow.launch.decision".to_string(),
            fields: BTreeMap::new(),
        },
    )
    .unwrap();
    metrics
        .record_metric(
            &backend,
            &MetricSample {
                name: "launch_attempts".to_string(),
                kind: MetricKind::Counter,
                backend_name: "workflow_launch_attempts_total".to_string(),
                value: 1.0,
                unit: "1".to_string(),
                labels: BTreeMap::new(),
            },
        )
        .unwrap();
    assert_eq!(logs.inspect_events(&backend).unwrap().len(), 1);
    assert_eq!(metrics.inspect_samples(&backend).unwrap().len(), 1);

    let mut tx = backend.begin().unwrap();
    backend
        .put(
            &mut tx,
            "orders",
            "order-1",
            Json::Object(BTreeMap::from([(
                "status".to_string(),
                Json::String("new".to_string()),
            )])),
        )
        .unwrap();
    backend.commit(tx).unwrap();
    let mut tx = backend.begin().unwrap();
    assert_eq!(
        backend
            .query(
                &mut tx,
                "orders",
                &Query::KeyPrefix {
                    prefix: "order-".to_string()
                },
            )
            .unwrap()
            .len(),
        1
    );
    backend.commit(tx).unwrap();
}
