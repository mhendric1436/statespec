#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_observability_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline std::vector<LogDefinition> log_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& log : system.logs)
    {
        out << "        LogDefinition{\n";
        out << "            " << cpp_string(log.name) << ",\n";
        out << "            " << cpp_string(log.level) << ",\n";
        out << "            " << cpp_string(log.event_name) << ",\n";
        out << "            {\n";
        for (const auto& field : log.fields)
        {
            out << "                " << cpp_field_descriptor_expr(field) << ",\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<MetricDefinition> metric_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& metric : system.metrics)
    {
        out << "        MetricDefinition{\n";
        out << "            " << cpp_string(metric.name) << ",\n";
        out << "            " << cpp_string(metric.kind) << ",\n";
        out << "            " << cpp_string(metric.backend_name) << ",\n";
        out << "            " << cpp_string(metric.unit) << ",\n";
        out << "            {\n";
        for (const auto& label : metric.labels)
        {
            out << "                " << cpp_field_descriptor_expr(label) << ",\n";
        }
        out << "            },\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
