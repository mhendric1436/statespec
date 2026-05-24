#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_shape_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& shape : system.shapes)
    {
        out << "inline constexpr const char* " << cpp_shape_name_constant_name(shape.name) << " = "
            << cpp_string(shape.name) << ";\n";
        for (const auto& field : shape.fields)
        {
            out << "inline constexpr const char* "
                << cpp_shape_field_constant_name(shape.name, field.name) << " = "
                << cpp_string(field.name) << ";\n";
            out << "inline constexpr const char* "
                << cpp_shape_field_type_name_constant_name(shape.name, field.name) << " = "
                << cpp_string(field.type) << ";\n";
        }
    }
    if (!system.shapes.empty())
    {
        out << "\n";
    }

    out << "inline std::vector<ShapeDescriptor> shape_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& shape : system.shapes)
    {
        out << "        ShapeDescriptor{\n";
        out << "            " << cpp_shape_name_constant_name(shape.name) << ",\n";
        out << "            {\n";
        for (const auto& field : shape.fields)
        {
            out << "                " << cpp_shape_field_descriptor_expr(shape.name, field)
                << ",\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
