#include "generator_cpp_descriptors.hpp"

#include "generator_cpp_descriptor_areas.hpp"
#include "generator_cpp_descriptor_support.hpp"
#include "identifier_case.hpp"

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

std::string generate_api_operation_handler_methods(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "    virtual ApiResponse handle_" << snake_identifier(api.name)
            << "(const ApiRequestContext& context) = 0;\n";
    }
    return out.str();
}

std::string generate_api_operation_dispatch_cases(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "    if (route->api_name == " << cpp_string(api.name) << ")\n";
        out << "    {\n";
        out << "        return handler.handle_" << snake_identifier(api.name) << "(context);\n";
        out << "    }\n";
    }
    return out.str();
}

std::string generate_api_operation_default_handler_methods(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "    ApiResponse handle_" << snake_identifier(api.name)
            << "(const ApiRequestContext&) override\n";
        out << "    {\n";
        out << "        return ApiResponse{501, statespec::backend::Json::object({{\"error\", "
               "\"handler_not_implemented\"}, {\"api\", "
            << cpp_string(api.name) << "}})};\n";
        out << "    }\n\n";
    }
    return out.str();
}

} // namespace statespec
