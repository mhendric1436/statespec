#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_declaration_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline std::vector<ValueDescriptor> value_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& value : system.values)
    {
        out << "        ValueDescriptor{\n";
        out << "            " << cpp_string(value.name) << ",\n";
        out << "            " << cpp_string(value.type) << ",\n";
        out << "            " << optional_string_expr(value.constraint) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<EnumDescriptor> enum_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& enum_decl : system.enums)
    {
        out << "        EnumDescriptor{\n";
        out << "            " << cpp_string(enum_decl.name) << ",\n";
        out << "            {\n";
        for (const auto& member : enum_decl.members)
        {
            out << "                EnumMemberDescriptor{" << cpp_string(member.name) << ", "
                << optional_string_expr(member.value) << "},\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<EventDescriptor> event_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& event : system.events)
    {
        out << "        EventDescriptor{\n";
        out << "            " << cpp_string(event.name) << ",\n";
        out << "            {\n";
        for (const auto& field : event.fields)
        {
            out << "                " << cpp_field_descriptor_expr(field) << ",\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
