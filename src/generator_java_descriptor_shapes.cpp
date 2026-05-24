#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_shape_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& shape : system.shapes)
    {
        out << "    public static final String " << java_shape_name_constant_name(shape.name)
            << " = " << java_string(shape.name) << ";\n";
        for (const auto& field : shape.fields)
        {
            out << "    public static final String "
                << java_shape_field_constant_name(shape.name, field.name) << " = "
                << java_string(field.name) << ";\n";
            out << "    public static final String "
                << java_shape_field_type_name_constant_name(shape.name, field.name) << " = "
                << java_string(field.type) << ";\n";
        }
    }
    if (!system.shapes.empty())
    {
        out << "\n";
    }

    out << "    public static List<ShapeDescriptor> shapeDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t shape_index = 0; shape_index < system.shapes.size(); ++shape_index)
    {
        const auto& shape = system.shapes[shape_index];
        out << "            new ShapeDescriptor(\n";
        out << "                " << java_shape_name_constant_name(shape.name) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < shape.fields.size(); ++i)
        {
            const auto& field = shape.fields[i];
            out << "                    " << java_shape_field_descriptor_expr(shape.name, field);
            out << (i + 1 < shape.fields.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (shape_index + 1 < system.shapes.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    return out.str();
}

} // namespace statespec
