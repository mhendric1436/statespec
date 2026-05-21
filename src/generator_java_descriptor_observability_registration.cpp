#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_observability_registration(const IrSystem&)
{
    std::ostringstream out;
    out << "\n    private static Log.Level logLevelFromDescriptor(String level) {\n";
    out << "        return switch (level.toLowerCase(Locale.ROOT)) {\n";
    out << "            case \"debug\" -> Log.Level.DEBUG;\n";
    out << "            case \"warn\" -> Log.Level.WARN;\n";
    out << "            case \"error\" -> Log.Level.ERROR;\n";
    out << "            default -> Log.Level.INFO;\n";
    out << "        };\n";
    out << "    }\n\n";

    out << "    private static Metric.Kind metricKindFromDescriptor(String kind) {\n";
    out << "        return switch (kind.toLowerCase(Locale.ROOT)) {\n";
    out << "            case \"gauge\" -> Metric.Kind.GAUGE;\n";
    out << "            case \"histogram\" -> Metric.Kind.HISTOGRAM;\n";
    out << "            default -> Metric.Kind.COUNTER;\n";
    out << "        };\n";
    out << "    }\n\n";

    return out.str();
}

} // namespace statespec
