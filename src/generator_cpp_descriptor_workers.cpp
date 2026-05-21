#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_worker_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline std::vector<WorkerDescriptor> worker_descriptors()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& worker : system.workers)
    {
        out << "        WorkerDescriptor{\n";
        out << "            " << cpp_string(worker.name) << ",\n";
        out << "            " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
        out << "            " << optional_string_expr(worker.lease) << ",\n";
        out << "            " << optional_string_expr(worker.polls) << ",\n";
        out << "            " << optional_string_expr(worker.executes) << ",\n";
        out << "            " << worker.concurrency.value_or(1) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    out << "inline std::vector<WorkerContext> worker_contexts()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& worker : system.workers)
    {
        out << "        WorkerContext{\n";
        out << "            " << cpp_string(worker.name) << ",\n";
        out << "            " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
        out << "            " << optional_string_expr(worker.lease) << ",\n";
        out << "            " << optional_string_expr(worker.polls) << ",\n";
        out << "            " << optional_string_expr(worker.executes) << ",\n";
        out << "            " << worker.concurrency.value_or(1) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
