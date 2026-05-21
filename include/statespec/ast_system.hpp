#pragma once

#include "statespec/ast_api_workflow.hpp"
#include "statespec/ast_declarations.hpp"
#include "statespec/ast_entities.hpp"
#include "statespec/ast_external_systems.hpp"
#include "statespec/ast_observability.hpp"
#include "statespec/ast_runtime.hpp"

namespace statespec
{

struct SystemDecl
{
    std::string name;
    std::optional<TenantScopeDecl> tenant_scope;
    std::optional<SystemTenantDecl> system_tenant;
    std::vector<ValueDecl> values;
    std::vector<EnumDecl> enums;
    std::vector<EventDecl> events;
    std::vector<ShapeDecl> shapes;
    std::vector<ExternalSystemDecl> external_systems;
    std::vector<FeatureFlagDecl> feature_flags;
    std::vector<LogDecl> logs;
    std::vector<MetricDecl> metrics;
    std::vector<EntityDecl> entities;
    std::vector<QueueDecl> queues;
    std::vector<LeaseDecl> leases;
    std::vector<WorkerDecl> workers;
    std::vector<ApiServerDecl> api_servers;
    std::vector<ApiDecl> apis;
    std::vector<WorkflowDecl> workflows;
    std::vector<PolicyDecl> policies;
    std::vector<BlockMemberOrder> member_order;
    SourceRange range;
};

struct Spec
{
    std::optional<std::string> version;
    std::vector<IncludeDecl> includes;
    std::optional<SystemDecl> system;
};

} // namespace statespec
