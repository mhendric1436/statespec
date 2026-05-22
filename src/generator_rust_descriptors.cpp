#include "generator_rust_descriptors.hpp"

#include "generator_rust_descriptor_areas.hpp"
#include "generator_rust_descriptor_support.hpp"
#include "identifier_case.hpp"

#include <sstream>

namespace statespec
{

std::string generate_descriptors_rs(const IrSystem& system)
{
    std::ostringstream out;
    out << generate_rust_descriptor_prelude(system);
    out << generate_rust_feature_flag_descriptors(system);
    out << generate_rust_declaration_descriptors(system);
    out << generate_rust_external_system_descriptors(system);
    out << generate_rust_api_descriptors(system);
    out << generate_rust_worker_descriptors(system);
    out << generate_rust_policy_descriptors(system);
    out << generate_rust_shape_descriptors(system);
    out << generate_rust_observability_descriptors(system);
    out << generate_rust_entity_descriptors(system);
    out << generate_rust_runtime_descriptors(system);
    out << generate_rust_observability_registration(system);
    out << generate_rust_runtime_registration(system);
    return out.str();
}

std::string generate_workflow_step_handler_keys_rs(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "        " << rust_string(workflow.name + "." + step.name) << ",\n";
        }
    }
    return out.str();
}

std::string generate_api_operation_handler_methods_rs(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "    fn handle_" << snake_identifier(api.name)
            << "(&self, context: &ApiRequestContext) -> BackendResult<ApiResponse>;\n";
    }
    return out.str();
}

std::string generate_api_operation_dispatch_cases_rs(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "        " << rust_string(api.name) << " => handler.handle_"
            << snake_identifier(api.name) << "(context).map(Some),\n";
    }
    return out.str();
}

} // namespace statespec
