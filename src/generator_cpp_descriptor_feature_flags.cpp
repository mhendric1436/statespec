#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>
#include <string_view>

namespace statespec
{
namespace
{

std::string cpp_feature_flag_type_expr(IrFeatureFlagType type)
{
    switch (type)
    {
    case IrFeatureFlagType::String:
        return "statespec::backend::FeatureFlagType::String";
    case IrFeatureFlagType::Integer:
        return "statespec::backend::FeatureFlagType::Integer";
    case IrFeatureFlagType::Decimal:
        return "statespec::backend::FeatureFlagType::Decimal";
    case IrFeatureFlagType::Bool:
        return "statespec::backend::FeatureFlagType::Bool";
    }
    return "statespec::backend::FeatureFlagType::Bool";
}

std::string cpp_feature_flag_scope_expr(IrFeatureFlagScopeKind scope)
{
    switch (scope)
    {
    case IrFeatureFlagScopeKind::System:
        return "statespec::backend::FeatureFlagScopeKind::System";
    case IrFeatureFlagScopeKind::User:
        return "statespec::backend::FeatureFlagScopeKind::User";
    case IrFeatureFlagScopeKind::Entity:
        return "statespec::backend::FeatureFlagScopeKind::Entity";
    case IrFeatureFlagScopeKind::Tenant:
        return "statespec::backend::FeatureFlagScopeKind::Tenant";
    }
    return "statespec::backend::FeatureFlagScopeKind::Tenant";
}

std::string cpp_feature_flag_value_expr(
    IrFeatureFlagType type,
    const std::string& value
)
{
    switch (type)
    {
    case IrFeatureFlagType::String:
        return "statespec::backend::FeatureFlagValue::string_value(" + cpp_string(value) + ")";
    case IrFeatureFlagType::Integer:
        return "statespec::backend::FeatureFlagValue::integer_value(" + value + ")";
    case IrFeatureFlagType::Decimal:
        return "statespec::backend::FeatureFlagValue::decimal_value(" + value + ")";
    case IrFeatureFlagType::Bool:
        return std::string{"statespec::backend::FeatureFlagValue::bool_value("} +
               (value == "true" ? "true" : "false") + ")";
    }
    return std::string{"statespec::backend::FeatureFlagValue::bool_value("} +
           (value == "true" ? "true" : "false") + ")";
}

} // namespace

std::string generate_cpp_feature_flag_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline std::vector<statespec::backend::FeatureFlagDefinition> "
           "feature_flag_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& flag : system.feature_flags)
    {
        out << "        statespec::backend::FeatureFlagDefinition{\n";
        out << "            " << cpp_string(flag.name) << ",\n";
        out << "            " << cpp_feature_flag_type_expr(flag.flag_type) << ",\n";
        out << "            " << cpp_feature_flag_value_expr(flag.flag_type, flag.default_value)
            << ",\n";
        out << "            " << cpp_feature_flag_scope_expr(flag.scope_kind) << ",\n";
        out << "            " << optional_string_expr(flag.owner) << ",\n";
        out << "            " << optional_string_expr(flag.description) << ",\n";
        out << "            " << optional_string_expr(flag.expires) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
