#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_worker_descriptors(const IrSystem& system)
{
    std::ostringstream out;
    out << "    public static List<WorkerDescriptor> workerDescriptors() {\n";
    out << "        return List.of(\n";
    for (std::size_t worker_index = 0; worker_index < system.workers.size(); ++worker_index)
    {
        const auto& worker = system.workers[worker_index];
        out << "            com.statespec.generated.descriptors.workers."
            << pascal_identifier(worker.name) << "DescriptorModule.workerDescriptor()"
            << (worker_index + 1 < system.workers.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static List<WorkerContext> workerContexts() {\n";
    out << "        return List.of(\n";
    for (std::size_t worker_index = 0; worker_index < system.workers.size(); ++worker_index)
    {
        const auto& worker = system.workers[worker_index];
        out << "            com.statespec.generated.descriptors.workers."
            << pascal_identifier(worker.name) << "DescriptorModule.workerContext()"
            << (worker_index + 1 < system.workers.size() ? "," : "") << "\n";
    }
    out << "        );\n";
    out << "    }\n\n";

    return out.str();
}

std::string generate_java_worker_descriptor_module(
    const IrWorker& worker,
    std::string_view package_name,
    std::string_view class_name
)
{
    std::ostringstream out;
    out << "package " << package_name << ";\n\n";
    out << "import com.statespec.generated.Descriptors.WorkerContext;\n";
    out << "import com.statespec.generated.Descriptors.WorkerDescriptor;\n\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class " << class_name << " {\n";
    out << "    private " << class_name << "() {}\n\n";
    out << "    public static WorkerDescriptor workerDescriptor() {\n";
    out << "        return new WorkerDescriptor(\n";
    out << "            " << java_string(worker.name) << ",\n";
    out << "            " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "            " << java_optional_string_expr(worker.lease) << ",\n";
    out << "            " << java_optional_string_expr(worker.polls) << ",\n";
    out << "            " << java_optional_string_expr(worker.executes) << ",\n";
    out << "            " << worker.concurrency.value_or(1) << "\n";
    out << "        );\n";
    out << "    }\n\n";
    out << "    public static WorkerContext workerContext() {\n";
    out << "        return new WorkerContext(\n";
    out << "            " << java_string(worker.name) << ",\n";
    out << "            " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "            " << java_optional_string_expr(worker.lease) << ",\n";
    out << "            " << java_optional_string_expr(worker.polls) << ",\n";
    out << "            " << java_optional_string_expr(worker.executes) << ",\n";
    out << "            " << worker.concurrency.value_or(1) << "\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

} // namespace statespec
