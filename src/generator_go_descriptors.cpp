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

const IrEntity* find_entity(
    const IrSystem& system,
    const std::string& name
)
{
    for (const auto& entity : system.entities)
    {
        if (entity.name == name)
        {
            return &entity;
        }
    }
    return nullptr;
}

const IrField* find_field(
    const IrShape& shape,
    const std::string& name
)
{
    for (const auto& field : shape.fields)
    {
        if (field.name == name)
        {
            return &field;
        }
    }
    return nullptr;
}

const IrEntity* create_entity_for_api_go(
    const IrSystem& system,
    const IrApi& api
)
{
    constexpr std::string_view prefix = "Create";
    if (api.method.value_or("") != "POST" || api.name.rfind(prefix, 0) != 0 || !api.input)
    {
        return nullptr;
    }
    return find_entity(system, api.name.substr(prefix.size()));
}

bool create_api_has_required_request_fields_go(
    const IrEntity& entity,
    const IrShape& request
)
{
    for (const auto& field : entity.fields)
    {
        if (field.name == "created_at" || field.name == "updated_at" || field.name == "status")
        {
            continue;
        }
        if (find_field(request, field.name) == nullptr)
        {
            return false;
        }
    }
    return true;
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

std::string go_create_response_expr(
    const IrField& field,
    const IrShape& request,
    const IrEntity& entity,
    const std::string& context_expr
)
{
    if (find_field(request, field.name) != nullptr)
    {
        return "decodedRequest." + pascal_identifier(field.name);
    }
    if (field.name == "status")
    {
        return go_string(entity.initial_state.value_or("Created"));
    }
    return go_default_response_expr(field, context_expr);
}

void write_go_parent_validation(
    std::ostringstream& out,
    const IrSystem& system,
    const IrEntity& entity,
    const IrShape& request
)
{
    for (const auto& relation : entity.relations)
    {
        if (relation.kind != "parent" || relation.optional)
        {
            continue;
        }
        const auto* parent = find_entity(system, relation.target);
        if (parent == nullptr)
        {
            continue;
        }
        out << "\t{\n";
        out << "\t\tparent := common.Default" << pascal_identifier(parent->name)
            << "Repository{}\n";
        out << "\t\tparentRecord, err := parent.GetTx(ctx, tx, []common.EntityKeyValue{\n";
        for (const auto& key_field : parent->key_fields)
        {
            const auto* request_field = find_field(request, key_field);
            out << "\t\t\t{Field: " << go_string(key_field) << ", Value: ";
            if (request_field == nullptr)
            {
                out << "common.JSONNull()";
            }
            else
            {
                out << go_encode_expr(
                    *request_field, "decodedRequest." + pascal_identifier(key_field)
                );
            }
            out << "},\n";
        }
        out << "\t\t})\n";
        out << "\t\tif err != nil { return common.APIResponse{}, err }\n";
        out << "\t\tif parentRecord == nil { return common.APIResponse{}, fmt.Errorf(\"missing "
               "required parent "
            << parent->name << "\") }\n";
        out << "\t}\n";
    }
}

bool write_go_create_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = create_entity_for_api_go(system, api);
    const auto* request = api.input ? find_shape(system, *api.input) : nullptr;
    if (entity == nullptr || request == nullptr)
    {
        return false;
    }
    if (!create_api_has_required_request_fields_go(*entity, *request))
    {
        out << "\treturn common.APIResponse{}, fmt.Errorf("
            << go_string(
                   "generated create handler for " + api.name +
                   " requires request fields for every non-foundational " + entity->name +
                   " entity field"
               )
            << ")\n";
        return true;
    }
    const auto status = entity->initial_state.value_or("Created");
    out << "\tdecodedRequest, err := Decode" << pascal_identifier(api.name) << "Request(request)\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\trepository := common.Default" << pascal_identifier(entity->name) << "Repository{}\n";
    out << "\tif err := repository.RegisterDescriptor(ctx, handler.Backend); err != nil { return "
           "common.APIResponse{}, err }\n";
    out << "\ttx, err := handler.Backend.Begin(ctx)\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tdefer func() { if tx.IsOpen() { _ = tx.Abort(ctx) } }()\n";
    write_go_parent_validation(out, system, *entity, *request);
    out << "\tdocument := map[string]common.JSON{}\n";
    for (const auto& field : entity->fields)
    {
        if (field.name == "created_at" || field.name == "updated_at")
        {
            out << "\tdocument[" << go_string(field.name)
                << "] = common.JSONString(time.Now().UTC().Format(time.RFC3339))\n";
        }
        else if (field.name == "status")
        {
            out << "\tdocument[" << go_string(field.name) << "] = common.JSONString("
                << go_string(status) << ")\n";
        }
        else if (const auto* request_field = find_field(*request, field.name);
                 request_field != nullptr)
        {
            out << "\tdocument[" << go_string(field.name) << "] = "
                << go_encode_expr(*request_field, "decodedRequest." + pascal_identifier(field.name))
                << "\n";
        }
    }
    out << "\trecord, err := repository.CreateTx(ctx, tx, common.JSONObject(document))\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tif record == nil { return common.APIResponse{}, fmt.Errorf(\"entity create failed\") "
           "}\n";
    out << "\tif err := handler.Backend.Commit(ctx, tx); err != nil { return common.APIResponse{}, "
           "err }\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            out << "\tresponse := common." << pascal_identifier(shape->name) << "{}\n";
            for (const auto& field : shape->fields)
            {
                const auto access = "response." + pascal_identifier(field.name);
                const auto value = go_create_response_expr(field, *request, *entity, "request");
                if (is_optional_type(field.type))
                {
                    out << "\t" << lower_camel_identifier(field.name) << " := " << value << "\n";
                    out << "\t" << access << " = &" << lower_camel_identifier(field.name) << "\n";
                }
                else
                {
                    out << "\t" << access << " = " << value << "\n";
                }
            }
            out << "\treturn Encode" << pascal_identifier(api.name)
                << "ResponseWithStatus(response, 201), nil\n";
        }
        else
        {
            out << "\treturn common.APIResponse{StatusCode: 201, Body: "
                   "common.JSONObject(map[string]common.JSON{})}, nil\n";
        }
    }
    else
    {
        out << "\treturn common.APIResponse{StatusCode: 201, Body: "
               "common.JSONObject(map[string]common.JSON{})}, nil\n";
    }
    return true;
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
    out << generate_go_runtime_registration(system, templates);
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
        out << "func (handler DefaultAPITierHandler) Handle" << pascal_identifier(api.name)
            << "(ctx context.Context, request common.APIRequestContext) (common.APIResponse, "
               "error) {\n";
        if (write_go_create_handler_body(out, system, api))
        {
            out << "}\n\n";
            continue;
        }
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
