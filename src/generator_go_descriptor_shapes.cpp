#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_go_shape_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "func ShapeDescriptors() []ShapeDescriptor {\n";
    out << "\treturn []ShapeDescriptor{\n";
    for (const auto& shape : system.shapes)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(shape.name) << ",\n";
        out << "\t\t\tFields: []FieldDescriptor{\n";
        for (const auto& field : shape.fields)
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
