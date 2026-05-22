#include "generator_rust_descriptors.hpp"

#include "generator_rust_descriptor_areas.hpp"
#include "generator_rust_descriptor_support.hpp"
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

std::string rust_decode_func(const IrField& field)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return "decode_bool";
    }
    if (base == "int" || base == "int32")
    {
        return "decode_i32";
    }
    if (base == "int64" || base == "long")
    {
        return "decode_i64";
    }
    if (base == "double" || base == "decimal")
    {
        return "decode_f64";
    }
    if (base == "json")
    {
        return "decode_json";
    }
    return "decode_string";
}

std::string rust_encode_expr(
    const IrField& field,
    const std::string& accessor
)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return "Json::Bool(" + accessor + ")";
    }
    if (base == "int" || base == "int32")
    {
        return "Json::Integer(i64::from(" + accessor + "))";
    }
    if (base == "int64" || base == "long")
    {
        return "Json::Integer(" + accessor + ")";
    }
    if (base == "double" || base == "decimal")
    {
        return "Json::Decimal(" + accessor + ")";
    }
    if (base == "json")
    {
        return accessor + ".clone()";
    }
    return "Json::String(" + accessor + ".clone())";
}

std::string rust_default_response_expr(
    const IrField& field,
    const std::string& context_expr
)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return field.name == "accepted" ? "true" : "false";
    }
    if (base == "int" || base == "int32")
    {
        return "0_i32";
    }
    if (base == "int64" || base == "long")
    {
        return "0_i64";
    }
    if (base == "double" || base == "decimal")
    {
        return "0.0_f64";
    }
    if (base == "json")
    {
        return context_expr + ".body.clone()";
    }
    if (field.name == "workflow_execution_id" || field.name == "message_id")
    {
        return "format!(\"{}:{}\", " + context_expr + ".api_name, " + context_expr +
               ".body.canonical_string())";
    }
    if (field.name == "status")
    {
        return "\"Accepted\".to_string()";
    }
    return "String::new()";
}

} // namespace

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

std::string generate_workflow_step_handler_methods_rs(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "    fn handle_" << snake_identifier(workflow.name + "_" + step.name)
                << "(&self, context: &WorkflowStepHandlerContext) -> BackendResult<()>;\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_dispatch_cases_rs(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "            (" << rust_string(workflow.name) << ", " << rust_string(step.name)
                << ") => self.handler.handle_" << snake_identifier(workflow.name + "_" + step.name)
                << "(&context),\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_next_cases_rs(const IrSystem& system)
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
                out << "            (" << rust_string(workflow.name) << ", "
                    << rust_string(step.name) << ") => Some(" << rust_string(*statement.target)
                    << ".to_string()),\n";
            }
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

std::string generate_api_operation_default_handler_methods_rs(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "    fn handle_" << snake_identifier(api.name)
            << "(&self, context: &ApiRequestContext) -> BackendResult<ApiResponse> {\n";
        if (api.input.has_value())
        {
            out << "        let request = crate::api_codecs::decode_" << snake_identifier(api.name)
                << "_request(context)?;\n";
            out << "        let _ = request;\n";
        }
        const auto status_code =
            api.starts_workflow.has_value() || api.enqueues.has_value() ? 202 : 200;
        if (api.output.has_value())
        {
            if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
            {
                out << "        let response = " << pascal_identifier(shape->name) << " {\n";
                for (const auto& field : shape->fields)
                {
                    out << "            " << field.name << ": ";
                    if (is_optional_type(field.type))
                    {
                        out << "Some(" << rust_default_response_expr(field, "context") << ")";
                    }
                    else
                    {
                        out << rust_default_response_expr(field, "context");
                    }
                    out << ",\n";
                }
                out << "        };\n";
                out << "        Ok(crate::api_codecs::encode_" << snake_identifier(api.name)
                    << "_response_with_status(&response, " << status_code << "))\n";
            }
            else
            {
                out << "        Ok(ApiResponse { status_code: " << status_code
                    << ", body: crate::json::Json::Object(std::collections::BTreeMap::new()) })\n";
            }
        }
        else
        {
            out << "        Ok(ApiResponse { status_code: " << status_code
                << ", body: crate::json::Json::Object(std::collections::BTreeMap::new()) })\n";
        }
        out << "    }\n\n";
    }
    return out.str();
}

std::string generate_api_codecs_rs(const IrSystem& system)
{
    std::ostringstream out;
    out << "fn codec_error(message: impl Into<String>) -> BackendError {\n";
    out << "    BackendError::Unsupported { message: message.into() }\n";
    out << "}\n\n";
    out << "fn require_member<'a>(value: &'a Json, field_name: &str) -> BackendResult<&'a Json> "
           "{\n";
    out << "    match value.find(field_name) {\n";
    out << "        Some(Json::Null) | None => Err(codec_error(format!(\"missing required API "
           "field {}\", field_name))),\n";
    out << "        Some(member) => Ok(member),\n";
    out << "    }\n";
    out << "}\n\n";
    out << "fn decode_string(value: &Json, field_name: &str) -> BackendResult<String> {\n";
    out << "    match value { Json::String(value) => Ok(value.clone()), _ => "
           "Err(codec_error(format!(\"API field {} must be a string\", field_name))) }\n";
    out << "}\n\n";
    out << "fn decode_bool(value: &Json, field_name: &str) -> BackendResult<bool> {\n";
    out << "    match value { Json::Bool(value) => Ok(*value), _ => Err(codec_error(format!(\"API "
           "field {} must be a bool\", field_name))) }\n";
    out << "}\n\n";
    out << "fn decode_i64(value: &Json, field_name: &str) -> BackendResult<i64> {\n";
    out << "    match value { Json::Integer(value) => Ok(*value), _ => "
           "Err(codec_error(format!(\"API field {} must be an integer\", field_name))) }\n";
    out << "}\n\n";
    out << "fn decode_i32(value: &Json, field_name: &str) -> BackendResult<i32> {\n";
    out << "    i32::try_from(decode_i64(value, field_name)?).map_err(|_| "
           "codec_error(format!(\"API field {} exceeds i32 range\", field_name)))\n";
    out << "}\n\n";
    out << "fn decode_f64(value: &Json, field_name: &str) -> BackendResult<f64> {\n";
    out << "    match value { Json::Decimal(value) => Ok(*value), Json::Integer(value) => "
           "Ok(*value as f64), _ => Err(codec_error(format!(\"API field {} must be a number\", "
           "field_name))) }\n";
    out << "}\n\n";
    out << "fn decode_json(value: &Json, _field_name: &str) -> BackendResult<Json> {\n";
    out << "    Ok(value.clone())\n";
    out << "}\n\n";

    for (const auto& api : system.apis)
    {
        if (api.input.has_value())
        {
            const auto* shape = find_shape(system, *api.input);
            if (shape != nullptr)
            {
                out << "pub fn decode_" << snake_identifier(api.name)
                    << "_request(request: &ApiRequestContext) -> BackendResult<"
                    << pascal_identifier(shape->name) << "> {\n";
                out << "    Ok(" << pascal_identifier(shape->name) << " {\n";
                for (const auto& field : shape->fields)
                {
                    out << "        " << field.name << ": ";
                    if (is_optional_type(field.type))
                    {
                        out << "match request.body.find(" << rust_string(field.name) << ") {\n";
                        out << "            Some(Json::Null) | None => None,\n";
                        out << "            Some(member) => Some(" << rust_decode_func(field)
                            << "(member, " << rust_string(field.name) << ")?),\n";
                        out << "        },\n";
                    }
                    else
                    {
                        out << rust_decode_func(field) << "(require_member(&request.body, "
                            << rust_string(field.name) << ")?, " << rust_string(field.name)
                            << ")?,\n";
                    }
                }
                out << "    })\n";
                out << "}\n\n";
            }
        }
        if (api.output.has_value())
        {
            const auto* shape = find_shape(system, *api.output);
            if (shape != nullptr)
            {
                out << "pub fn decode_" << snake_identifier(api.name)
                    << "_response(response: &ApiResponse) -> BackendResult<"
                    << pascal_identifier(shape->name) << "> {\n";
                out << "    Ok(" << pascal_identifier(shape->name) << " {\n";
                for (const auto& field : shape->fields)
                {
                    out << "        " << field.name << ": ";
                    if (is_optional_type(field.type))
                    {
                        out << "match response.body.find(" << rust_string(field.name) << ") {\n";
                        out << "            Some(Json::Null) | None => None,\n";
                        out << "            Some(member) => Some(" << rust_decode_func(field)
                            << "(member, " << rust_string(field.name) << ")?),\n";
                        out << "        },\n";
                    }
                    else
                    {
                        out << rust_decode_func(field) << "(require_member(&response.body, "
                            << rust_string(field.name) << ")?, " << rust_string(field.name)
                            << ")?,\n";
                    }
                }
                out << "    })\n";
                out << "}\n\n";

                out << "pub fn encode_" << snake_identifier(api.name) << "_response(response: &"
                    << pascal_identifier(shape->name) << ") -> ApiResponse {\n";
                out << "    encode_" << snake_identifier(api.name)
                    << "_response_with_status(response, 200)\n";
                out << "}\n\n";
                out << "pub fn encode_" << snake_identifier(api.name)
                    << "_response_with_status(response: &" << pascal_identifier(shape->name)
                    << ", status_code: i32) -> ApiResponse {\n";
                out << "    let mut body = BTreeMap::new();\n";
                for (const auto& field : shape->fields)
                {
                    if (is_optional_type(field.type))
                    {
                        const auto base = strip_optional_type(field.type);
                        const auto accessor = (base == "bool" || base == "int" || base == "int32" ||
                                               base == "int64" || base == "long" ||
                                               base == "double" || base == "decimal")
                                                  ? "*value"
                                                  : "value";
                        out << "    if let Some(value) = &response." << field.name << " {\n";
                        out << "        body.insert(" << rust_string(field.name) << ".to_string(), "
                            << rust_encode_expr(field, accessor) << ");\n";
                        out << "    }\n";
                    }
                    else
                    {
                        out << "    body.insert(" << rust_string(field.name) << ".to_string(), "
                            << rust_encode_expr(field, "response." + field.name) << ");\n";
                    }
                }
                out << "    ApiResponse { status_code, body: Json::Object(body) }\n";
                out << "}\n\n";
            }
        }
    }
    return out.str();
}

} // namespace statespec
