#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string go_log_level_expr(const std::string& level)
{
    if (level == "debug")
    {
        return "LogLevelDebug";
    }
    if (level == "warn")
    {
        return "LogLevelWarn";
    }
    if (level == "error")
    {
        return "LogLevelError";
    }
    return "LogLevelInfo";
}

std::string go_metric_kind_expr(const std::string& kind)
{
    if (kind == "gauge")
    {
        return "MetricGauge";
    }
    if (kind == "histogram")
    {
        return "MetricHistogram";
    }
    return "MetricCounter";
}

} // namespace

std::string generate_go_observability_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "func LogDefinitions() []LogSignalDefinition {\n";
    out << "\treturn []LogSignalDefinition{\n";
    for (const auto& log : system.logs)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(log.name) << ",\n";
        out << "\t\t\tLevel: " << go_log_level_expr(log.level) << ",\n";
        out << "\t\t\tEventName: " << go_string(log.event_name) << ",\n";
        out << "\t\t\tFields: []FieldDescriptor{\n";
        for (const auto& field : log.fields)
        {
            out << "\t\t\t\t" << go_field_descriptor_expr(field) << ",\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func MetricDefinitions() []MetricInstrumentDefinition {\n";
    out << "\treturn []MetricInstrumentDefinition{\n";
    for (const auto& metric : system.metrics)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(metric.name) << ",\n";
        out << "\t\t\tKind: " << go_metric_kind_expr(metric.kind) << ",\n";
        out << "\t\t\tBackendName: " << go_string(metric.backend_name) << ",\n";
        out << "\t\t\tUnit: " << go_string(metric.unit) << ",\n";
        out << "\t\t\tLabels: []FieldDescriptor{\n";
        for (const auto& label : metric.labels)
        {
            out << "\t\t\t\t" << go_field_descriptor_expr(label) << ",\n";
        }
        out << "\t\t\t},\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
