#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_observability_registration(const IrSystem&)
{
    std::ostringstream out;
    out << "inline statespec::backend::LogLevel log_level_from_string(std::string_view level)\n";
    out << "{\n";
    out << "    if (level == \"debug\") { return statespec::backend::LogLevel::Debug; }\n";
    out << "    if (level == \"warn\") { return statespec::backend::LogLevel::Warn; }\n";
    out << "    if (level == \"error\") { return statespec::backend::LogLevel::Error; }\n";
    out << "    return statespec::backend::LogLevel::Info;\n";
    out << "}\n\n";

    out << "inline statespec::backend::MetricKind metric_kind_from_string(std::string_view kind)\n";
    out << "{\n";
    out << "    if (kind == \"gauge\") { return statespec::backend::MetricKind::Gauge; }\n";
    out << "    if (kind == \"histogram\") { return statespec::backend::MetricKind::Histogram; }\n";
    out << "    return statespec::backend::MetricKind::Counter;\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
