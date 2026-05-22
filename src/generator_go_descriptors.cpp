#include "generator_go_descriptors.hpp"

#include "generator_go_descriptor_areas.hpp"
#include "generator_go_descriptor_support.hpp"
#include "identifier_case.hpp"

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

std::string generate_api_operation_handler_methods_go(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "\tHandle" << pascal_identifier(api.name)
            << "(context.Context, common.APIRequestContext) (common.APIResponse, error)\n";
    }
    return out.str();
}

std::string generate_api_operation_dispatch_cases_go(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "\tcase " << go_string(api.name) << ":\n";
        out << "\t\tresponse, err := handler.Handle" << pascal_identifier(api.name)
            << "(ctx, request)\n";
        out << "\t\tif err != nil {\n";
        out << "\t\t\treturn common.APIResponse{}, true, err\n";
        out << "\t\t}\n";
        out << "\t\treturn response, true, nil\n";
    }
    return out.str();
}

std::string generate_api_operation_default_handler_methods_go(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "func (DefaultAPITierHandler) Handle" << pascal_identifier(api.name)
            << "(context.Context, common.APIRequestContext) (common.APIResponse, error) {\n";
        out << "\treturn common.APIResponse{StatusCode: 501, Body: "
               "common.JSONObject(map[string]common.JSON{\"error\": "
               "common.JSONString(\"handler_not_implemented\"), \"api\": common.JSONString("
            << go_string(api.name) << ")})}, nil\n";
        out << "}\n\n";
    }
    return out.str();
}

} // namespace statespec
