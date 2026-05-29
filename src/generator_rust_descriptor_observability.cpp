#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string rust_log_level_expr(const std::string& level)
{
    if (level == "debug")
    {
        return "LogLevel::Debug";
    }
    if (level == "warn")
    {
        return "LogLevel::Warn";
    }
    if (level == "error")
    {
        return "LogLevel::Error";
    }
    return "LogLevel::Info";
}

std::string rust_metric_kind_expr(const std::string& kind)
{
    if (kind == "gauge")
    {
        return "MetricKind::Gauge";
    }
    if (kind == "histogram")
    {
        return "MetricKind::Histogram";
    }
    return "MetricKind::Counter";
}

} // namespace

std::string generate_rust_observability_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn log_definitions() -> Vec<LogDefinition> {\n";
    out << "    vec![\n";
    for (const auto& log : system.logs)
    {
        out << "        LogDefinition {\n";
        out << "            name: " << rust_string(log.name) << ".to_string(),\n";
        out << "            level: " << rust_log_level_expr(log.level) << ",\n";
        out << "            event_name: " << rust_string(log.event_name) << ".to_string(),\n";
        out << "            fields: vec![\n";
        for (const auto& field : log.fields)
        {
            out << "                " << rust_field_descriptor_expr(field) << ",\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn metric_definitions() -> Vec<MetricDefinition> {\n";
    out << "    vec![\n";
    for (const auto& metric : system.metrics)
    {
        out << "        MetricDefinition {\n";
        out << "            name: " << rust_string(metric.name) << ".to_string(),\n";
        out << "            kind: " << rust_metric_kind_expr(metric.kind) << ",\n";
        out << "            backend_name: " << rust_string(metric.backend_name)
            << ".to_string(),\n";
        out << "            unit: " << rust_string(metric.unit) << ".to_string(),\n";
        out << "            labels: vec![\n";
        for (const auto& label : metric.labels)
        {
            out << "                " << rust_field_descriptor_expr(label) << ",\n";
        }
        out << "            ],\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
