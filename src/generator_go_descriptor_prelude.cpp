#include "generator_go_descriptor_areas.hpp"

#include "generator_go_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

namespace
{

std::string go_descriptor_module_imports(const IrSystem& system)
{
    std::ostringstream out;
    out << "\t_ \"statespec-generated/common/backend/descriptors\"\n";
    if (!system.workflows.empty())
    {
        out << "\tworkflows \"statespec-generated/common/backend/workflows\"\n";
    }
    return out.str();
}

} // namespace

std::string generate_go_descriptor_prelude(
    const IrSystem& system,
    const std::string& external_system_runtime,
    const std::string& external_system_metadata_runtime,
    const std::string& entity_repository_runtime
)
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import (\n";
    out << "\t\"context\"\n";
    out << "\t\"errors\"\n";
    out << go_descriptor_module_imports(system);
    out << "\t\"strings\"\n";
    out << "\t\"time\"\n";
    out << ")\n\n";
    out << generate_go_backend_descriptor_types(
        external_system_runtime, external_system_metadata_runtime, entity_repository_runtime
    );

    return out.str();
}

} // namespace statespec
