#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string go_feature_flag_type_expr(const std::string& type)
{
    if (type == "string")
    {
        return "FeatureFlagString";
    }
    if (type == "int")
    {
        return "FeatureFlagInteger";
    }
    if (type == "decimal")
    {
        return "FeatureFlagDecimal";
    }
    return "FeatureFlagBool";
}

std::string go_feature_flag_scope_expr(const std::string& scope)
{
    if (scope == "system")
    {
        return "FeatureFlagScopeSystem";
    }
    if (scope == "user")
    {
        return "FeatureFlagScopeUser";
    }
    if (scope.rfind("entity ", 0) == 0)
    {
        return "FeatureFlagScopeEntity";
    }
    return "FeatureFlagScopeTenant";
}

std::string go_feature_flag_value_expr(
    const std::string& type,
    const std::string& value
)
{
    if (type == "string")
    {
        return "StringFeatureFlagValue(" + go_string(value) + ")";
    }
    if (type == "int")
    {
        return "IntegerFeatureFlagValue(" + value + ")";
    }
    if (type == "decimal")
    {
        return "DecimalFeatureFlagValue(" + value + ")";
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
        out << "\t\t\tType: " << go_feature_flag_type_expr(flag.type) << ",\n";
        out << "\t\t\tDefaultValue: " << go_feature_flag_value_expr(flag.type, flag.default_value)
            << ",\n";
        out << "\t\t\tScope: " << go_feature_flag_scope_expr(flag.scope) << ",\n";
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
