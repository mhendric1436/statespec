#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_go_policy_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "func PolicyDescriptors() []PolicyDescriptor {\n";
    out << "\treturn []PolicyDescriptor{\n";
    for (const auto& policy : system.policies)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(policy.name) << ",\n";
        out << "\t\t\tTenantScopedBy: " << string_ptr_expr(policy.tenant_scoped_by) << ",\n";
        out << "\t\t\tAllows: []PolicyRuleDescriptor{\n";
        for (const auto& allow : policy.allows)
        {
            out << "\t\t\t\t{Action: " << go_string(allow.action)
                << ", Condition: " << go_string(allow.condition) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tDenies: []PolicyRuleDescriptor{\n";
        for (const auto& deny : policy.denies)
        {
            out << "\t\t\t\t{Action: " << go_string(deny.action)
                << ", Condition: " << go_string(deny.condition) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tQuotas: []QuotaDescriptor{\n";
        for (const auto& quota : policy.quotas)
        {
            out << "\t\t\t\t{Name: " << go_string(quota.name)
                << ", Expression: " << go_string(quota.expression) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t\tAudits: []string{";
        for (std::size_t i = 0; i < policy.audits.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_string(policy.audits[i]);
        }
        out << "},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
