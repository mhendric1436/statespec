#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_worker_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "pub fn worker_descriptors() -> Vec<WorkerDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& worker : system.workers)
    {
        out << "        WorkerDescriptor {\n";
        out << "            name: " << rust_string(worker.name) << ".to_string(),\n";
        out << "            singleton: " << (worker.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "            lease: " << rust_optional_string_expr(worker.lease) << ",\n";
        out << "            polls: " << rust_optional_string_expr(worker.polls) << ",\n";
        out << "            executes: " << rust_optional_string_expr(worker.executes) << ",\n";
        out << "            concurrency: " << worker.concurrency.value_or(1) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    out << "pub fn worker_contexts() -> Vec<WorkerContext> {\n";
    out << "    vec![\n";
    for (const auto& worker : system.workers)
    {
        out << "        WorkerContext {\n";
        out << "            worker_name: " << rust_string(worker.name) << ".to_string(),\n";
        out << "            singleton: " << (worker.singleton.value_or(false) ? "true" : "false")
            << ",\n";
        out << "            lease: " << rust_optional_string_expr(worker.lease) << ",\n";
        out << "            polls: " << rust_optional_string_expr(worker.polls) << ",\n";
        out << "            executes: " << rust_optional_string_expr(worker.executes) << ",\n";
        out << "            concurrency: " << worker.concurrency.value_or(1) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
