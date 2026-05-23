#include "statespec/runtime_usage.hpp"

namespace statespec
{

RuntimeDomainUsage runtime_domain_usage(const IrSystem& system)
{
    RuntimeDomainUsage usage;

    usage.uses_feature_flags = !system.feature_flags.empty();
    usage.uses_queues = !system.queues.empty();
    usage.uses_leases = !system.leases.empty();
    usage.uses_workflows = !system.workflows.empty();
    usage.uses_logs = !system.logs.empty();
    usage.uses_metrics = !system.metrics.empty();

    for (const auto& entity : system.entities)
    {
        for (const auto& state : entity.states)
        {
            usage.uses_entity_gc = usage.uses_entity_gc || state.garbage_collection.has_value();
        }
    }

    for (const auto& api : system.apis)
    {
        usage.uses_workflows = usage.uses_workflows || api.starts_workflow.has_value();
        usage.uses_queues = usage.uses_queues || api.enqueues.has_value();
    }

    for (const auto& worker : system.workers)
    {
        usage.uses_workflows = usage.uses_workflows || worker.executes.has_value();
        usage.uses_queues = usage.uses_queues || worker.polls.has_value();
        usage.uses_leases =
            usage.uses_leases || worker.lease.has_value() || worker.singleton.value_or(false);
    }

    usage.uses_observability = usage.uses_logs || usage.uses_metrics;
    usage.uses_any_runtime_domain = usage.uses_feature_flags || usage.uses_queues ||
                                    usage.uses_leases || usage.uses_workflows ||
                                    usage.uses_observability;

    return usage;
}

} // namespace statespec
