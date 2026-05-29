#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string java_feature_flag_type_expr(const std::string& type)
{
    if (type == "string")
    {
        return "FeatureFlag.Type.STRING";
    }
    if (type == "int")
    {
        return "FeatureFlag.Type.INT";
    }
    if (type == "decimal")
    {
        return "FeatureFlag.Type.DECIMAL";
    }
    return "FeatureFlag.Type.BOOL";
}

std::string java_feature_flag_scope_expr(const std::string& scope)
{
    if (scope == "system")
    {
        return "FeatureFlag.ScopeKind.SYSTEM";
    }
    if (scope == "user")
    {
        return "FeatureFlag.ScopeKind.USER";
    }
    if (scope.rfind("entity ", 0) == 0)
    {
        return "FeatureFlag.ScopeKind.ENTITY";
    }
    return "FeatureFlag.ScopeKind.TENANT";
}

std::string java_feature_flag_value_expr(
    const std::string& type,
    const std::string& value
)
{
    if (type == "string")
    {
        return "new FeatureFlag.Value.StringValue(" + java_string(value) + ")";
    }
    if (type == "int")
    {
        return "new FeatureFlag.Value.IntValue(" + value + "L)";
    }
    if (type == "decimal")
    {
        return "new FeatureFlag.Value.DecimalValue(" + java_string(value) + ")";
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
        out << "                " << java_feature_flag_type_expr(flag.type) << ",\n";
        out << "                " << java_feature_flag_value_expr(flag.type, flag.default_value)
            << ",\n";
        out << "                " << java_feature_flag_scope_expr(flag.scope) << ",\n";
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
