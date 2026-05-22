#pragma once

#include "statespec/ir_system.hpp"

namespace statespec
{

struct RuntimeDomainUsage
{
    bool uses_feature_flags = false;
    bool uses_queues = false;
    bool uses_leases = false;
    bool uses_workflows = false;
    bool uses_logs = false;
    bool uses_metrics = false;
    bool uses_observability = false;
    bool uses_any_runtime_domain = false;
};

RuntimeDomainUsage runtime_domain_usage(const IrSystem& system);

} // namespace statespec
