#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_feature_flag_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline std::vector<FeatureFlagDefinition> feature_flag_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& flag : system.feature_flags)
    {
        out << "        FeatureFlagDefinition{\n";
        out << "            " << cpp_string(flag.name) << ",\n";
        out << "            " << cpp_string(flag.type) << ",\n";
        out << "            " << cpp_string(flag.default_value) << ",\n";
        out << "            " << cpp_string(flag.scope) << ",\n";
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
