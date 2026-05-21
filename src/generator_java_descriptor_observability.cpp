#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_observability_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<LogDefinition> logDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t log_index = 0; log_index < system.logs.size(); ++log_index)
    {
        const auto& log = system.logs[log_index];
        out << "            new LogDefinition(\n";
        out << "                " << java_string(log.name) << ",\n";
        out << "                " << java_string(log.level) << ",\n";
        out << "                " << java_string(log.event_name) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < log.fields.size(); ++i)
        {
            const auto& field = log.fields[i];
            out << "                    " << java_field_descriptor_expr(field);
            out << (i + 1 < log.fields.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (log_index + 1 < system.logs.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<MetricDefinition> metricDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t metric_index = 0; metric_index < system.metrics.size(); ++metric_index)
    {
        const auto& metric = system.metrics[metric_index];
        out << "            new MetricDefinition(\n";
        out << "                " << java_string(metric.name) << ",\n";
        out << "                " << java_string(metric.kind) << ",\n";
        out << "                " << java_string(metric.backend_name) << ",\n";
        out << "                " << java_string(metric.unit) << ",\n";
        out << "                List.of(\n";
        for (std::size_t i = 0; i < metric.labels.size(); ++i)
        {
            const auto& label = metric.labels[i];
            out << "                    " << java_field_descriptor_expr(label);
            out << (i + 1 < metric.labels.size() ? "," : "") << "\n";
        }
        out << "                )\n";
        out << "            )" << (metric_index + 1 < system.metrics.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    return out.str();
}

} // namespace statespec
