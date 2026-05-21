#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_go_worker_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "func WorkerDescriptors() []WorkerDescriptor {\n";
    out << "\treturn []WorkerDescriptor{\n";
    for (const auto& worker : system.workers)
    {
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(worker.name) << ",\n";
        out << "\t\t\tSingleton: " << (worker.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "\t\t\tLease: " << string_ptr_expr(worker.lease) << ",\n";
        out << "\t\t\tPolls: " << string_ptr_expr(worker.polls) << ",\n";
        out << "\t\t\tExecutes: " << string_ptr_expr(worker.executes) << ",\n";
        out << "\t\t\tConcurrency: " << worker.concurrency.value_or(1) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    out << "func WorkerContexts() []WorkerContext {\n";
    out << "\treturn []WorkerContext{\n";
    for (const auto& worker : system.workers)
    {
        out << "\t\t{\n";
        out << "\t\t\tWorkerName: " << go_string(worker.name) << ",\n";
        out << "\t\t\tSingleton: " << (worker.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "\t\t\tLease: " << string_ptr_expr(worker.lease) << ",\n";
        out << "\t\t\tPolls: " << string_ptr_expr(worker.polls) << ",\n";
        out << "\t\t\tExecutes: " << string_ptr_expr(worker.executes) << ",\n";
        out << "\t\t\tConcurrency: " << worker.concurrency.value_or(1) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
