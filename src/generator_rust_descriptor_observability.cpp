#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_observability_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn log_definitions() -> Vec<LogDefinition> {\n";
    out << "    vec![\n";
    for (const auto& log : system.logs)
    {
        out << "        LogDefinition {\n";
        out << "            name: " << rust_string(log.name) << ".to_string(),\n";
        out << "            level: " << rust_string(log.level) << ".to_string(),\n";
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
        out << "            kind: " << rust_string(metric.kind) << ".to_string(),\n";
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
