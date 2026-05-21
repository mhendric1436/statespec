#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_feature_flag_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn feature_flag_definitions() -> Vec<FeatureFlagDefinition> {\n";
    out << "    vec![\n";
    for (const auto& flag : system.feature_flags)
    {
        out << "        FeatureFlagDefinition {\n";
        out << "            name: " << rust_string(flag.name) << ".to_string(),\n";
        out << "            flag_type: " << rust_string(flag.type) << ".to_string(),\n";
        out << "            default_value: " << rust_string(flag.default_value)
            << ".to_string(),\n";
        out << "            scope: " << rust_string(flag.scope) << ".to_string(),\n";
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
