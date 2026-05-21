#include "generator_java_descriptors.hpp"

#include "generator_java_descriptor_areas.hpp"
#include "generator_java_descriptor_support.hpp"

#include <sstream>
#include <vector>

namespace statespec
{

std::string generate_descriptors_java(const IrSystem& system)
{
    std::ostringstream out;
    out << generate_java_descriptor_prelude(system);
    out << generate_java_feature_flag_descriptors(system);
    out << generate_java_declaration_descriptors(system);
    out << generate_java_external_system_descriptors(system);
    out << generate_java_api_descriptors(system);
    out << generate_java_worker_descriptors(system);
    out << generate_java_policy_descriptors(system);
    out << generate_java_shape_descriptors(system);
    out << generate_java_observability_descriptors(system);
    out << generate_java_entity_descriptors(system);
    out << generate_java_runtime_descriptors(system);
    out << generate_java_observability_registration(system);
    out << generate_java_runtime_registration(system);
    out << "}\n";
    return out.str();
}

std::string generate_workflow_step_handler_keys_java(const IrSystem& system)
{
    std::ostringstream out;
    std::vector<std::string> keys;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            keys.push_back(workflow.name + "." + step.name);
        }
    }
    for (std::size_t i = 0; i < keys.size(); ++i)
    {
        out << "            " << java_string(keys[i]) << (i + 1 < keys.size() ? "," : "") << "\n";
    }
    return out.str();
}

} // namespace statespec
