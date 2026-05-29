#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string java_feature_flag_type_expr(IrFeatureFlagType type)
{
    switch (type)
    {
    case IrFeatureFlagType::String:
        return "FeatureFlag.Type.STRING";
    case IrFeatureFlagType::Integer:
        return "FeatureFlag.Type.INT";
    case IrFeatureFlagType::Decimal:
        return "FeatureFlag.Type.DECIMAL";
    case IrFeatureFlagType::Bool:
        return "FeatureFlag.Type.BOOL";
    }
    return "FeatureFlag.Type.BOOL";
}

std::string java_feature_flag_scope_expr(IrFeatureFlagScopeKind scope)
{
    switch (scope)
    {
    case IrFeatureFlagScopeKind::System:
        return "FeatureFlag.ScopeKind.SYSTEM";
    case IrFeatureFlagScopeKind::User:
        return "FeatureFlag.ScopeKind.USER";
    case IrFeatureFlagScopeKind::Entity:
        return "FeatureFlag.ScopeKind.ENTITY";
    case IrFeatureFlagScopeKind::Tenant:
        return "FeatureFlag.ScopeKind.TENANT";
    }
    return "FeatureFlag.ScopeKind.TENANT";
}

std::string java_feature_flag_value_expr(
    IrFeatureFlagType type,
    const std::string& value
)
{
    switch (type)
    {
    case IrFeatureFlagType::String:
        return "new FeatureFlag.Value.StringValue(" + java_string(value) + ")";
    case IrFeatureFlagType::Integer:
        return "new FeatureFlag.Value.IntValue(" + value + "L)";
    case IrFeatureFlagType::Decimal:
        return "new FeatureFlag.Value.DecimalValue(" + java_string(value) + ")";
    case IrFeatureFlagType::Bool:
        return std::string{"new FeatureFlag.Value.BoolValue("} +
               (value == "true" ? "true" : "false") + ")";
    }
    return std::string{"new FeatureFlag.Value.BoolValue("} + (value == "true" ? "true" : "false") +
           ")";
}

} // namespace

std::string generate_java_feature_flag_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<FeatureFlag.Definition> featureFlagDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t i = 0; i < system.feature_flags.size(); ++i)
    {
        const auto& flag = system.feature_flags[i];
        out << "            new FeatureFlag.Definition(\n";
        out << "                " << java_string(flag.name) << ",\n";
        out << "                " << java_feature_flag_type_expr(flag.flag_type) << ",\n";
        out << "                "
            << java_feature_flag_value_expr(flag.flag_type, flag.default_value) << ",\n";
        out << "                " << java_feature_flag_scope_expr(flag.scope_kind) << ",\n";
        out << "                " << java_optional_string_expr(flag.owner) << ",\n";
        out << "                " << java_optional_string_expr(flag.description) << ",\n";
        out << "                " << java_optional_string_expr(flag.expires) << "\n";
        out << "            )" << (i + 1 < system.feature_flags.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    return out.str();
}

} // namespace statespec
