#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_go_feature_flag_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "func FeatureFlagDefinitions() []FeatureFlagDescriptor {\n";
    out << "\treturn []FeatureFlagDescriptor{\n";
    for (const auto& flag : system.feature_flags)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(flag.name) << ",\n";
        out << "\t\t\tType: " << go_string(flag.type) << ",\n";
        out << "\t\t\tDefaultValue: " << go_string(flag.default_value) << ",\n";
        out << "\t\t\tScope: " << go_string(flag.scope) << ",\n";
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
