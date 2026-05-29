#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string go_feature_flag_type_expr(IrFeatureFlagType type)
{
    switch (type)
    {
    case IrFeatureFlagType::String:
        return "FeatureFlagString";
    case IrFeatureFlagType::Integer:
        return "FeatureFlagInteger";
    case IrFeatureFlagType::Decimal:
        return "FeatureFlagDecimal";
    case IrFeatureFlagType::Bool:
        return "FeatureFlagBool";
    }
    return "FeatureFlagBool";
}

std::string go_feature_flag_scope_expr(IrFeatureFlagScopeKind scope)
{
    switch (scope)
    {
    case IrFeatureFlagScopeKind::System:
        return "FeatureFlagScopeSystem";
    case IrFeatureFlagScopeKind::User:
        return "FeatureFlagScopeUser";
    case IrFeatureFlagScopeKind::Entity:
        return "FeatureFlagScopeEntity";
    case IrFeatureFlagScopeKind::Tenant:
        return "FeatureFlagScopeTenant";
    }
    return "FeatureFlagScopeTenant";
}

std::string go_feature_flag_value_expr(
    IrFeatureFlagType type,
    const std::string& value
)
{
    switch (type)
    {
    case IrFeatureFlagType::String:
        return "StringFeatureFlagValue(" + go_string(value) + ")";
    case IrFeatureFlagType::Integer:
        return "IntegerFeatureFlagValue(" + value + ")";
    case IrFeatureFlagType::Decimal:
        return "DecimalFeatureFlagValue(" + value + ")";
    case IrFeatureFlagType::Bool:
        return std::string{"BoolFeatureFlagValue("} + (value == "true" ? "true" : "false") + ")";
    }
    return std::string{"BoolFeatureFlagValue("} + (value == "true" ? "true" : "false") + ")";
}

} // namespace

std::string generate_go_feature_flag_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "func FeatureFlagDefinitions() []FeatureFlagDefinition {\n";
    out << "\treturn []FeatureFlagDefinition{\n";
    for (const auto& flag : system.feature_flags)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(flag.name) << ",\n";
        out << "\t\t\tType: " << go_feature_flag_type_expr(flag.flag_type) << ",\n";
        out << "\t\t\tDefaultValue: "
            << go_feature_flag_value_expr(flag.flag_type, flag.default_value) << ",\n";
        out << "\t\t\tScope: " << go_feature_flag_scope_expr(flag.scope_kind) << ",\n";
        out << "\t\t\tOwner: " << string_ptr_expr(flag.owner) << ",\n";
        out << "\t\t\tDescription: " << string_ptr_expr(flag.description) << ",\n";
        out << "\t\t\tExpires: " << string_ptr_expr(flag.expires) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
