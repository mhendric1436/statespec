#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string rust_feature_flag_type_expr(IrFeatureFlagType type)
{
    switch (type)
    {
    case IrFeatureFlagType::String:
        return "FeatureFlagType::String";
    case IrFeatureFlagType::Integer:
        return "FeatureFlagType::Integer";
    case IrFeatureFlagType::Decimal:
        return "FeatureFlagType::Decimal";
    case IrFeatureFlagType::Bool:
        return "FeatureFlagType::Bool";
    }
    return "FeatureFlagType::Bool";
}

std::string rust_feature_flag_scope_expr(IrFeatureFlagScopeKind scope)
{
    switch (scope)
    {
    case IrFeatureFlagScopeKind::System:
        return "FeatureFlagScopeKind::System";
    case IrFeatureFlagScopeKind::User:
        return "FeatureFlagScopeKind::User";
    case IrFeatureFlagScopeKind::Entity:
        return "FeatureFlagScopeKind::Entity";
    case IrFeatureFlagScopeKind::Tenant:
        return "FeatureFlagScopeKind::Tenant";
    }
    return "FeatureFlagScopeKind::Tenant";
}

std::string rust_feature_flag_value_expr(
    IrFeatureFlagType type,
    const std::string& value
)
{
    switch (type)
    {
    case IrFeatureFlagType::String:
        return "FeatureFlagValue::String(" + rust_string(value) + ".to_string())";
    case IrFeatureFlagType::Integer:
        return "FeatureFlagValue::Integer(" + value + ")";
    case IrFeatureFlagType::Decimal:
        return "FeatureFlagValue::Decimal(" + value + ")";
    case IrFeatureFlagType::Bool:
        return std::string{"FeatureFlagValue::Bool("} + (value == "true" ? "true" : "false") + ")";
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
        out << "            flag_type: " << rust_feature_flag_type_expr(flag.flag_type) << ",\n";
        out << "            default_value: "
            << rust_feature_flag_value_expr(flag.flag_type, flag.default_value) << ",\n";
        out << "            scope: " << rust_feature_flag_scope_expr(flag.scope_kind) << ",\n";
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
