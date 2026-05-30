#include "generator_go_descriptors.hpp"

#include "generator_entity_index_helpers.hpp"
#include "generator_go_descriptor_areas.hpp"
#include "generator_go_descriptor_support.hpp"
#include "identifier_case.hpp"
#include "statespec/language_constants.hpp"
#include "type_syntax.hpp"

#include <algorithm>
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

const IrField* find_entity_field_go(
    const IrEntity& entity,
    const std::string& name
)
{
    for (const auto& field : entity.fields)
    {
        if (field.name == name)
        {
            return &field;
        }
    }
    return nullptr;
}

std::string go_entity_field_expr(
    const IrEntity& entity,
    const std::string& field_name
)
{
    if (find_entity_field_go(entity, field_name) == nullptr)
    {
        return go_string(field_name);
    }
    return snake_identifier(entity.name) + "." +
           go_entity_field_constant_name(entity.name, field_name);
}

std::string go_entity_state_expr(
    const IrEntity& entity,
    const std::string& state_name
)
{
    return snake_identifier(entity.name) + "." +
           go_entity_state_constant_name(entity.name, state_name);
}

std::string go_api_body_field_expr(
    const IrEntity& entity,
    const std::string& field_name
)
{
    if (find_entity_field_go(entity, field_name) == nullptr)
    {
        return go_string(field_name);
    }
    return go_entity_field_expr(entity, field_name);
}

std::string go_entity_repository_expr(const IrEntity& entity)
{
    return snake_identifier(entity.name) + ".Default" + pascal_identifier(entity.name) +
           "Repository{}";
}

bool api_repository_operation_is_go(
    const IrApi& api,
    std::string_view operation
)
{
    return api.repository_operation.has_value() && *api.repository_operation == operation &&
           api.entity.has_value();
}

const IrEntity* create_entity_for_api_go(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api_repository_operation_is_go(api, "create") || !api.input)
    {
        return nullptr;
    }
    return find_entity(system, *api.entity);
}

const IrEntity* get_entity_for_api_go(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api_repository_operation_is_go(api, "get"))
    {
        return nullptr;
    }
    return find_entity(system, *api.entity);
}

bool ends_with_suffix_go(
    const std::string& value,
    std::string_view suffix
)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string list_item_type_go(const std::string& type)
{
    if (ends_with_suffix_go(type, "[]"))
    {
        return type.substr(0, type.size() - 2);
    }
    return {};
}

const IrField* list_response_field_go(const IrShape& shape)
{
    for (const auto& field : shape.fields)
    {
        if (!list_item_type_go(field.type).empty())
        {
            return &field;
        }
    }
    return nullptr;
}

const IrEntity* list_entity_for_api_go(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api_repository_operation_is_go(api, "list") || !api.output)
    {
        return nullptr;
    }
    const auto* output = find_shape(system, *api.output);
    if (output == nullptr)
    {
        return nullptr;
    }
    const auto* list_field = list_response_field_go(*output);
    if (list_field == nullptr)
    {
        return nullptr;
    }
    auto item_type = list_item_type_go(list_field->type);
    if (ends_with_suffix_go(item_type, "Response"))
    {
        item_type = item_type.substr(0, item_type.size() - std::string_view{"Response"}.size());
    }
    const auto* entity = find_entity(system, *api.entity);
    return entity != nullptr && entity->name == item_type ? entity : nullptr;
}

const IrEntity* update_status_entity_for_api_go(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api_repository_operation_is_go(api, "update_status") || !api.input)
    {
        return nullptr;
    }
    return find_entity(system, *api.entity);
}

const IrEntity* delete_entity_for_api_go(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api_repository_operation_is_go(api, "delete"))
    {
        return nullptr;
    }
    return find_entity(system, *api.entity);
}

const std::string* conventional_soft_delete_terminal_state_go(const IrEntity& entity)
{
    const auto found = std::find(
        entity.terminal_states.begin(), entity.terminal_states.end(),
        std::string{ConventionalSoftDeleteTerminalStateName}
    );
    return found == entity.terminal_states.end() ? nullptr : &*found;
}

bool entity_needs_status_transition_helper_go(
    const IrSystem& system,
    const IrEntity& entity
)
{
    return std::any_of(
        system.apis.begin(), system.apis.end(),
        [&](const IrApi& api)
        {
            return api.entity.has_value() && *api.entity == entity.name &&
                   api.repository_operation.has_value() &&
                   (*api.repository_operation == "update_status" ||
                    *api.repository_operation == "delete");
        }
    );
}

bool status_update_has_required_request_fields_go(const IrShape& request)
{
    return find_field(request, std::string{EntityStatusFieldName}) != nullptr;
}

bool create_api_has_required_request_fields_go(
    const IrEntity& entity,
    const IrShape& request,
    const IrApi& api
)
{
    for (const auto& field : entity.fields)
    {
        if (field.name == EntityCreatedAtFieldName || field.name == EntityUpdatedAtFieldName ||
            field.name == EntityStatusFieldName)
        {
            continue;
        }
        if (find_field(request, field.name) == nullptr &&
            api.path.value_or("").find("{" + field.name + "}") == std::string::npos)
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

std::string go_path_json_expr(
    const IrEntity& entity,
    const std::string& field_name
)
{
    return "pathParameterJSON(pathParameters, " + go_entity_field_expr(entity, field_name) + ")";
}

std::string go_request_or_path_json_expr(
    const IrEntity& entity,
    const IrShape& request,
    const std::string& field_name
)
{
    if (const auto* request_field = find_field(request, field_name); request_field != nullptr)
    {
        return go_encode_expr(*request_field, "decodedRequest." + pascal_identifier(field_name));
    }
    return go_path_json_expr(entity, field_name);
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
    if (field.name == EntityStatusFieldName)
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
    if (field.name == EntityStatusFieldName)
    {
        if (entity.initial_state.has_value())
        {
            return go_entity_state_expr(entity, *entity.initial_state);
        }
        return "\"\"";
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
        out << "\t\tparent := " << go_entity_repository_expr(*parent) << "\n";
        out << "\t\tparentRecord, err := parent.GetTx(ctx, tx, []common.EntityKeyValue{\n";
        for (const auto& key_field : parent->key_fields)
        {
            out << "\t\t\t{Field: " << go_entity_field_expr(*parent, key_field) << ", Value: ";
            out << go_request_or_path_json_expr(entity, request, key_field);
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
    if (!create_api_has_required_request_fields_go(*entity, *request, api))
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
    out << "\tpathParameters := extractAPIPathParameters(" << go_string(api.path.value_or(""))
        << ", request.Path)\n";
    out << "\t_ = pathParameters\n";
    out << "\trepository := " << go_entity_repository_expr(*entity) << "\n";
    out << "\tif err := repository.RegisterDescriptor(ctx, handler.Backend); err != nil { return "
           "common.APIResponse{}, err }\n";
    out << "\ttx, err := handler.Backend.Begin(ctx)\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tdefer func() { if tx.IsOpen() { _ = tx.Abort(ctx) } }()\n";
    write_go_parent_validation(out, system, *entity, *request);
    out << "\tcreateTimestamp := time.Now().UTC().Format(time.RFC3339)\n";
    out << "\tdocument := map[string]common.JSON{}\n";
    for (const auto& field : entity->fields)
    {
        if (field.name == EntityCreatedAtFieldName || field.name == EntityUpdatedAtFieldName)
        {
            out << "\tdocument[" << go_entity_field_expr(*entity, field.name)
                << "] = common.JSONString(createTimestamp)\n";
        }
        else if (field.name == EntityStatusFieldName)
        {
            out << "\tdocument[" << go_entity_field_expr(*entity, field.name)
                << "] = common.JSONString(" << go_entity_state_expr(*entity, status) << ")\n";
        }
        else if (const auto* request_field = find_field(*request, field.name);
                 request_field != nullptr)
        {
            out << "\tdocument[" << go_entity_field_expr(*entity, field.name) << "] = "
                << go_encode_expr(*request_field, "decodedRequest." + pascal_identifier(field.name))
                << "\n";
        }
        else if (api.path.value_or("").find("{" + field.name + "}") != std::string::npos)
        {
            out << "\tdocument[" << go_entity_field_expr(*entity, field.name)
                << "] = " << go_path_json_expr(*entity, field.name) << "\n";
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
            out << "\tresponse := shapes." << pascal_identifier(shape->name) << "{}\n";
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

bool write_go_get_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = get_entity_for_api_go(system, api);
    if (entity == nullptr)
    {
        return false;
    }
    out << "\tpathParameters := extractAPIPathParameters(" << go_string(api.path.value_or(""))
        << ", request.Path)\n";
    out << "\t_ = pathParameters\n";
    out << "\trepository := " << go_entity_repository_expr(*entity) << "\n";
    out << "\tif err := repository.RegisterDescriptor(ctx, handler.Backend); err != nil { return "
           "common.APIResponse{}, err }\n";
    out << "\ttx, err := handler.Backend.Begin(ctx)\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tdefer func() { if tx.IsOpen() { _ = tx.Abort(ctx) } }()\n";
    out << "\trecord, err := repository.GetTx(ctx, tx, []common.EntityKeyValue{\n";
    for (const auto& key_field : entity->key_fields)
    {
        out << "\t\t{Field: " << go_entity_field_expr(*entity, key_field)
            << ", Value: pathParameterJSON(pathParameters, "
            << go_entity_field_expr(*entity, key_field) << ")},\n";
    }
    out << "\t})\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tif err := handler.Backend.Commit(ctx, tx); err != nil { return common.APIResponse{}, "
           "err }\n";
    out << "\tif record == nil { return common.APIResponse{StatusCode: 404, Body: "
           "common.JSONObject(map[string]common.JSON{})}, nil }\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            out << "\tresponse := shapes." << pascal_identifier(shape->name) << "{}\n";
            for (const auto& field : shape->fields)
            {
                const auto var_name = lower_camel_identifier(field.name);
                out << "\t" << var_name << "JSON, ok := record.Document.Find("
                    << go_entity_field_expr(*entity, field.name) << ")\n";
                out << "\tif !ok { return common.APIResponse{}, fmt.Errorf(\"missing entity field "
                    << field.name << "\") }\n";
                out << "\t" << var_name << ", err := " << go_decode_func(field) << "(" << var_name
                    << "JSON, " << go_entity_field_expr(*entity, field.name) << ")\n";
                out << "\tif err != nil { return common.APIResponse{}, err }\n";
                out << "\tresponse." << pascal_identifier(field.name) << " = " << var_name << "\n";
            }
            out << "\treturn Encode" << pascal_identifier(api.name)
                << "ResponseWithStatus(response, 200), nil\n";
        }
        else
        {
            out << "\treturn common.APIResponse{StatusCode: 200, Body: record.Document}, nil\n";
        }
    }
    else
    {
        out << "\treturn common.APIResponse{StatusCode: 200, Body: record.Document}, nil\n";
    }
    return true;
}

bool write_go_list_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = list_entity_for_api_go(system, api);
    const auto* shape = api.output ? find_shape(system, *api.output) : nullptr;
    const auto* list_field = shape == nullptr ? nullptr : list_response_field_go(*shape);
    if (entity == nullptr || shape == nullptr || list_field == nullptr)
    {
        return false;
    }
    const auto* index = select_entity_list_index_for_selector(*entity, api.list_selector);
    if (index == nullptr)
    {
        out << "\treturn common.APIResponse{}, fmt.Errorf("
            << go_string(
                   "generated list handler for " + api.name +
                   " requires a key or index backed selector"
               )
            << ")\n";
        return true;
    }
    out << "\tpathParameters := extractAPIPathParameters(" << go_string(api.path.value_or(""))
        << ", request.Path)\n";
    out << "\t_ = pathParameters\n";
    out << "\trepository := " << go_entity_repository_expr(*entity) << "\n";
    out << "\tif err := repository.RegisterDescriptor(ctx, handler.Backend); err != nil { return "
           "common.APIResponse{}, err }\n";
    out << "\ttx, err := handler.Backend.Begin(ctx)\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tdefer func() { if tx.IsOpen() { _ = tx.Abort(ctx) } }()\n";
    out << "\trecords, err := repository.";
    out << go_entity_index_repository_method_name(index->name)
        << "(ctx, tx, []common.IndexValue{\n";
    for (const auto& field_name : api.list_selector)
    {
        out << "\t\tcommon.StringIndexValue(pathParameters["
            << go_entity_field_expr(*entity, field_name) << "]),\n";
    }
    out << "\t})\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tif err := handler.Backend.Commit(ctx, tx); err != nil { return common.APIResponse{}, "
           "err }\n";
    out << "\tbody := map[string]common.JSON{}\n";
    for (const auto& field : shape->fields)
    {
        if (&field == list_field)
        {
            out << "\titems := []common.JSON{}\n";
            out << "\tfor _, record := range records { items = append(items, record.Document) }\n";
            out << "\tbody[" << go_api_body_field_expr(*entity, field.name)
                << "] = common.JSONArray(items)\n";
        }
        else
        {
            out << "\tbody[" << go_api_body_field_expr(*entity, field.name)
                << "] = pathParameterJSON(pathParameters, "
                << go_entity_field_expr(*entity, field.name) << ")\n";
        }
    }
    out << "\treturn common.APIResponse{StatusCode: 200, Body: common.JSONObject(body)}, nil\n";
    return true;
}

bool write_go_update_status_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = update_status_entity_for_api_go(system, api);
    const auto* request = api.input ? find_shape(system, *api.input) : nullptr;
    if (entity == nullptr || request == nullptr)
    {
        return false;
    }
    if (!status_update_has_required_request_fields_go(*request))
    {
        out << "\treturn common.APIResponse{}, fmt.Errorf("
            << go_string(
                   "generated status update handler for " + api.name +
                   " requires status and entity key request fields"
               )
            << ")\n";
        return true;
    }
    out << "\tdecodedRequest, err := Decode" << pascal_identifier(api.name) << "Request(request)\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tpathParameters := extractAPIPathParameters(" << go_string(api.path.value_or(""))
        << ", request.Path)\n";
    out << "\t_ = pathParameters\n";
    out << "\trepository := " << go_entity_repository_expr(*entity) << "\n";
    out << "\tif err := repository.RegisterDescriptor(ctx, handler.Backend); err != nil { return "
           "common.APIResponse{}, err }\n";
    out << "\ttx, err := handler.Backend.Begin(ctx)\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tdefer func() { if tx.IsOpen() { _ = tx.Abort(ctx) } }()\n";
    out << "\tkeyValues := []common.EntityKeyValue{\n";
    for (const auto& key_field : entity->key_fields)
    {
        out << "\t\t{Field: " << go_entity_field_expr(*entity, key_field)
            << ", Value: " << go_request_or_path_json_expr(*entity, *request, key_field) << "},\n";
    }
    out << "\t}\n";
    out << "\trecord, err := repository.GetTx(ctx, tx, keyValues)\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tif record == nil {\n";
    out << "\t\tif err := handler.Backend.Commit(ctx, tx); err != nil { return "
           "common.APIResponse{}, err }\n";
    out << "\t\treturn common.APIResponse{StatusCode: 404, Body: "
           "common.JSONObject(map[string]common.JSON{})}, nil\n";
    out << "\t}\n";
    out << "\tstatusJSON, ok := record.Document.Find("
        << go_entity_field_expr(*entity, std::string{EntityStatusFieldName}) << ")\n";
    out << "\tif !ok { return common.APIResponse{}, fmt.Errorf(\"missing entity field status\") "
           "}\n";
    out << "\tcurrentStatus, err := decodeString(statusJSON, "
        << go_entity_field_expr(*entity, std::string{EntityStatusFieldName}) << ")\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\trequestedStatus := decodedRequest.Status\n";
    out << "\tif !" << lower_camel_identifier(entity->name)
        << "StatusTransitionAllowed(currentStatus, requestedStatus) { return "
           "common.APIResponse{}, fmt.Errorf(\"invalid entity "
           "status transition\") }\n";
    out << "\tdocument, ok := record.Document.AsObject()\n";
    out << "\tif !ok { return common.APIResponse{}, fmt.Errorf(\"entity document must be an "
           "object\") }\n";
    out << "\tupdatedDocument := map[string]common.JSON{}\n";
    out << "\tfor key, value := range document { updatedDocument[key] = value }\n";
    out << "\tupdatedDocument[" << go_entity_field_expr(*entity, std::string{EntityStatusFieldName})
        << "] = common.JSONString(requestedStatus)\n";
    out << "\tupdatedDocument["
        << go_entity_field_expr(*entity, std::string{EntityUpdatedAtFieldName})
        << "] = "
           "common.JSONString(time.Now().UTC().Format(time.RFC3339))\n";
    out << "\tupdated, err := repository.UpdateTx(ctx, tx, keyValues, "
           "common.JSONObject(updatedDocument), record.Version)\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tif updated == nil { return common.APIResponse{}, fmt.Errorf(\"entity update "
           "failed\") }\n";
    out << "\tif err := handler.Backend.Commit(ctx, tx); err != nil { return common.APIResponse{}, "
           "err }\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            out << "\tresponse := shapes." << pascal_identifier(shape->name) << "{}\n";
            for (const auto& field : shape->fields)
            {
                const auto var_name = "response" + pascal_identifier(field.name);
                out << "\t" << var_name << "JSON, ok := updated.Document.Find("
                    << go_entity_field_expr(*entity, field.name) << ")\n";
                out << "\tif !ok { return common.APIResponse{}, fmt.Errorf(\"missing entity field "
                    << field.name << "\") }\n";
                out << "\t" << var_name << ", err := " << go_decode_func(field) << "(" << var_name
                    << "JSON, " << go_entity_field_expr(*entity, field.name) << ")\n";
                out << "\tif err != nil { return common.APIResponse{}, err }\n";
                out << "\tresponse." << pascal_identifier(field.name) << " = " << var_name << "\n";
            }
            out << "\treturn Encode" << pascal_identifier(api.name)
                << "ResponseWithStatus(response, 200), nil\n";
        }
        else
        {
            out << "\treturn common.APIResponse{StatusCode: 200, Body: updated.Document}, nil\n";
        }
    }
    else
    {
        out << "\treturn common.APIResponse{StatusCode: 200, Body: updated.Document}, nil\n";
    }
    return true;
}

bool write_go_delete_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = delete_entity_for_api_go(system, api);
    const auto* delete_state =
        entity == nullptr ? nullptr : conventional_soft_delete_terminal_state_go(*entity);
    if (entity == nullptr || delete_state == nullptr)
    {
        return false;
    }
    out << "\tpathParameters := extractAPIPathParameters(" << go_string(api.path.value_or(""))
        << ", request.Path)\n";
    out << "\t_ = pathParameters\n";
    out << "\trepository := " << go_entity_repository_expr(*entity) << "\n";
    out << "\tif err := repository.RegisterDescriptor(ctx, handler.Backend); err != nil { return "
           "common.APIResponse{}, err }\n";
    out << "\ttx, err := handler.Backend.Begin(ctx)\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tdefer func() { if tx.IsOpen() { _ = tx.Abort(ctx) } }()\n";
    out << "\tkeyValues := []common.EntityKeyValue{\n";
    for (const auto& key_field : entity->key_fields)
    {
        out << "\t\t{Field: " << go_entity_field_expr(*entity, key_field)
            << ", Value: pathParameterJSON(pathParameters, "
            << go_entity_field_expr(*entity, key_field) << ")},\n";
    }
    out << "\t}\n";
    out << "\trecord, err := repository.GetTx(ctx, tx, keyValues)\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tif record == nil {\n";
    out << "\t\tif err := handler.Backend.Commit(ctx, tx); err != nil { return "
           "common.APIResponse{}, err }\n";
    out << "\t\treturn common.APIResponse{StatusCode: 404, Body: "
           "common.JSONObject(map[string]common.JSON{})}, nil\n";
    out << "\t}\n";
    out << "\tstatusJSON, ok := record.Document.Find("
        << go_entity_field_expr(*entity, std::string{EntityStatusFieldName}) << ")\n";
    out << "\tif !ok { return common.APIResponse{}, fmt.Errorf(\"missing entity field status\") "
           "}\n";
    out << "\tcurrentStatus, err := decodeString(statusJSON, "
        << go_entity_field_expr(*entity, std::string{EntityStatusFieldName}) << ")\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\trequestedStatus := " << go_entity_state_expr(*entity, *delete_state) << "\n";
    out << "\tif !" << lower_camel_identifier(entity->name)
        << "StatusTransitionAllowed(currentStatus, requestedStatus) { return "
           "common.APIResponse{}, fmt.Errorf(\"invalid entity "
           "delete transition\") }\n";
    out << "\tdocument, ok := record.Document.AsObject()\n";
    out << "\tif !ok { return common.APIResponse{}, fmt.Errorf(\"entity document must be an "
           "object\") }\n";
    out << "\tupdatedDocument := map[string]common.JSON{}\n";
    out << "\tfor key, value := range document { updatedDocument[key] = value }\n";
    out << "\tupdatedDocument[" << go_entity_field_expr(*entity, std::string{EntityStatusFieldName})
        << "] = common.JSONString(requestedStatus)\n";
    out << "\tupdatedDocument["
        << go_entity_field_expr(*entity, std::string{EntityUpdatedAtFieldName})
        << "] = "
           "common.JSONString(time.Now().UTC().Format(time.RFC3339))\n";
    out << "\tupdated, err := repository.UpdateTx(ctx, tx, keyValues, "
           "common.JSONObject(updatedDocument), record.Version)\n";
    out << "\tif err != nil { return common.APIResponse{}, err }\n";
    out << "\tif updated == nil { return common.APIResponse{}, fmt.Errorf(\"entity delete update "
           "failed\") }\n";
    out << "\tif err := handler.Backend.Commit(ctx, tx); err != nil { return common.APIResponse{}, "
           "err }\n";
    out << "\treturn common.APIResponse{StatusCode: 204, Body: "
           "common.JSONObject(map[string]common.JSON{})}, nil\n";
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
        templates.load("generated/external_system_metadata_runtime.go.tmpl"), {}
    );
    return out.str();
}

std::string generate_workflow_step_handler_keys_go(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "\t\tWorkflowStepKeyString(" << go_string(workflow.name) << ", "
                << workflow.version.value_or(1) << ", " << go_string(step.name) << "),\n";
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

std::string generate_default_workflow_step_handler_methods_go(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "func (DefaultWorkflowStepHandler) Handle" << pascal_identifier(step.name)
                << "(context.Context, WorkflowStepHandlerContext) error {\n";
            out << "\treturn fmt.Errorf(\"generated workflow step handler " << workflow.name << "."
                << step.name << " is not implemented\")\n";
            out << "}\n\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_handler_imports_go(const IrSystem& system)
{
    if (system.workflows.empty())
    {
        return {};
    }
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        out << "\tworkflow" << snake_identifier(workflow.name)
            << " \"statespec-generated/worker/backend/workflows/" << snake_identifier(workflow.name)
            << "\"\n";
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
            << "(context.Context, descriptortypes.APIRequestContext) "
               "(descriptortypes.APIResponse, error)\n";
    }
    return out.str();
}

std::string generate_business_api_operation_handler_methods_go(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        if (api.entity.has_value() && api.repository_operation.has_value())
        {
            continue;
        }
        out << "\tHandle" << pascal_identifier(api.name)
            << "(context.Context, descriptortypes.APIRequestContext) "
               "(descriptortypes.APIResponse, error)\n";
    }
    return out.str();
}

std::string generate_api_handler_lookup_entries_go(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "\t" << go_string(api.name) << ": func(ctx context.Context, handler APITierHandler, "
            << "request descriptortypes.APIRequestContext) (descriptortypes.APIResponse, error) "
               "{\n";
        out << "\t\treturn handler.Handle" << pascal_identifier(api.name) << "(ctx, request)\n";
        out << "\t},\n";
    }
    return out.str();
}

std::string generate_api_operation_default_handler_methods_go_for_receiver(
    const IrSystem& system,
    std::string_view receiver_type
)
{
    std::ostringstream out;
    for (const auto& entity : system.entities)
    {
        if (!entity_needs_status_transition_helper_go(system, entity))
        {
            continue;
        }
        out << "func " << lower_camel_identifier(entity.name)
            << "StatusTransitionAllowed(currentStatus string, requestedStatus string) bool {\n";
        out << "\treturn currentStatus == requestedStatus";
        for (const auto& transition : entity.transitions)
        {
            out << " ||\n";
            out << "\t\t(currentStatus == " << go_entity_state_expr(entity, transition.from)
                << " && requestedStatus == " << go_entity_state_expr(entity, transition.to) << ")";
        }
        out << "\n";
        out << "}\n\n";
    }
    for (const auto& api : system.apis)
    {
        out << "func (handler " << receiver_type << ") Handle" << pascal_identifier(api.name)
            << "(ctx context.Context, request common.APIRequestContext) (common.APIResponse, "
               "error) {\n";
        if (write_go_create_handler_body(out, system, api))
        {
            out << "}\n\n";
            continue;
        }
        if (write_go_get_handler_body(out, system, api))
        {
            out << "}\n\n";
            continue;
        }
        if (write_go_list_handler_body(out, system, api))
        {
            out << "}\n\n";
            continue;
        }
        if (write_go_update_status_handler_body(out, system, api))
        {
            out << "}\n\n";
            continue;
        }
        if (write_go_delete_handler_body(out, system, api))
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
                out << "\tresponse := shapes." << pascal_identifier(shape->name) << "{}\n";
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

std::string generate_api_operation_default_handler_methods_go(const IrSystem& system)
{
    return generate_api_operation_default_handler_methods_go_for_receiver(
        system, "DefaultAPITierHandler"
    );
}

std::string generate_api_codec_helpers_go()
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
    return out.str();
}

std::string generate_api_codec_operations_go(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        const auto* entity = api.entity.has_value() ? find_entity(system, *api.entity) : nullptr;
        if (api.input.has_value())
        {
            const auto* shape = find_shape(system, *api.input);
            if (shape != nullptr)
            {
                out << "func Decode" << pascal_identifier(api.name)
                    << "Request(request common.APIRequestContext) (shapes."
                    << pascal_identifier(shape->name) << ", error) {\n";
                out << "\tvar decoded shapes." << pascal_identifier(shape->name) << "\n";
                for (const auto& field : shape->fields)
                {
                    const auto field_name = entity != nullptr
                                                ? go_api_codec_field_name_expr(*entity, field.name)
                                                : go_string(field.name);
                    const auto field_var = lower_camel_identifier(field.name);
                    const auto field_access = pascal_identifier(field.name);
                    if (is_optional_type(field.type))
                    {
                        out << "\tif member, ok := request.Body.Find(" << field_name
                            << "); ok && !member.IsNull() {\n";
                        out << "\t\t" << field_var << ", err := " << go_decode_func(field)
                            << "(member, " << field_name << ")\n";
                        out << "\t\tif err != nil { return shapes."
                            << pascal_identifier(shape->name) << "{}, err }\n";
                        out << "\t\tdecoded." << field_access << " = &" << field_var << "\n";
                        out << "\t}\n";
                    }
                    else
                    {
                        out << "\t" << field_var << "JSON, err := requireMember(request.Body, "
                            << field_name << ")\n";
                        out << "\tif err != nil { return shapes." << pascal_identifier(shape->name)
                            << "{}, err }\n";
                        out << "\t" << field_var << ", err := " << go_decode_func(field) << "("
                            << field_var << "JSON, " << field_name << ")\n";
                        out << "\tif err != nil { return shapes." << pascal_identifier(shape->name)
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
                    << "Response(response common.APIResponse) (shapes."
                    << pascal_identifier(shape->name) << ", error) {\n";
                out << "\trequest := common.APIRequestContext{Body: response.Body}\n";
                out << "\treturn Decode" << pascal_identifier(api.name)
                    << "ResponseBody(request)\n";
                out << "}\n\n";

                out << "func Decode" << pascal_identifier(api.name)
                    << "ResponseBody(request common.APIRequestContext) (shapes."
                    << pascal_identifier(shape->name) << ", error) {\n";
                out << "\tvar decoded shapes." << pascal_identifier(shape->name) << "\n";
                for (const auto& field : shape->fields)
                {
                    const auto field_name = entity != nullptr
                                                ? go_api_codec_field_name_expr(*entity, field.name)
                                                : go_string(field.name);
                    const auto field_var = lower_camel_identifier(field.name);
                    const auto field_access = pascal_identifier(field.name);
                    if (is_optional_type(field.type))
                    {
                        out << "\tif member, ok := request.Body.Find(" << field_name
                            << "); ok && !member.IsNull() {\n";
                        out << "\t\t" << field_var << ", err := " << go_decode_func(field)
                            << "(member, " << field_name << ")\n";
                        out << "\t\tif err != nil { return shapes."
                            << pascal_identifier(shape->name) << "{}, err }\n";
                        out << "\t\tdecoded." << field_access << " = &" << field_var << "\n";
                        out << "\t}\n";
                    }
                    else
                    {
                        out << "\t" << field_var << "JSON, err := requireMember(request.Body, "
                            << field_name << ")\n";
                        out << "\tif err != nil { return shapes." << pascal_identifier(shape->name)
                            << "{}, err }\n";
                        out << "\t" << field_var << ", err := " << go_decode_func(field) << "("
                            << field_var << "JSON, " << field_name << ")\n";
                        out << "\tif err != nil { return shapes." << pascal_identifier(shape->name)
                            << "{}, err }\n";
                        out << "\tdecoded." << field_access << " = " << field_var << "\n";
                    }
                }
                out << "\treturn decoded, nil\n";
                out << "}\n\n";

                out << "func Encode" << pascal_identifier(api.name) << "Response(response shapes."
                    << pascal_identifier(shape->name) << ") common.APIResponse {\n";
                out << "\treturn Encode" << pascal_identifier(api.name)
                    << "ResponseWithStatus(response, 200)\n";
                out << "}\n\n";
                out << "func Encode" << pascal_identifier(api.name)
                    << "ResponseWithStatus(response shapes." << pascal_identifier(shape->name)
                    << ", statusCode int) common.APIResponse {\n";
                out << "\tbody := map[string]common.JSON{}\n";
                for (const auto& field : shape->fields)
                {
                    const auto field_name = entity != nullptr
                                                ? go_api_codec_field_name_expr(*entity, field.name)
                                                : go_string(field.name);
                    const auto access = "response." + pascal_identifier(field.name);
                    if (is_optional_type(field.type))
                    {
                        out << "\tif " << access << " != nil {\n";
                        out << "\t\tbody[" << field_name
                            << "] = " << go_encode_expr(field, "*" + access) << "\n";
                        out << "\t}\n";
                    }
                    else
                    {
                        out << "\tbody[" << field_name << "] = " << go_encode_expr(field, access)
                            << "\n";
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

std::string generate_api_codecs_go(const IrSystem& system)
{
    return generate_api_codec_helpers_go() + generate_api_codec_operations_go(system);
}

} // namespace statespec
