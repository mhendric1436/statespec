#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_declaration_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn value_descriptors() -> Vec<ValueDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& value : system.values)
    {
        out << "        ValueDescriptor {\n";
        out << "            name: " << rust_string(value.name) << ".to_string(),\n";
        out << "            value_type: " << rust_string(value.type) << ".to_string(),\n";
        out << "            constraint: " << rust_optional_string_expr(value.constraint) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn enum_descriptors() -> Vec<EnumDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& enum_decl : system.enums)
    {
        out << "        EnumDescriptor {\n";
        out << "            name: " << rust_string(enum_decl.name) << ".to_string(),\n";
        out << "            members: vec![\n";
        for (const auto& member : enum_decl.members)
        {
            out << "                EnumMemberDescriptor { name: " << rust_string(member.name)
                << ".to_string(), value: " << rust_optional_string_expr(member.value) << " },\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn event_descriptors() -> Vec<EventDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& event : system.events)
    {
        out << "        EventDescriptor {\n";
        out << "            name: " << rust_string(event.name) << ".to_string(),\n";
        out << "            fields: vec![\n";
        for (const auto& field : event.fields)
        {
            out << "                " << rust_field_descriptor_expr(field) << ",\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
