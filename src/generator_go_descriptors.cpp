#include "generator_go_descriptors.hpp"

#include "generator_go_descriptor_areas.hpp"
#include "generator_go_descriptor_support.hpp"
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

std::string go_decode_func(const IrField& field)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return "decodeBool";
    }
    if (base == "int" || base == "int32")
    {
        return "decodeInt32";
    }
    if (base == "int64" || base == "long")
    {
        return "decodeInt64";
    }
    if (base == "double" || base == "decimal")
    {
        return "decodeFloat64";
    }
    if (base == "json")
    {
        return "decodeJSON";
    }
    return "decodeString";
}

std::string go_encode_expr(
    const IrField& field,
    const std::string& accessor
)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return "common.JSONBool(" + accessor + ")";
    }
    if (base == "int" || base == "int32")
    {
        return "common.JSONInt(int64(" + accessor + "))";
    }
    if (base == "int64" || base == "long")
    {
        return "common.JSONInt(" + accessor + ")";
    }
    if (base == "double" || base == "decimal")
    {
        return "mustJSONFloat(" + accessor + ")";
    }
    if (base == "json")
    {
        return accessor;
    }
    return "common.JSONString(" + accessor + ")";
}

std::string go_default_response_expr(
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
        return "int32(0)";
    }
    if (base == "int64" || base == "long")
    {
        return "int64(0)";
    }
    if (base == "double" || base == "decimal")
    {
        return "float64(0)";
    }
    if (base == "json")
    {
        return context_expr + ".Body";
    }
    if (field.name == "workflow_execution_id" || field.name == "message_id")
    {
        return context_expr + ".APIName + \":\" + " + context_expr + ".Body.CanonicalString()";
    }
    if (field.name == "status")
    {
        return "\"Accepted\"";
    }
    return "\"\"";
}

} // namespace

std::string generate_descriptors_go(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    std::ostringstream out;
    out << generate_go_descriptor_prelude(
        system, templates.load("generated/external_system_runtime.go.tmpl"),
        templates.load("generated/external_system_metadata_runtime.go.tmpl"),
        templates.load("generated/entity_repository.go.tmpl")
    );
    out << generate_go_feature_flag_descriptors(system);
    out << generate_go_declaration_descriptors(system);
    out << generate_go_external_system_descriptors(
        system, templates.load("generated/external_system_call_adapters.go.tmpl")
    );
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

std::string generate_workflow_step_handler_methods_go(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "\tHandle" << pascal_identifier(workflow.name + "_" + step.name)
                << "(context.Context, WorkflowStepHandlerContext) error\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_dispatch_cases_go(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "\tcase record.WorkflowName == " << go_string(workflow.name)
                << " && record.CurrentStep == " << go_string(step.name) << ":\n";
            out << "\t\thandlerErr = runner.Handler.Handle"
                << pascal_identifier(workflow.name + "_" + step.name) << "(ctx, stepContext)\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_next_cases_go(const IrSystem& system)
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
                out << "\tif record.WorkflowName == " << go_string(workflow.name)
                    << " && record.CurrentStep == " << go_string(step.name) << " {\n";
                out << "\t\tnextStepValue := " << go_string(*statement.target) << "\n";
                out << "\t\tnextStep = &nextStepValue\n";
                out << "\t}\n";
            }
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
            << "(ctx context.Context, request common.APIRequestContext) (common.APIResponse, "
               "error) {\n";
        out << "\t_ = ctx\n";
        if (api.input.has_value())
        {
            out << "\tdecodedRequest, err := Decode" << pascal_identifier(api.name)
                << "Request(request)\n";
            out << "\tif err != nil { return common.APIResponse{}, err }\n";
            out << "\t_ = decodedRequest\n";
        }
        const auto status_code =
            api.starts_workflow.has_value() || api.enqueues.has_value() ? 202 : 200;
        if (api.output.has_value())
        {
            if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
            {
                out << "\tresponse := common." << pascal_identifier(shape->name) << "{}\n";
                for (const auto& field : shape->fields)
                {
                    const auto access = "response." + pascal_identifier(field.name);
                    const auto value = go_default_response_expr(field, "request");
                    if (is_optional_type(field.type))
                    {
                        out << "\t" << lower_camel_identifier(field.name) << " := " << value
                            << "\n";
                        out << "\t" << access << " = &" << lower_camel_identifier(field.name)
                            << "\n";
                    }
                    else
                    {
                        out << "\t" << access << " = " << value << "\n";
                    }
                }
                out << "\treturn Encode" << pascal_identifier(api.name)
                    << "ResponseWithStatus(response, " << status_code << "), nil\n";
            }
            else
            {
                out << "\treturn common.APIResponse{StatusCode: " << status_code
                    << ", Body: common.JSONObject(map[string]common.JSON{})}, nil\n";
            }
        }
        else
        {
            out << "\treturn common.APIResponse{StatusCode: " << status_code
                << ", Body: common.JSONObject(map[string]common.JSON{})}, nil\n";
        }
        out << "}\n\n";
    }
    return out.str();
}

std::string generate_api_codecs_go(const IrSystem& system)
{
    std::ostringstream out;
    out << "func requireMember(value common.JSON, fieldName string) (common.JSON, error) {\n";
    out << "\tmember, ok := value.Find(fieldName)\n";
    out << "\tif !ok || member.IsNull() {\n";
    out << "\t\treturn common.JSON{}, fmt.Errorf(\"missing required API field %q\", fieldName)\n";
    out << "\t}\n";
    out << "\treturn member, nil\n";
    out << "}\n\n";
    out << "func decodeString(value common.JSON, fieldName string) (string, error) {\n";
    out << "\tdecoded, ok := value.AsString()\n";
    out << "\tif !ok { return \"\", fmt.Errorf(\"API field %q must be a string\", fieldName) }\n";
    out << "\treturn decoded, nil\n";
    out << "}\n\n";
    out << "func decodeBool(value common.JSON, fieldName string) (bool, error) {\n";
    out << "\tdecoded, ok := value.AsBool()\n";
    out << "\tif !ok { return false, fmt.Errorf(\"API field %q must be a bool\", fieldName) }\n";
    out << "\treturn decoded, nil\n";
    out << "}\n\n";
    out << "func decodeInt64(value common.JSON, fieldName string) (int64, error) {\n";
    out << "\tdecoded, ok := value.AsInt()\n";
    out << "\tif !ok { return 0, fmt.Errorf(\"API field %q must be an integer\", fieldName) }\n";
    out << "\treturn decoded, nil\n";
    out << "}\n\n";
    out << "func decodeInt32(value common.JSON, fieldName string) (int32, error) {\n";
    out << "\tdecoded, err := decodeInt64(value, fieldName)\n";
    out << "\tif err != nil { return 0, err }\n";
    out << "\treturn int32(decoded), nil\n";
    out << "}\n\n";
    out << "func decodeFloat64(value common.JSON, fieldName string) (float64, error) {\n";
    out << "\tdecoded, ok := value.AsFloat()\n";
    out << "\tif !ok { return 0, fmt.Errorf(\"API field %q must be a number\", fieldName) }\n";
    out << "\treturn decoded, nil\n";
    out << "}\n\n";
    out << "func decodeJSON(value common.JSON, _ string) (common.JSON, error) {\n";
    out << "\treturn value, nil\n";
    out << "}\n\n";
    out << "func mustJSONFloat(value float64) common.JSON {\n";
    out << "\tencoded, err := common.JSONFloat(value)\n";
    out << "\tif err != nil { panic(err) }\n";
    out << "\treturn encoded\n";
    out << "}\n\n";

    for (const auto& api : system.apis)
    {
        if (api.input.has_value())
        {
            const auto* shape = find_shape(system, *api.input);
            if (shape != nullptr)
            {
                out << "func Decode" << pascal_identifier(api.name)
                    << "Request(request common.APIRequestContext) (common."
                    << pascal_identifier(shape->name) << ", error) {\n";
                out << "\tvar decoded common." << pascal_identifier(shape->name) << "\n";
                for (const auto& field : shape->fields)
                {
                    const auto field_name = go_string(field.name);
                    const auto field_var = lower_camel_identifier(field.name);
                    const auto field_access = pascal_identifier(field.name);
                    if (is_optional_type(field.type))
                    {
                        out << "\tif member, ok := request.Body.Find(" << field_name
                            << "); ok && !member.IsNull() {\n";
                        out << "\t\t" << field_var << ", err := " << go_decode_func(field)
                            << "(member, " << field_name << ")\n";
                        out << "\t\tif err != nil { return common."
                            << pascal_identifier(shape->name) << "{}, err }\n";
                        out << "\t\tdecoded." << field_access << " = &" << field_var << "\n";
                        out << "\t}\n";
                    }
                    else
                    {
                        out << "\t" << field_var << "JSON, err := requireMember(request.Body, "
                            << field_name << ")\n";
                        out << "\tif err != nil { return common." << pascal_identifier(shape->name)
                            << "{}, err }\n";
                        out << "\t" << field_var << ", err := " << go_decode_func(field) << "("
                            << field_var << "JSON, " << field_name << ")\n";
                        out << "\tif err != nil { return common." << pascal_identifier(shape->name)
                            << "{}, err }\n";
                        out << "\tdecoded." << field_access << " = " << field_var << "\n";
                    }
                }
                out << "\treturn decoded, nil\n";
                out << "}\n\n";
            }
        }

        if (api.output.has_value())
        {
            const auto* shape = find_shape(system, *api.output);
            if (shape != nullptr)
            {
                out << "func Decode" << pascal_identifier(api.name)
                    << "Response(response common.APIResponse) (common."
                    << pascal_identifier(shape->name) << ", error) {\n";
                out << "\trequest := common.APIRequestContext{Body: response.Body}\n";
                out << "\treturn Decode" << pascal_identifier(api.name)
                    << "ResponseBody(request)\n";
                out << "}\n\n";

                out << "func Decode" << pascal_identifier(api.name)
                    << "ResponseBody(request common.APIRequestContext) (common."
                    << pascal_identifier(shape->name) << ", error) {\n";
                out << "\tvar decoded common." << pascal_identifier(shape->name) << "\n";
                for (const auto& field : shape->fields)
                {
                    const auto field_name = go_string(field.name);
                    const auto field_var = lower_camel_identifier(field.name);
                    const auto field_access = pascal_identifier(field.name);
                    if (is_optional_type(field.type))
                    {
                        out << "\tif member, ok := request.Body.Find(" << field_name
                            << "); ok && !member.IsNull() {\n";
                        out << "\t\t" << field_var << ", err := " << go_decode_func(field)
                            << "(member, " << field_name << ")\n";
                        out << "\t\tif err != nil { return common."
                            << pascal_identifier(shape->name) << "{}, err }\n";
                        out << "\t\tdecoded." << field_access << " = &" << field_var << "\n";
                        out << "\t}\n";
                    }
                    else
                    {
                        out << "\t" << field_var << "JSON, err := requireMember(request.Body, "
                            << field_name << ")\n";
                        out << "\tif err != nil { return common." << pascal_identifier(shape->name)
                            << "{}, err }\n";
                        out << "\t" << field_var << ", err := " << go_decode_func(field) << "("
                            << field_var << "JSON, " << field_name << ")\n";
                        out << "\tif err != nil { return common." << pascal_identifier(shape->name)
                            << "{}, err }\n";
                        out << "\tdecoded." << field_access << " = " << field_var << "\n";
                    }
                }
                out << "\treturn decoded, nil\n";
                out << "}\n\n";

                out << "func Encode" << pascal_identifier(api.name) << "Response(response common."
                    << pascal_identifier(shape->name) << ") common.APIResponse {\n";
                out << "\treturn Encode" << pascal_identifier(api.name)
                    << "ResponseWithStatus(response, 200)\n";
                out << "}\n\n";
                out << "func Encode" << pascal_identifier(api.name)
                    << "ResponseWithStatus(response common." << pascal_identifier(shape->name)
                    << ", statusCode int) common.APIResponse {\n";
                out << "\tbody := map[string]common.JSON{}\n";
                for (const auto& field : shape->fields)
                {
                    const auto access = "response." + pascal_identifier(field.name);
                    if (is_optional_type(field.type))
                    {
                        out << "\tif " << access << " != nil {\n";
                        out << "\t\tbody[" << go_string(field.name)
                            << "] = " << go_encode_expr(field, "*" + access) << "\n";
                        out << "\t}\n";
                    }
                    else
                    {
                        out << "\tbody[" << go_string(field.name)
                            << "] = " << go_encode_expr(field, access) << "\n";
                    }
                }
                out << "\treturn common.APIResponse{StatusCode: statusCode, Body: "
                       "common.JSONObject(body)}\n";
                out << "}\n\n";
            }
        }
    }
    return out.str();
}

} // namespace statespec
