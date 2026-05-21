#include "common/backend.hpp"
#include "common/memory/backend.hpp"
#include "common/runtime/feature_flag_store.hpp"
#include "common/runtime/lease_store.hpp"
#include "common/runtime/log_sink.hpp"
#include "common/runtime/metric_sink.hpp"
#include "common/runtime/queue_store.hpp"
#include "common/runtime/workflow_store.hpp"

#include <cassert>
#include <chrono>

int main()
{
    using namespace statespec::backend;
    using namespace statespec::backend::memory;
    using namespace statespec::backend::runtime;

    InMemoryBackend backend;
    RuntimeFeatureFlagStore flags;
    RuntimeQueueStore queues;
    RuntimeLeaseStore leases;
    RuntimeWorkflowStore workflows;
    RuntimeLogSink logs;
    RuntimeMetricSink metrics;

    backend.ensure_collection(CollectionDescriptor{.name = "orders", .key_fields = {"order_id"}});

    auto tx = backend.begin();
    flags.register_definitionTx(
        *tx, FeatureFlagDefinition{
                 .name = "new_scheduler",
                 .type = FeatureFlagType::Bool,
                 .default_value = FeatureFlagValue::bool_value(true),
                 .scope = FeatureFlagScopeKind::System,
             }
    );
    queues.register_definitionTx(
        *tx, RegisterQueueDefinitionRequest{
                 .definition = QueueDefinition{
                     .queue = "workflow",
                     .channel = "launch",
                     .visibility_timeout = std::chrono::minutes{1},
                     .max_attempts = 3,
                     .metadata = Json::object({}),
                 },
             }
    );

    LeaseDefinitionId lease_id{.name = "workflow_step", .version = 1};
    leases.register_definitionTx(
        *tx, LeaseDefinition{
                 .id = lease_id,
                 .resource_pattern = "workflow/*",
                 .ttl = std::chrono::minutes{1},
                 .renew_every = std::chrono::seconds{30},
                 .fencing_token = true,
             }
    );
    workflows.register_definitionTx(
        *tx, RegisterWorkflowDefinitionRequest{
                 .definition = WorkflowDefinition{
                     .workflow_name = "ProvisionService",
                     .workflow_version = 1,
                     .start_step = "validate_request",
                     .expected_execution_time = std::chrono::minutes{1},
                     .steps =
                         {
                             WorkflowStepDefinition{
                                 .name = "validate_request",
                                 .expected_execution_time = std::chrono::seconds{1},
                                 .max_retries = 3,
                             },
                         },
                     .metadata = Json::object({}),
                 },
             }
    );
    logs.register_definitionTx(
        *tx, LogDefinition{
                 .name = "launch_decision",
                 .level = LogLevel::Info,
                 .event_name = "workflow.launch.decision",
             }
    );
    metrics.register_definitionTx(
        *tx, MetricDefinition{
                 .name = "launch_attempts",
                 .kind = MetricKind::Counter,
                 .backend_name = "workflow_launch_attempts_total",
                 .unit = "1",
             }
    );
    backend.commit(*tx);

    assert(
        flags.evaluate(backend, FeatureFlagEvaluationRequest{.name = "new_scheduler"}).as_bool() ==
        true
    );

    const auto now = std::chrono::system_clock::from_time_t(100);
    auto message = queues.enqueue(
        backend, EnqueueMessageRequest{
                     .message_id = "msg-1",
                     .queue = "workflow",
                     .channel = "launch",
                     .payload = Json::object({}),
                 }
    );
    assert(message.status == "Pending");

    auto claimed = queues.claim(
        backend, ClaimMessageRequest{
                     .queue = "workflow",
                     .channel = "launch",
                     .claimant = "worker-1",
                     .now = now,
                     .visibility_timeout = std::chrono::minutes{1},
                     .max_messages = 1,
                 }
    );
    assert(claimed.size() == 1);

    auto lease = leases.acquire(
        backend, LeaseAcquireRequest{
                     .definition_id = lease_id,
                     .resource = "workflow/msg-1",
                     .holder = "worker-1",
                     .now = now,
                 }
    );
    assert(lease.acquired);
    assert(lease.lease.has_value());

    auto started = workflows.start(
        backend, StartWorkflowRequest{
                     .workflow_execution_id = "wf-1",
                     .workflow_name = "ProvisionService",
                     .workflow_version = 1,
                     .start_step = "validate_request",
                     .state = Json::object({}),
                 }
    );
    assert(started.status == "Running");

    auto steps = workflows.claim_steps(
        backend, ClaimWorkflowStepRequest{
                     .workflow_execution_id = "wf-1",
                     .workflow_name = "ProvisionService",
                     .workflow_version = 1,
                     .worker = "worker-1",
                     .now = now,
                     .lease_duration = std::chrono::minutes{1},
                     .max_steps = 1,
                 }
    );
    assert(steps.size() == 1);

    workflows.keep_alive_step(
        backend, KeepAliveWorkflowStepRequest{
                     .workflow_execution_id = "wf-1",
                     .worker = "worker-1",
                     .current_step = "validate_request",
                     .now = now,
                     .lease_duration = std::chrono::minutes{1},
                 }
    );

    logs.emit_log(
        backend, LogEvent{
                     .name = "launch_decision",
                     .level = LogLevel::Info,
                     .event_name = "workflow.launch.decision",
                 }
    );
    metrics.record_metric(
        backend, MetricSample{
                     .name = "launch_attempts",
                     .kind = MetricKind::Counter,
                     .backend_name = "workflow_launch_attempts_total",
                     .value = 1.0,
                     .unit = "1",
                 }
    );
    assert(logs.inspect_events(backend).size() == 1);
    assert(metrics.inspect_samples(backend).size() == 1);

    auto write_tx = backend.begin();
    backend.put(*write_tx, "orders", "order-1", Json::object({{"status", "new"}}));
    backend.commit(*write_tx);

    auto read_tx = backend.begin();
    assert(backend.query(*read_tx, "orders", Query::key_prefix_query("order-")).size() == 1);
    backend.commit(*read_tx);

    return 0;
}
