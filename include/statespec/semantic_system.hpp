#pragma once

#include "statespec/semantic_api_workflow.hpp"
#include "statespec/semantic_declarations.hpp"
#include "statespec/semantic_entities.hpp"
#include "statespec/semantic_external_systems.hpp"
#include "statespec/semantic_observability.hpp"
#include "statespec/semantic_runtime.hpp"

namespace statespec
{

struct SemanticSystem
{
    std::string name;
    std::optional<SemanticTenantScope> tenant_scope;
    std::optional<SemanticSystemTenant> system_tenant;
    std::vector<SemanticValue> values;
    std::vector<SemanticEnum> enums;
    std::vector<SemanticEvent> events;
    std::vector<SemanticShape> shapes;
    std::vector<SemanticExternalSystem> external_systems;
    std::vector<SemanticFeatureFlag> feature_flags;
    std::vector<SemanticLog> logs;
    std::vector<SemanticMetric> metrics;
    std::vector<SemanticEntity> entities;
    std::vector<SemanticQueue> queues;
    std::vector<SemanticLease> leases;
    std::vector<SemanticWorker> workers;
    std::vector<SemanticApiServer> api_servers;
    std::vector<SemanticApi> apis;
    std::vector<SemanticWorkflow> workflows;
    std::vector<SemanticPolicy> policies;
};

} // namespace statespec
