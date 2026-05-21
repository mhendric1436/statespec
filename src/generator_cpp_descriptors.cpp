#include "generator_cpp_descriptors.hpp"

#include "generator_cpp_descriptor_areas.hpp"
#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_system_descriptors_header(const IrSystem& system)
{
    std::ostringstream out;
    out << generate_cpp_descriptor_prelude(system);
    out << generate_cpp_feature_flag_descriptors(system);
    out << generate_cpp_declaration_descriptors(system);
    out << generate_cpp_external_system_descriptors(system);
    out << generate_cpp_api_descriptors(system);
    out << generate_cpp_worker_descriptors(system);
    out << generate_cpp_policy_descriptors(system);
    out << generate_cpp_shape_descriptors(system);
    out << generate_cpp_observability_descriptors(system);
    out << generate_cpp_entity_descriptors(system);
    out << generate_cpp_runtime_descriptors(system);
    out << generate_cpp_observability_registration(system);
    out << generate_cpp_runtime_registration(system);
    out << "} // namespace statespec_generated\n";
    return out.str();
}

std::string generate_workflow_step_handler_keys(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "        " << cpp_string(workflow.name + "." + step.name) << ",\n";
        }
    }
    return out.str();
}

} // namespace statespec
