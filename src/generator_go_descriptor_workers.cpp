#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_go_worker_descriptor_module(const IrWorker& worker)
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "func " << pascal_identifier(worker.name) << "WorkerDescriptor() WorkerDescriptor {\n";
    out << "\treturn WorkerDescriptor{\n";
    out << "\t\tName: " << go_string(worker.name) << ",\n";
    out << "\t\tSingleton: " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "\t\tLease: " << string_ptr_expr(worker.lease) << ",\n";
    out << "\t\tPolls: " << string_ptr_expr(worker.polls) << ",\n";
    out << "\t\tExecutes: " << string_ptr_expr(worker.executes) << ",\n";
    out << "\t\tConcurrency: " << worker.concurrency.value_or(1) << ",\n";
    out << "\t}\n";
    out << "}\n\n";
    out << "func " << pascal_identifier(worker.name) << "WorkerContext() WorkerContext {\n";
    out << "\treturn WorkerContext{\n";
    out << "\t\tWorkerName: " << go_string(worker.name) << ",\n";
    out << "\t\tSingleton: " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "\t\tLease: " << string_ptr_expr(worker.lease) << ",\n";
    out << "\t\tPolls: " << string_ptr_expr(worker.polls) << ",\n";
    out << "\t\tExecutes: " << string_ptr_expr(worker.executes) << ",\n";
    out << "\t\tConcurrency: " << worker.concurrency.value_or(1) << ",\n";
    out << "\t}\n";
    out << "}\n";
    return out.str();
}

} // namespace statespec
