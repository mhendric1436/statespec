#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_worker_descriptor_module(const IrWorker& worker)
{
    std::ostringstream out;
    out << "use super::{WorkerContext, WorkerDescriptor};\n\n";
    out << "pub fn worker_descriptor() -> WorkerDescriptor {\n";
    out << "    WorkerDescriptor {\n";
    out << "        name: " << rust_string(worker.name) << ".to_string(),\n";
    out << "        singleton: " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "        lease: " << rust_optional_string_expr(worker.lease) << ",\n";
    out << "        polls: " << rust_optional_string_expr(worker.polls) << ",\n";
    out << "        executes: " << rust_optional_string_expr(worker.executes) << ",\n";
    out << "        concurrency: " << worker.concurrency.value_or(1) << ",\n";
    out << "    }\n";
    out << "}\n\n";
    out << "pub fn worker_context() -> WorkerContext {\n";
    out << "    WorkerContext {\n";
    out << "        worker_name: " << rust_string(worker.name) << ".to_string(),\n";
    out << "        singleton: " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "        lease: " << rust_optional_string_expr(worker.lease) << ",\n";
    out << "        polls: " << rust_optional_string_expr(worker.polls) << ",\n";
    out << "        executes: " << rust_optional_string_expr(worker.executes) << ",\n";
    out << "        concurrency: " << worker.concurrency.value_or(1) << ",\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

} // namespace statespec
