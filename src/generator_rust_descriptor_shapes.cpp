#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_shape_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn shape_descriptors() -> Vec<ShapeDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& shape : system.shapes)
    {
        out << "        ShapeDescriptor {\n";
        out << "            name: " << rust_string(shape.name) << ".to_string(),\n";
        out << "            fields: vec![\n";
        for (const auto& field : shape.fields)
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
