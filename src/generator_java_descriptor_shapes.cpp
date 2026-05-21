#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_shape_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<ShapeDescriptor> shapeDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t shape_index = 0; shape_index < system.shapes.size(); ++shape_index)
    {
        const auto& shape = system.shapes[shape_index];
        out << "            new ShapeDescriptor(\n";
        out << "                " << java_string(shape.name) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < shape.fields.size(); ++i)
        {
            const auto& field = shape.fields[i];
            out << "                    " << java_field_descriptor_expr(field);
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
