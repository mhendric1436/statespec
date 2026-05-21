#include "ir_policies.hpp"

#include <utility>

namespace statespec
{

void lower_ir_policies(
    const SemanticSystem& system,
    IrSystem& ir
)
{
    for (const auto& policy : system.policies)
    {
        IrPolicy ir_policy;
        ir_policy.name = policy.name;
        ir_policy.tenant_scoped_by = policy.tenant_scoped_by;
        for (const auto& allow : policy.allows)
        {
            ir_policy.allows.push_back(IrPolicyRule{allow.action.name, allow.condition});
        }
        for (const auto& deny : policy.denies)
        {
            ir_policy.denies.push_back(IrPolicyRule{deny.action.name, deny.condition});
        }
        for (const auto& quota : policy.quotas)
        {
            ir_policy.quotas.push_back(IrQuota{quota.name, quota.expression});
        }
        for (const auto& audit : policy.audits)
        {
            ir_policy.audits.push_back(audit.name);
        }
        ir.policies.push_back(std::move(ir_policy));
    }
}

} // namespace statespec
