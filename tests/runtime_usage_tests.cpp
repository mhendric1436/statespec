#include "statespec/runtime_usage.hpp"

#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"

namespace
{

void runtime_usage_is_empty_for_empty_system()
{
    const auto usage = statespec::runtime_domain_usage(statespec::IrSystem{});

    statespec::test::require(!usage.uses_feature_flags, "empty system should not use flags");
    statespec::test::require(!usage.uses_queues, "empty system should not use queues");
    statespec::test::require(!usage.uses_leases, "empty system should not use leases");
    statespec::test::require(!usage.uses_workflows, "empty system should not use workflows");
    statespec::test::require(!usage.uses_logs, "empty system should not use logs");
    statespec::test::require(!usage.uses_metrics, "empty system should not use metrics");
    statespec::test::require(
        !usage.uses_observability, "empty system should not use observability"
    );
    statespec::test::require(
        !usage.uses_any_runtime_domain, "empty system should not use runtime domains"
    );
}

void runtime_usage_detects_declared_runtime_domains()
{
    statespec::IrSystem system;
    system.feature_flags.push_back(statespec::IrFeatureFlag{});
    system.queues.push_back(statespec::IrQueue{});
    system.leases.push_back(statespec::IrLease{});
    system.workflows.push_back(statespec::IrWorkflow{});
    system.logs.push_back(statespec::IrLog{});
    system.metrics.push_back(statespec::IrMetric{});

    const auto usage = statespec::runtime_domain_usage(system);

    statespec::test::require(usage.uses_feature_flags, "feature flag declarations use flags");
    statespec::test::require(usage.uses_queues, "queue declarations use queues");
    statespec::test::require(usage.uses_leases, "lease declarations use leases");
    statespec::test::require(usage.uses_workflows, "workflow declarations use workflows");
    statespec::test::require(usage.uses_logs, "log declarations use logs");
    statespec::test::require(usage.uses_metrics, "metric declarations use metrics");
    statespec::test::require(usage.uses_observability, "logs or metrics use observability");
    statespec::test::require(
        usage.uses_any_runtime_domain, "declared runtime domains should set aggregate usage"
    );
}

void runtime_usage_detects_api_transitive_domains()
{
    statespec::IrSystem system;
    statespec::IrApi start_workflow_api;
    start_workflow_api.starts_workflow = "ProvisionOrder";
    system.apis.push_back(start_workflow_api);

    statespec::IrApi enqueue_api;
    enqueue_api.enqueues = "OrderQueue.StartOrder";
    system.apis.push_back(enqueue_api);

    const auto usage = statespec::runtime_domain_usage(system);

    statespec::test::require(usage.uses_workflows, "API workflow starts use workflow runtime");
    statespec::test::require(usage.uses_queues, "API enqueue actions use queue runtime");
    statespec::test::require(!usage.uses_leases, "API actions should not imply leases");
}

void runtime_usage_detects_worker_transitive_domains()
{
    statespec::IrSystem system;

    statespec::IrWorker workflow_worker;
    workflow_worker.executes = "ProvisionOrder";
    system.workers.push_back(workflow_worker);

    statespec::IrWorker queue_worker;
    queue_worker.polls = "OrderQueue";
    system.workers.push_back(queue_worker);

    statespec::IrWorker lease_worker;
    lease_worker.lease = "WorkflowLaunchControl";
    system.workers.push_back(lease_worker);

    const auto usage = statespec::runtime_domain_usage(system);

    statespec::test::require(usage.uses_workflows, "worker executes uses workflow runtime");
    statespec::test::require(usage.uses_queues, "worker polls uses queue runtime");
    statespec::test::require(usage.uses_leases, "worker lease uses lease runtime");
}

void runtime_usage_detects_singleton_worker_lease_requirement()
{
    statespec::IrSystem system;
    statespec::IrWorker singleton_worker;
    singleton_worker.singleton = true;
    system.workers.push_back(singleton_worker);

    const auto usage = statespec::runtime_domain_usage(system);

    statespec::test::require(usage.uses_leases, "singleton workers use lease runtime");
}

} // namespace

TEST_CASE("runtime usage is empty for empty system")
{
    runtime_usage_is_empty_for_empty_system();
}

TEST_CASE("runtime usage detects declared runtime domains")
{
    runtime_usage_detects_declared_runtime_domains();
}

TEST_CASE("runtime usage detects API transitive domains")
{
    runtime_usage_detects_api_transitive_domains();
}

TEST_CASE("runtime usage detects worker transitive domains")
{
    runtime_usage_detects_worker_transitive_domains();
}

TEST_CASE("runtime usage detects singleton worker lease requirement")
{
    runtime_usage_detects_singleton_worker_lease_requirement();
}
