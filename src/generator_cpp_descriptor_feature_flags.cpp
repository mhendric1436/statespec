#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>
#include <string_view>

namespace statespec
{
namespace
{

std::string cpp_feature_flag_type_expr(std::string_view type)
{
    if (type == "string")
    {
        return "statespec::backend::FeatureFlagType::String";
    }
    if (type == "int" || type == "integer")
    {
        return "statespec::backend::FeatureFlagType::Integer";
    }
    if (type == "decimal")
    {
        return "statespec::backend::FeatureFlagType::Decimal";
    }
    return "statespec::backend::FeatureFlagType::Bool";
}

std::string cpp_feature_flag_scope_expr(std::string_view scope)
{
    if (scope == "system")
    {
        return "statespec::backend::FeatureFlagScopeKind::System";
    }
    if (scope == "user")
    {
        return "statespec::backend::FeatureFlagScopeKind::User";
    }
    if (scope.rfind("entity ", 0) == 0)
    {
        return "statespec::backend::FeatureFlagScopeKind::Entity";
    }
    return "statespec::backend::FeatureFlagScopeKind::Tenant";
}

std::string cpp_feature_flag_value_expr(
    std::string_view type,
    std::string_view value
)
{
    if (type == "string")
    {
        return "statespec::backend::FeatureFlagValue::string_value(" +
               cpp_string(std::string{value}) + ")";
    }
    if (type == "int" || type == "integer")
    {
        return "statespec::backend::FeatureFlagValue::integer_value(" + std::string{value} + ")";
    }
    if (type == "decimal")
    {
        return "statespec::backend::FeatureFlagValue::decimal_value(" + std::string{value} + ")";
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
        out << "            " << cpp_feature_flag_type_expr(flag.type) << ",\n";
        out << "            " << cpp_feature_flag_value_expr(flag.type, flag.default_value)
            << ",\n";
        out << "            " << cpp_feature_flag_scope_expr(flag.scope) << ",\n";
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
