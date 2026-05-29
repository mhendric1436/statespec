#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>
#include <string_view>

namespace statespec
{
namespace
{

std::string cpp_log_level_expr(std::string_view level)
{
    if (level == "debug")
    {
        return "statespec::backend::LogLevel::Debug";
    }
    if (level == "warn")
    {
        return "statespec::backend::LogLevel::Warn";
    }
    if (level == "error")
    {
        return "statespec::backend::LogLevel::Error";
    }
    return "statespec::backend::LogLevel::Info";
}

std::string cpp_metric_kind_expr(std::string_view kind)
{
    if (kind == "gauge")
    {
        return "statespec::backend::MetricKind::Gauge";
    }
    if (kind == "histogram")
    {
        return "statespec::backend::MetricKind::Histogram";
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
        out << "            " << cpp_log_level_expr(log.level) << ",\n";
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
        out << "            " << cpp_metric_kind_expr(metric.kind) << ",\n";
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
