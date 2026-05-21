#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_feature_flag_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<FeatureFlagDefinition> featureFlagDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t i = 0; i < system.feature_flags.size(); ++i)
    {
        const auto& flag = system.feature_flags[i];
        out << "            new FeatureFlagDefinition(\n";
        out << "                " << java_string(flag.name) << ",\n";
        out << "                " << java_string(flag.type) << ",\n";
        out << "                " << java_string(flag.default_value) << ",\n";
        out << "                " << java_string(flag.scope) << ",\n";
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
