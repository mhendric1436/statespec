#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_value_enum_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<ValueDescriptor> valueDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t value_index = 0; value_index < system.values.size(); ++value_index)
    {
        const auto& value = system.values[value_index];
        out << "            new ValueDescriptor(\n";
        out << "                " << java_string(value.name) << ",\n";
        out << "                " << java_string(value.type) << ",\n";
        out << "                " << java_optional_string_expr(value.constraint) << "\n";
        out << "            )" << (value_index + 1 < system.values.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<EnumDescriptor> enumDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t enum_index = 0; enum_index < system.enums.size(); ++enum_index)
    {
        const auto& enum_decl = system.enums[enum_index];
        out << "            new EnumDescriptor(\n";
        out << "                " << java_string(enum_decl.name) << ",\n";
        out << "                List.of(\n";
        for (std::size_t member_index = 0; member_index < enum_decl.members.size(); ++member_index)
        {
            const auto& member = enum_decl.members[member_index];
            out << "                    new EnumMemberDescriptor(" << java_string(member.name)
                << ", " << java_optional_string_expr(member.value) << ")";
            out << (member_index + 1 < enum_decl.members.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (enum_index + 1 < system.enums.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    return out.str();
}

std::string generate_java_event_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<EventDescriptor> eventDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t event_index = 0; event_index < system.events.size(); ++event_index)
    {
        const auto& event = system.events[event_index];
        out << "            new EventDescriptor(\n";
        out << "                " << java_string(event.name) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "                    " << java_field_descriptor_expr(field);
            out << (i + 1 < event.fields.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (event_index + 1 < system.events.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    return out.str();
}

std::string generate_java_declaration_descriptors(const IrSystem& system)
{
    return generate_java_value_enum_descriptors(system) + generate_java_event_descriptors(system);
}

} // namespace statespec
