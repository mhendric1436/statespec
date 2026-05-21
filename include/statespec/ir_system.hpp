#pragma once

#include "statespec/ir_api_workflow.hpp"
#include "statespec/ir_declarations.hpp"
#include "statespec/ir_entities.hpp"
#include "statespec/ir_external_systems.hpp"
#include "statespec/ir_observability.hpp"
#include "statespec/ir_runtime.hpp"

namespace statespec
{

struct IrSystem
{
    std::string name;
    std::optional<IrTenantScope> tenant_scope;
    std::optional<IrSystemTenant> system_tenant;
    std::vector<IrValue> values;
    std::vector<IrEnum> enums;
    std::vector<IrEvent> events;
    std::vector<IrShape> shapes;
    std::vector<IrExternalSystem> external_systems;
    std::vector<IrFeatureFlag> feature_flags;
    std::vector<IrLog> logs;
    std::vector<IrMetric> metrics;
    std::vector<IrEntity> entities;
    std::vector<IrQueue> queues;
    std::vector<IrLease> leases;
    std::vector<IrWorker> workers;
    std::vector<IrApiServer> api_servers;
    std::vector<IrApi> apis;
    std::vector<IrWorkflow> workflows;
    std::vector<IrPolicy> policies;
};

} // namespace statespec
