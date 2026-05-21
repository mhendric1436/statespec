#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_observability_registration(const IrSystem&)
{
    std::ostringstream out;
    out << "\nfn log_level_from_descriptor(level: &str) -> LogLevel {\n";
    out << "    match level {\n";
    out << "        \"debug\" => LogLevel::Debug,\n";
    out << "        \"warn\" => LogLevel::Warn,\n";
    out << "        \"error\" => LogLevel::Error,\n";
    out << "        _ => LogLevel::Info,\n";
    out << "    }\n";
    out << "}\n\n";

    out << "fn metric_kind_from_descriptor(kind: &str) -> MetricKind {\n";
    out << "    match kind {\n";
    out << "        \"gauge\" => MetricKind::Gauge,\n";
    out << "        \"histogram\" => MetricKind::Histogram,\n";
    out << "        _ => MetricKind::Counter,\n";
    out << "    }\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
