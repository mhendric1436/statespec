#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_policy_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline std::vector<PolicyDescriptor> policy_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& policy : system.policies)
    {
        out << "        PolicyDescriptor{\n";
        out << "            " << cpp_string(policy.name) << ",\n";
        out << "            " << optional_string_expr(policy.tenant_scoped_by) << ",\n";
        out << "            {\n";
        for (const auto& allow : policy.allows)
        {
            out << "                PolicyRuleDescriptor{" << cpp_string(allow.action) << ", "
                << cpp_string(allow.condition) << "},\n";
        }
        out << "            },\n";
        out << "            {\n";
        for (const auto& deny : policy.denies)
        {
            out << "                PolicyRuleDescriptor{" << cpp_string(deny.action) << ", "
                << cpp_string(deny.condition) << "},\n";
        }
        out << "            },\n";
        out << "            {\n";
        for (const auto& quota : policy.quotas)
        {
            out << "                QuotaDescriptor{" << cpp_string(quota.name) << ", "
                << cpp_string(quota.expression) << "},\n";
        }
        out << "            },\n";
        out << "            {";
        for (std::size_t i = 0; i < policy.audits.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_string(policy.audits[i]);
        }
        out << "},\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
