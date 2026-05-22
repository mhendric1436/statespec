#include "validator_declarations.hpp"

namespace statespec::validator_detail
{

namespace
{

int system_member_order_index(const std::string& kind)
{
    if (kind == "tenant")
    {
        return 0;
    }
    if (kind == "system_tenant")
    {
        return 1;
    }
    if (kind == "value" || kind == "enum" || kind == "shape" || kind == "external_system" ||
        kind == "event")
    {
        return 2;
    }
    if (kind == "feature_flag")
    {
        return 3;
    }
    if (kind == "log" || kind == "metric")
    {
        return 4;
    }
    if (kind == "entity")
    {
        return 5;
    }
    if (kind == "queue")
    {
        return 6;
    }
    if (kind == "lease")
    {
        return 7;
    }
    if (kind == "worker")
    {
        return 8;
    }
    if (kind == "workflow")
    {
        return 9;
    }
    if (kind == "api_server" || kind == "api")
    {
        return 10;
    }
    if (kind == "policy")
    {
        return 11;
    }
    return 12;
}

void validate_system_member_order_impl(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    int previous_order = -1;
    for (const auto& member : system.member_order)
    {
        const auto order = system_member_order_index(member.kind);
        if (order < previous_order)
        {
            diagnostics.warning(
                member.range, diagnostic_codes::NoncanonicalSystemOrder,
                "system '" + system.name +
                    "' members should use canonical order: tenant, system_tenant, "
                    "values/enums/shapes/events, feature_flags, logs/metrics, "
                    "entities, queues, leases, workers, workflows, APIs, policies"
            );
            return;
        }
        previous_order = order;
    }
}
} // namespace

void validate_system_member_order(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    validate_system_member_order_impl(system, diagnostics);
}

} // namespace statespec::validator_detail
