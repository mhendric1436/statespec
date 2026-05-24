#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_shape_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& shape : system.shapes)
    {
        out << "pub const " << rust_shape_name_constant_name(shape.name)
            << ": &str = " << rust_string(shape.name) << ";\n";
        for (const auto& field : shape.fields)
        {
            out << "pub const " << rust_shape_field_constant_name(shape.name, field.name)
                << ": &str = " << rust_string(field.name) << ";\n";
            out << "pub const " << rust_shape_field_type_name_constant_name(shape.name, field.name)
                << ": &str = " << rust_string(field.type) << ";\n";
        }
    }
    if (!system.shapes.empty())
    {
        out << "\n";
    }

    out << "pub fn shape_descriptors() -> Vec<ShapeDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& shape : system.shapes)
    {
        out << "        ShapeDescriptor {\n";
        out << "            name: " << rust_shape_name_constant_name(shape.name)
            << ".to_string(),\n";
        out << "            fields: vec![\n";
        for (const auto& field : shape.fields)
        {
            out << "                " << rust_shape_field_descriptor_expr(shape.name, field)
                << ",\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
