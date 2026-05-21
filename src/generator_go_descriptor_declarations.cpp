#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_go_declaration_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "func ValueDescriptors() []ValueDescriptor {\n";
    out << "\treturn []ValueDescriptor{\n";
    for (const auto& value : system.values)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(value.name) << ",\n";
        out << "\t\t\tType: " << go_string(value.type) << ",\n";
        out << "\t\t\tConstraint: " << string_ptr_expr(value.constraint) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func EnumDescriptors() []EnumDescriptor {\n";
    out << "\treturn []EnumDescriptor{\n";
    for (const auto& enum_decl : system.enums)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(enum_decl.name) << ",\n";
        out << "\t\t\tMembers: []EnumMemberDescriptor{\n";
        for (const auto& member : enum_decl.members)
        {
            out << "\t\t\t\t{Name: " << go_string(member.name)
                << ", Value: " << string_ptr_expr(member.value) << "},\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func EventDescriptors() []EventDescriptor {\n";
    out << "\treturn []EventDescriptor{\n";
    for (const auto& event : system.events)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(event.name) << ",\n";
        out << "\t\t\tFields: []FieldDescriptor{\n";
        for (const auto& field : event.fields)
        {
            out << "\t\t\t\t" << go_field_descriptor_expr(field) << ",\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
