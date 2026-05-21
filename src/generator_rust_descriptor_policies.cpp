#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_policy_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn policy_descriptors() -> Vec<PolicyDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& policy : system.policies)
    {
        out << "        PolicyDescriptor {\n";
        out << "            name: " << rust_string(policy.name) << ".to_string(),\n";
        out << "            tenant_scoped_by: "
            << rust_optional_string_expr(policy.tenant_scoped_by) << ",\n";
        out << "            allows: vec![\n";
        for (const auto& allow : policy.allows)
        {
            out << "                PolicyRuleDescriptor { action: " << rust_string(allow.action)
                << ".to_string(), condition: " << rust_string(allow.condition)
                << ".to_string() },\n";
        }
        out << "            ],\n";
        out << "            denies: vec![\n";
        for (const auto& deny : policy.denies)
        {
            out << "                PolicyRuleDescriptor { action: " << rust_string(deny.action)
                << ".to_string(), condition: " << rust_string(deny.condition)
                << ".to_string() },\n";
        }
        out << "            ],\n";
        out << "            quotas: vec![\n";
        for (const auto& quota : policy.quotas)
        {
            out << "                QuotaDescriptor { name: " << rust_string(quota.name)
                << ".to_string(), expression: " << rust_string(quota.expression)
                << ".to_string() },\n";
        }
        out << "            ],\n";
        out << "            audits: vec![";
        for (std::size_t i = 0; i < policy.audits.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_string(policy.audits[i]) << ".to_string()";
        }
        out << "],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
