#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_worker_descriptor_module(const IrWorker& worker)
{
    std::ostringstream out;
    out << "inline WorkerDescriptor " << snake_identifier(worker.name) << "_worker_descriptor()\n";
    out << "{\n";
    out << "    return WorkerDescriptor{\n";
    out << "        " << cpp_string(worker.name) << ",\n";
    out << "        " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "        " << optional_string_expr(worker.lease) << ",\n";
    out << "        " << optional_string_expr(worker.polls) << ",\n";
    out << "        " << optional_string_expr(worker.executes) << ",\n";
    out << "        " << worker.concurrency.value_or(1) << ",\n";
    out << "    };\n";
    out << "}\n\n";
    out << "inline WorkerContext " << snake_identifier(worker.name) << "_worker_context()\n";
    out << "{\n";
    out << "    return WorkerContext{\n";
    out << "        " << cpp_string(worker.name) << ",\n";
    out << "        " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "        " << optional_string_expr(worker.lease) << ",\n";
    out << "        " << optional_string_expr(worker.polls) << ",\n";
    out << "        " << optional_string_expr(worker.executes) << ",\n";
    out << "        " << worker.concurrency.value_or(1) << ",\n";
    out << "    };\n";
    out << "}\n\n";
    return out.str();
}

} // namespace statespec
