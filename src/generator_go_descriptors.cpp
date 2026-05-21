#include "generator_go_descriptors.hpp"

#include "generator_go_descriptor_areas.hpp"
#include "generator_go_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_descriptors_go(const IrSystem& system)
{
    std::ostringstream out;
    out << generate_go_descriptor_prelude(system);
    out << generate_go_feature_flag_descriptors(system);
    out << generate_go_declaration_descriptors(system);
    out << generate_go_external_system_descriptors(system);
    out << generate_go_api_descriptors(system);
    out << generate_go_worker_descriptors(system);
    out << generate_go_policy_descriptors(system);
    out << generate_go_shape_descriptors(system);
    out << generate_go_observability_descriptors(system);
    out << generate_go_entity_descriptors(system);
    out << generate_go_runtime_descriptors(system);
    out << generate_go_observability_registration(system);
    out << generate_go_runtime_registration(system);
    return out.str();
}

std::string generate_workflow_step_handler_keys_go(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "\t\t" << go_string(workflow.name + "." + step.name) << ",\n";
        }
    }
    return out.str();
}

} // namespace statespec
