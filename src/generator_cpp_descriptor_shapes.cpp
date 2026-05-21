#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_shape_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline std::vector<ShapeDescriptor> shape_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& shape : system.shapes)
    {
        out << "        ShapeDescriptor{\n";
        out << "            " << cpp_string(shape.name) << ",\n";
        out << "            {\n";
        for (const auto& field : shape.fields)
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
