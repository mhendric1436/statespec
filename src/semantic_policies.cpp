#include "semantic_policies.hpp"

#include "semantic_symbols.hpp"

#include <utility>

namespace statespec
{

void resolve_semantic_policies(
    const SystemDecl& system,
    const SymbolTable& symbols,
    SemanticSystem& resolved
)
{
    for (const auto& policy : system.policies)
    {
        SemanticPolicy resolved_policy;
        resolved_policy.name = policy.name;
        resolved_policy.tenant_scoped_by = policy.tenant_scoped_by;
        for (const auto& allow : policy.allows)
        {
            resolved_policy.allows.push_back(
                SemanticPolicyRule{resolve_reference(symbols, allow.action), allow.condition}
            );
        }
        for (const auto& deny : policy.denies)
        {
            resolved_policy.denies.push_back(
                SemanticPolicyRule{resolve_reference(symbols, deny.action), deny.condition}
            );
        }
        for (const auto& quota : policy.quotas)
        {
            resolved_policy.quotas.push_back(SemanticQuota{quota.name, quota.expression});
        }
        for (const auto& audit : policy.audits)
        {
            resolved_policy.audits.push_back(resolve_reference(symbols, audit));
        }
        resolved.policies.push_back(std::move(resolved_policy));
    }
}

} // namespace statespec
