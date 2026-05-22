#include "generator_cpp_descriptors.hpp"

#include "generator_cpp_descriptor_areas.hpp"
#include "generator_cpp_descriptor_support.hpp"
#include "identifier_case.hpp"
#include "type_syntax.hpp"

#include <sstream>

namespace statespec
{
namespace
{

const IrShape* find_shape(
    const IrSystem& system,
    const std::string& name
)
{
    for (const auto& shape : system.shapes)
    {
        if (shape.name == name)
        {
            return &shape;
        }
    }
    return nullptr;
}

std::string cpp_decode_expr(
    const IrField& field,
    std::string value_expr
)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return "decode_bool(" + value_expr + ", " + cpp_string(field.name) + ")";
    }
    if (base == "int" || base == "int32")
    {
        return "static_cast<std::int32_t>(decode_int64(" + value_expr + ", " +
               cpp_string(field.name) + "))";
    }
    if (base == "int64" || base == "long")
    {
        return "decode_int64(" + value_expr + ", " + cpp_string(field.name) + ")";
    }
    if (base == "double" || base == "decimal")
    {
        return "decode_double(" + value_expr + ", " + cpp_string(field.name) + ")";
    }
    if (base == "json")
    {
        return value_expr;
    }
    return "decode_string(" + value_expr + ", " + cpp_string(field.name) + ")";
}

std::string cpp_json_expr(
    const IrField& field,
    const std::string& accessor
)
{
    const auto base = strip_optional_type(field.type);
    if (base == "json")
    {
        return accessor;
    }
    return "statespec::backend::Json{" + accessor + "}";
}

} // namespace

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

std::string generate_workflow_step_handler_methods(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "    virtual void handle_" << snake_identifier(workflow.name + "_" + step.name)
                << "(const WorkflowStepHandlerContext& context) = 0;\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_dispatch_cases(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "            if (record.workflow_name == " << cpp_string(workflow.name)
                << " && record.current_step == " << cpp_string(step.name) << ")\n";
            out << "            {\n";
            out << "                handler_.handle_"
                << snake_identifier(workflow.name + "_" + step.name) << "(context);\n";
            out << "                handled = true;\n";
            out << "            }\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_next_cases(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            for (const auto& statement : step.statements)
            {
                if (statement.kind != "transition_to" || !statement.target.has_value())
                {
                    continue;
                }
                out << "            if (record.workflow_name == " << cpp_string(workflow.name)
                    << " && record.current_step == " << cpp_string(step.name) << ")\n";
                out << "            {\n";
                out << "                next_step = " << cpp_string(*statement.target) << ";\n";
                out << "            }\n";
            }
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

std::string generate_api_codecs(const IrSystem& system)
{
    std::ostringstream out;
    out << "inline const statespec::backend::Json& require_member(\n";
    out << "    const statespec::backend::Json& value,\n";
    out << "    std::string_view field_name\n";
    out << ")\n";
    out << "{\n";
    out << "    const auto* member = value.find(field_name);\n";
    out << "    if (member == nullptr || member->is_null())\n";
    out << "    {\n";
    out << "        throw std::runtime_error(\"missing required API field\");\n";
    out << "    }\n";
    out << "    return *member;\n";
    out << "}\n\n";
    out << "inline std::string decode_string(const statespec::backend::Json& value, "
           "std::string_view)\n";
    out << "{\n";
    out << "    return value.as_string();\n";
    out << "}\n\n";
    out << "inline bool decode_bool(const statespec::backend::Json& value, std::string_view)\n";
    out << "{\n";
    out << "    return value.as_bool();\n";
    out << "}\n\n";
    out << "inline std::int64_t decode_int64(const statespec::backend::Json& value, "
           "std::string_view)\n";
    out << "{\n";
    out << "    return value.as_int64();\n";
    out << "}\n\n";
    out << "inline double decode_double(const statespec::backend::Json& value, "
           "std::string_view)\n";
    out << "{\n";
    out << "    return value.as_double();\n";
    out << "}\n\n";
    for (const auto& api : system.apis)
    {
        if (api.input.has_value())
        {
            const auto* shape = find_shape(system, *api.input);
            if (shape != nullptr)
            {
                out << "inline ::statespec_generated::" << pascal_identifier(shape->name)
                    << " decode_" << snake_identifier(api.name)
                    << "_request(const ApiRequestContext& context)\n";
                out << "{\n";
                out << "    ::statespec_generated::" << pascal_identifier(shape->name)
                    << " decoded{};\n";
                for (const auto& field : shape->fields)
                {
                    if (is_optional_type(field.type))
                    {
                        out << "    if (const auto* member = context.body.find("
                            << cpp_string(field.name)
                            << "); member != nullptr && !member->is_null())\n";
                        out << "    {\n";
                        out << "        decoded." << field.name << " = "
                            << cpp_decode_expr(field, "*member") << ";\n";
                        out << "    }\n";
                    }
                    else
                    {
                        out << "    decoded." << field.name << " = "
                            << cpp_decode_expr(
                                   field,
                                   "require_member(context.body, " + cpp_string(field.name) + ")"
                               )
                            << ";\n";
                    }
                }
                out << "    return decoded;\n";
                out << "}\n\n";
            }
        }
        if (api.output.has_value())
        {
            const auto* shape = find_shape(system, *api.output);
            if (shape != nullptr)
            {
                out << "inline ApiResponse encode_" << snake_identifier(api.name)
                    << "_response(const ::statespec_generated::" << pascal_identifier(shape->name)
                    << "& response, int status_code = 200)\n";
                out << "{\n";
                out << "    statespec::backend::Json::Object body;\n";
                for (const auto& field : shape->fields)
                {
                    if (is_optional_type(field.type))
                    {
                        out << "    if (response." << field.name << ".has_value())\n";
                        out << "    {\n";
                        out << "        body.emplace(" << cpp_string(field.name) << ", "
                            << cpp_json_expr(field, "*response." + field.name) << ");\n";
                        out << "    }\n";
                    }
                    else
                    {
                        out << "    body.emplace(" << cpp_string(field.name) << ", "
                            << cpp_json_expr(field, "response." + field.name) << ");\n";
                    }
                }
                out << "    return ApiResponse{status_code, "
                       "statespec::backend::Json::object(std::move(body))};\n";
                out << "}\n\n";

                out << "inline ::statespec_generated::" << pascal_identifier(shape->name)
                    << " decode_" << snake_identifier(api.name)
                    << "_response(const ApiResponse& response)\n";
                out << "{\n";
                out << "    ::statespec_generated::" << pascal_identifier(shape->name)
                    << " decoded{};\n";
                for (const auto& field : shape->fields)
                {
                    if (is_optional_type(field.type))
                    {
                        out << "    if (const auto* member = response.body.find("
                            << cpp_string(field.name)
                            << "); member != nullptr && !member->is_null())\n";
                        out << "    {\n";
                        out << "        decoded." << field.name << " = "
                            << cpp_decode_expr(field, "*member") << ";\n";
                        out << "    }\n";
                    }
                    else
                    {
                        out << "    decoded." << field.name << " = "
                            << cpp_decode_expr(
                                   field,
                                   "require_member(response.body, " + cpp_string(field.name) + ")"
                               )
                            << ";\n";
                    }
                }
                out << "    return decoded;\n";
                out << "}\n\n";
            }
        }
    }
    return out.str();
}

} // namespace statespec
