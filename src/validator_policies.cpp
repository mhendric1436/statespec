#include "validator_declarations.hpp"

#include "validator_helpers.hpp"

namespace statespec::validator_detail
{

namespace
{

int policy_member_order_index(const std::string& kind)
{
    if (kind == "tenant")
    {
        return 0;
    }
    if (kind == "allow")
    {
        return 1;
    }
    if (kind == "deny")
    {
        return 2;
    }
    if (kind == "quota")
    {
        return 3;
    }
    if (kind == "audit")
    {
        return 4;
    }
    return 5;
}

void validate_policy_member_order(
    const PolicyDecl& policy,
    DiagnosticBag& diagnostics
)
{
    int previous_order = -1;
    for (const auto& member : policy.member_order)
    {
        const auto order = policy_member_order_index(member.kind);
        if (order < previous_order)
        {
            diagnostics.warning(
                member.range, diagnostic_codes::NoncanonicalPolicyOrder,
                "policy '" + policy.name +
                    "' members should use canonical order: tenant, allow, deny, quota, audit"
            );
            return;
        }
        previous_order = order;
    }
}
} // namespace

void validate_policies(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& policy : system.policies)
    {
        validate_policy_member_order(policy, diagnostics);

        if (!policy.tenant_scoped_by.has_value())
        {
            required_error(
                diagnostics, policy.range, "policy '" + policy.name + "'", "tenant scoped_by"
            );
        }
        if (policy.tenant_scoped_by.has_value() && system.tenant_scope.has_value() &&
            *policy.tenant_scoped_by != system.tenant_scope->field_name)
        {
            diagnostics.error(
                policy.range, diagnostic_codes::TenantPolicyScopeMismatch,
                "policy '" + policy.name + "' tenant field '" + *policy.tenant_scoped_by +
                    "' must match system tenant field '" + system.tenant_scope->field_name + "'"
            );
        }
        if (policy.allows.empty() && policy.denies.empty() && policy.quotas.empty() &&
            policy.audits.empty())
        {
            required_error(
                diagnostics, policy.range, "policy '" + policy.name + "'",
                "at least one allow, deny, quota, or audit declaration"
            );
        }

        for (const auto& rule : policy.allows)
        {
            if (!symbols.find(rule.action).has_value())
            {
                unknown_reference_error(
                    diagnostics, rule.range, "policy allow action", rule.action
                );
            }
            validate_expression(system, rule.range, rule.condition, diagnostics);
        }
        for (const auto& rule : policy.denies)
        {
            if (!symbols.find(rule.action).has_value())
            {
                unknown_reference_error(diagnostics, rule.range, "policy deny action", rule.action);
            }
            validate_expression(system, rule.range, rule.condition, diagnostics);
        }
        for (const auto& quota : policy.quotas)
        {
            validate_expression(system, quota.range, quota.expression, diagnostics);
        }
        for (const auto& audit : policy.audits)
        {
            if (!symbols.find(audit).has_value())
            {
                unknown_reference_error(diagnostics, policy.range, "policy audit action", audit);
            }
        }
    }
}
} // namespace statespec::validator_detail
