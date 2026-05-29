#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <algorithm>
#include <sstream>

namespace statespec
{

namespace
{

bool cpp_shape_is_api_contract(
    const IrSystem& system,
    std::string_view shape_name
)
{
    return std::any_of(
        system.apis.begin(), system.apis.end(),
        [&](const auto& api)
        {
            return (api.input.has_value() && *api.input == shape_name) ||
                   (api.output.has_value() && *api.output == shape_name);
        }
    );
}

bool cpp_has_common_shapes(const IrSystem& system)
{
    return std::any_of(
        system.shapes.begin(), system.shapes.end(),
        [&](const auto& shape) { return !cpp_shape_is_api_contract(system, shape.name); }
    );
}

std::string cpp_descriptor_module_includes(const IrSystem& system)
{
    std::ostringstream out;
    out << "#include \"descriptors/core.hpp\"\n";
    if (cpp_has_common_shapes(system))
    {
        out << "#include \"descriptors/shapes.hpp\"\n";
    }
    for (const auto& entity : system.entities)
    {
        out << "#include \"entities/" << snake_identifier(entity.name) << "/schema.hpp\"\n";
    }
    for (const auto& workflow : system.workflows)
    {
        out << "#include \"workflows/" << snake_identifier(workflow.name) << ".hpp\"\n";
    }
    return out.str();
}

} // namespace

std::string generate_cpp_descriptor_prelude(
    const IrSystem& system,
    const std::string&,
    const std::string&,
    const std::string& entity_repository_runtime
)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"backend.hpp\"\n";
    out << "#include \"external_system.hpp\"\n";
    out << "#include \"feature_flag.hpp\"\n";
    out << "#include \"lease.hpp\"\n";
    out << "#include \"log.hpp\"\n";
    out << "#include \"metric.hpp\"\n";
    out << "#include \"queue.hpp\"\n";
    out << "#include \"workflow.hpp\"\n\n";
    out << "#include \"shape_types.hpp\"\n";
    out << "#include \"descriptors/types.hpp\"\n\n";
    out << cpp_descriptor_module_includes(system) << "\n";
    out << "#include <chrono>\n";
    out << "#include <cstdint>\n";
    out << "#include <map>\n";
    out << "#include <optional>\n";
    out << "#include <string>\n";
    out << "#include <string_view>\n";
    out << "#include <utility>\n";
    out << "#include <vector>\n\n";
    out << "namespace statespec_generated\n";
    out << "{\n\n";
    out << entity_repository_runtime << "\n";

    return out.str();
}

} // namespace statespec
