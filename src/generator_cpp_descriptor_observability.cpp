#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>
#include <string_view>

namespace statespec
{
namespace
{

std::string cpp_log_level_expr(IrLogLevel level)
{
    switch (level)
    {
    case IrLogLevel::Debug:
        return "statespec::backend::LogLevel::Debug";
    case IrLogLevel::Warn:
        return "statespec::backend::LogLevel::Warn";
    case IrLogLevel::Error:
        return "statespec::backend::LogLevel::Error";
    case IrLogLevel::Info:
        return "statespec::backend::LogLevel::Info";
    }
    return "statespec::backend::LogLevel::Info";
}

std::string cpp_metric_kind_expr(IrMetricKind kind)
{
    switch (kind)
    {
    case IrMetricKind::Gauge:
        return "statespec::backend::MetricKind::Gauge";
    case IrMetricKind::Histogram:
        return "statespec::backend::MetricKind::Histogram";
    case IrMetricKind::Counter:
        return "statespec::backend::MetricKind::Counter";
    }
    return "statespec::backend::MetricKind::Counter";
}

} // namespace

std::string generate_cpp_observability_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline std::vector<statespec::backend::LogDefinition> log_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& log : system.logs)
    {
        out << "        statespec::backend::LogDefinition{\n";
        out << "            " << cpp_string(log.name) << ",\n";
        out << "            " << cpp_log_level_expr(log.level_kind) << ",\n";
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

    out << "inline std::vector<statespec::backend::MetricDefinition> metric_definitions()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& metric : system.metrics)
    {
        out << "        statespec::backend::MetricDefinition{\n";
        out << "            " << cpp_string(metric.name) << ",\n";
        out << "            " << cpp_metric_kind_expr(metric.kind_value) << ",\n";
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
