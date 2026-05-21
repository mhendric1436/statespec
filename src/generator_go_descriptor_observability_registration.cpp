#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_go_observability_registration(const IrSystem&)
{
    std::ostringstream out;
    out << "\nfunc logLevelFromDescriptor(level string) LogLevel {\n";
    out << "\tswitch level {\n";
    out << "\tcase \"debug\":\n";
    out << "\t\treturn LogLevelDebug\n";
    out << "\tcase \"warn\":\n";
    out << "\t\treturn LogLevelWarn\n";
    out << "\tcase \"error\":\n";
    out << "\t\treturn LogLevelError\n";
    out << "\tdefault:\n";
    out << "\t\treturn LogLevelInfo\n";
    out << "\t}\n";
    out << "}\n\n";

    out << "func metricKindFromDescriptor(kind string) MetricKind {\n";
    out << "\tswitch kind {\n";
    out << "\tcase \"gauge\":\n";
    out << "\t\treturn MetricGauge\n";
    out << "\tcase \"histogram\":\n";
    out << "\t\treturn MetricHistogram\n";
    out << "\tdefault:\n";
    out << "\t\treturn MetricCounter\n";
    out << "\t}\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
