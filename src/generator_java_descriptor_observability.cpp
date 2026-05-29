#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string java_log_level_expr(IrLogLevel level)
{
    switch (level)
    {
    case IrLogLevel::Debug:
        return "Log.Level.DEBUG";
    case IrLogLevel::Warn:
        return "Log.Level.WARN";
    case IrLogLevel::Error:
        return "Log.Level.ERROR";
    case IrLogLevel::Info:
        return "Log.Level.INFO";
    }
    return "Log.Level.INFO";
}

std::string java_metric_kind_expr(IrMetricKind kind)
{
    switch (kind)
    {
    case IrMetricKind::Gauge:
        return "Metric.Kind.GAUGE";
    case IrMetricKind::Histogram:
        return "Metric.Kind.HISTOGRAM";
    case IrMetricKind::Counter:
        return "Metric.Kind.COUNTER";
    }
    return "Metric.Kind.COUNTER";
}

} // namespace

std::string generate_java_observability_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<Log.Definition> logDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t log_index = 0; log_index < system.logs.size(); ++log_index)
    {
        const auto& log = system.logs[log_index];
        out << "            new Log.Definition(\n";
        out << "                " << java_string(log.name) << ",\n";
        out << "                " << java_log_level_expr(log.level_kind) << ",\n";
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

    out << "    public static List<Metric.Definition> metricDefinitions() {\n";
    out << "        return List.of(\n";
    for (std::size_t metric_index = 0; metric_index < system.metrics.size(); ++metric_index)
    {
        const auto& metric = system.metrics[metric_index];
        out << "            new Metric.Definition(\n";
        out << "                " << java_string(metric.name) << ",\n";
        out << "                " << java_metric_kind_expr(metric.kind_value) << ",\n";
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
