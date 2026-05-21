#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_policy_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<PolicyDescriptor> policyDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t policy_index = 0; policy_index < system.policies.size(); ++policy_index)
    {
        const auto& policy = system.policies[policy_index];
        out << "            new PolicyDescriptor(\n";
        out << "                " << java_string(policy.name) << ",\n";
        out << "                " << java_optional_string_expr(policy.tenant_scoped_by) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < policy.allows.size(); ++i)
        {
            const auto& allow = policy.allows[i];
            out << "                    new PolicyRuleDescriptor(" << java_string(allow.action)
                << ", " << java_string(allow.condition) << ")";
            out << (i + 1 < policy.allows.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < policy.denies.size(); ++i)
        {
            const auto& deny = policy.denies[i];
            out << "                    new PolicyRuleDescriptor(" << java_string(deny.action)
                << ", " << java_string(deny.condition) << ")";
            out << (i + 1 < policy.denies.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < policy.quotas.size(); ++i)
        {
            const auto& quota = policy.quotas[i];
            out << "                    new QuotaDescriptor(" << java_string(quota.name) << ", "
                << java_string(quota.expression) << ")";
            out << (i + 1 < policy.quotas.size() ? "," : "") << "\n";
        }
        out << "                ),\n";
        out << "                List.of(";
        for (std::size_t i = 0; i < policy.audits.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << java_string(policy.audits[i]);
        }
        out << ")\n";
        out << "            )" << (policy_index + 1 < system.policies.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    return out.str();
}

} // namespace statespec
