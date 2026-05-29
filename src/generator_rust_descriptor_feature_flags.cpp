#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string rust_feature_flag_type_expr(const std::string& type)
{
    if (type == "string")
    {
        return "FeatureFlagType::String";
    }
    if (type == "int")
    {
        return "FeatureFlagType::Integer";
    }
    if (type == "decimal")
    {
        return "FeatureFlagType::Decimal";
    }
    return "FeatureFlagType::Bool";
}

std::string rust_feature_flag_scope_expr(const std::string& scope)
{
    if (scope == "system")
    {
        return "FeatureFlagScopeKind::System";
    }
    if (scope == "user")
    {
        return "FeatureFlagScopeKind::User";
    }
    if (scope.rfind("entity ", 0) == 0)
    {
        return "FeatureFlagScopeKind::Entity";
    }
    return "FeatureFlagScopeKind::Tenant";
}

std::string rust_feature_flag_value_expr(
    const std::string& type,
    const std::string& value
)
{
    if (type == "string")
    {
        return "FeatureFlagValue::String(" + rust_string(value) + ".to_string())";
    }
    if (type == "int")
    {
        return "FeatureFlagValue::Integer(" + value + ")";
    }
    if (type == "decimal")
    {
        return "FeatureFlagValue::Decimal(" + value + ")";
    }
    return std::string{"FeatureFlagValue::Bool("} + (value == "true" ? "true" : "false") + ")";
}

} // namespace

std::string generate_rust_feature_flag_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn feature_flag_definitions() -> Vec<FeatureFlagDefinition> {\n";
    out << "    vec![\n";
    for (const auto& flag : system.feature_flags)
    {
        out << "        FeatureFlagDefinition {\n";
        out << "            name: " << rust_string(flag.name) << ".to_string(),\n";
        out << "            flag_type: " << rust_feature_flag_type_expr(flag.type) << ",\n";
        out << "            default_value: "
            << rust_feature_flag_value_expr(flag.type, flag.default_value) << ",\n";
        out << "            scope: " << rust_feature_flag_scope_expr(flag.scope) << ",\n";
        out << "            owner: " << rust_optional_string_expr(flag.owner) << ",\n";
        out << "            description: " << rust_optional_string_expr(flag.description) << ",\n";
        out << "            expires: " << rust_optional_string_expr(flag.expires) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
