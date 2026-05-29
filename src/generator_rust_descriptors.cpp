#include "generator_rust_descriptors.hpp"

#include "generator_entity_index_helpers.hpp"
#include "generator_rust_descriptor_areas.hpp"
#include "generator_rust_descriptor_support.hpp"
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

const IrField* find_entity_field_rs(
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

std::string rust_entity_field_expr(
    const IrEntity& entity,
    const std::string& field_name
)
{
    if (find_entity_field_rs(entity, field_name) == nullptr)
    {
        return rust_string(field_name);
    }
    return "crate::entity_" + snake_identifier(entity.name) +
           "::constants::" + rust_entity_field_constant_name(entity.name, field_name);
}

std::string rust_entity_state_expr(
    const IrEntity& entity,
    const std::string& state_name
)
{
    return "crate::entity_" + snake_identifier(entity.name) +
           "::constants::" + rust_entity_state_constant_name(entity.name, state_name);
}

std::string rust_api_body_field_expr(
    const IrEntity& entity,
    const std::string& field_name
)
{
    if (find_entity_field_rs(entity, field_name) == nullptr)
    {
        return rust_string(field_name);
    }
    return rust_entity_field_expr(entity, field_name);
}

std::string rust_entity_repository_type(const IrEntity& entity)
{
    return "crate::entity_" + snake_identifier(entity.name) + "::persistence::Default" +
           pascal_identifier(entity.name) + "Repository";
}

std::string rust_entity_repository_trait(const IrEntity& entity)
{
    return "crate::entity_" + snake_identifier(entity.name) +
           "::persistence::" + pascal_identifier(entity.name) + "Repository";
}

std::string rust_entity_field_string_expr(
    const IrEntity& entity,
    const std::string& field_name
)
{
    return rust_entity_field_expr(entity, field_name) + ".to_string()";
}

const IrEntity* create_entity_for_api_rs(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api.repository_operation.has_value() || *api.repository_operation != "create" ||
        !api.entity.has_value() || !api.input)
    {
        return nullptr;
    }
    return find_entity(system, *api.entity);
}

const IrEntity* get_entity_for_api_rs(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api.repository_operation.has_value() || *api.repository_operation != "get" ||
        !api.entity.has_value())
    {
        return nullptr;
    }
    return find_entity(system, *api.entity);
}

bool ends_with_suffix_rs(
    const std::string& value,
    std::string_view suffix
)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string list_item_type_rs(const std::string& type)
{
    if (ends_with_suffix_rs(type, "[]"))
    {
        return type.substr(0, type.size() - 2);
    }
    return {};
}

const IrField* list_response_field_rs(const IrShape& shape)
{
    for (const auto& field : shape.fields)
    {
        if (!list_item_type_rs(field.type).empty())
        {
            return &field;
        }
    }
    return nullptr;
}

const IrEntity* list_entity_for_api_rs(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api.repository_operation.has_value() || *api.repository_operation != "list" ||
        !api.entity.has_value() || !api.output)
    {
        return nullptr;
    }
    const auto* output = find_shape(system, *api.output);
    if (output == nullptr)
    {
        return nullptr;
    }
    const auto* list_field = list_response_field_rs(*output);
    if (list_field == nullptr)
    {
        return nullptr;
    }
    auto item_type = list_item_type_rs(list_field->type);
    if (ends_with_suffix_rs(item_type, "Response"))
    {
        item_type = item_type.substr(0, item_type.size() - std::string_view{"Response"}.size());
    }
    const auto* entity = find_entity(system, *api.entity);
    return entity != nullptr && entity->name == item_type ? entity : nullptr;
}

const IrEntity* update_status_entity_for_api_rs(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api.repository_operation.has_value() || *api.repository_operation != "update_status" ||
        !api.entity.has_value() || !api.input)
    {
        return nullptr;
    }
    return find_entity(system, *api.entity);
}

const IrEntity* delete_entity_for_api_rs(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api.repository_operation.has_value() || *api.repository_operation != "delete" ||
        !api.entity.has_value())
    {
        return nullptr;
    }
    return find_entity(system, *api.entity);
}

const std::string* conventional_soft_delete_terminal_state_rs(const IrEntity& entity)
{
    const auto found = std::find(
        entity.terminal_states.begin(), entity.terminal_states.end(),
        std::string{ConventionalSoftDeleteTerminalStateName}
    );
    return found == entity.terminal_states.end() ? nullptr : &*found;
}

bool status_update_has_required_request_fields_rs(const IrShape& request)
{
    return find_field(request, std::string{EntityStatusFieldName}) != nullptr;
}

bool create_api_has_required_request_fields_rs(
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

std::string rust_path_json_expr(
    const IrEntity& entity,
    const std::string& field_name
)
{
    return "path_parameter_json(&path_parameters, " + rust_entity_field_expr(entity, field_name) +
           ")";
}

std::string rust_request_or_path_json_expr(
    const IrEntity& entity,
    const IrShape& request,
    const std::string& field_name
)
{
    if (const auto* request_field = find_field(request, field_name); request_field != nullptr)
    {
        return rust_encode_expr(*request_field, "request." + field_name);
    }
    return rust_path_json_expr(entity, field_name);
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
    if (field.name == EntityStatusFieldName)
    {
        return "\"Accepted\".to_string()";
    }
    return "String::new()";
}

std::string rust_create_response_expr(
    const IrField& field,
    const IrShape& request,
    const IrEntity& entity,
    const std::string& context_expr
)
{
    if (find_field(request, field.name) != nullptr)
    {
        return "request." + field.name + ".clone()";
    }
    if (field.name == EntityStatusFieldName)
    {
        if (entity.initial_state.has_value())
        {
            return rust_entity_state_expr(entity, *entity.initial_state) + ".to_string()";
        }
        return "String::new()";
    }
    return rust_default_response_expr(field, context_expr);
}

void write_rust_parent_validation(
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
        out << "        {\n";
        out << "            let parent = " << rust_entity_repository_type(*parent) << ";\n";
        out << "            let parent_record = <" << rust_entity_repository_type(*parent) << " as "
            << rust_entity_repository_trait(*parent) << "<B>>::get_tx(\n";
        out << "                &parent,\n";
        out << "                &mut tx,\n";
        out << "                vec![\n";
        for (const auto& key_field : parent->key_fields)
        {
            out << "                    EntityKeyValue { field: "
                << rust_entity_field_string_expr(*parent, key_field) << ", value: ";
            out << rust_request_or_path_json_expr(entity, request, key_field);
            out << " },\n";
        }
        out << "                ],\n";
        out << "            )?;\n";
        out << "            if parent_record.is_none() {\n";
        out << "                return Err(BackendError::NotFound { message: "
            << rust_string("missing required parent " + parent->name) << ".to_string() });\n";
        out << "            }\n";
        out << "        }\n";
    }
}

bool write_rust_create_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = create_entity_for_api_rs(system, api);
    const auto* request = api.input ? find_shape(system, *api.input) : nullptr;
    if (entity == nullptr || request == nullptr)
    {
        return false;
    }
    if (!create_api_has_required_request_fields_rs(*entity, *request, api))
    {
        out << "        return Err(BackendError::InvalidSchema { message: "
            << rust_string(
                   "generated create handler for " + api.name +
                   " requires request fields for every non-foundational " + entity->name +
                   " entity field"
               )
            << ".to_string() });\n";
        return true;
    }
    const auto status = entity->initial_state.value_or("Created");
    out << "        let request = crate::api_codecs::decode_" << snake_identifier(api.name)
        << "_request(context)?;\n";
    out << "        let path_parameters = extract_api_path_parameters("
        << rust_string(api.path.value_or("")) << ", &context.path);\n";
    out << "        let repository = " << rust_entity_repository_type(*entity) << ";\n";
    out << "        <" << rust_entity_repository_type(*entity) << " as "
        << rust_entity_repository_trait(*entity)
        << "<B>>::register_descriptor(&repository, &self.backend)?;\n";
    out << "        let mut tx = self.backend.begin()?;\n";
    write_rust_parent_validation(out, system, *entity, *request);
    out << "        let create_timestamp = generated_api_timestamp();\n";
    out << "        let mut document = std::collections::BTreeMap::new();\n";
    for (const auto& field : entity->fields)
    {
        if (field.name == EntityCreatedAtFieldName || field.name == EntityUpdatedAtFieldName)
        {
            out << "        document.insert(" << rust_entity_field_string_expr(*entity, field.name)
                << ", Json::String(create_timestamp.clone()));\n";
        }
        else if (field.name == EntityStatusFieldName)
        {
            out << "        document.insert(" << rust_entity_field_string_expr(*entity, field.name)
                << ", Json::String(" << rust_entity_state_expr(*entity, status)
                << ".to_string()));\n";
        }
        else if (const auto* request_field = find_field(*request, field.name);
                 request_field != nullptr)
        {
            out << "        document.insert(" << rust_entity_field_string_expr(*entity, field.name)
                << ", " << rust_encode_expr(*request_field, "request." + field.name) << ");\n";
        }
        else if (api.path.value_or("").find("{" + field.name + "}") != std::string::npos)
        {
            out << "        document.insert(" << rust_entity_field_string_expr(*entity, field.name)
                << ", " << rust_path_json_expr(*entity, field.name) << ");\n";
        }
    }
    out << "        let record = <" << rust_entity_repository_type(*entity) << " as "
        << rust_entity_repository_trait(*entity) << "<B>>::create_tx(\n";
    out << "            &repository,\n";
    out << "            &mut tx,\n";
    out << "            Json::Object(document),\n";
    out << "        )?;\n";
    out << "        if record.is_none() {\n";
    out << "            return Err(BackendError::Internal { message: \"entity create "
           "failed\".to_string() });\n";
    out << "        }\n";
    out << "        self.backend.commit(tx)?;\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            out << "        let response = shapes::" << pascal_identifier(shape->name) << " {\n";
            for (const auto& field : shape->fields)
            {
                out << "            " << field.name << ": ";
                if (is_optional_type(field.type))
                {
                    out << "Some(" << rust_create_response_expr(field, *request, *entity, "context")
                        << ")";
                }
                else
                {
                    out << rust_create_response_expr(field, *request, *entity, "context");
                }
                out << ",\n";
            }
            out << "        };\n";
            out << "        Ok(crate::api_codecs::encode_" << snake_identifier(api.name)
                << "_response_with_status(&response, 201))\n";
        }
        else
        {
            out << "        Ok(ApiResponse { status_code: 201, body: "
                   "Json::Object(std::collections::BTreeMap::new()) })\n";
        }
    }
    else
    {
        out << "        Ok(ApiResponse { status_code: 201, body: "
               "Json::Object(std::collections::BTreeMap::new()) })\n";
    }
    return true;
}

bool write_rust_get_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = get_entity_for_api_rs(system, api);
    if (entity == nullptr)
    {
        return false;
    }
    out << "        let path_parameters = extract_api_path_parameters("
        << rust_string(api.path.value_or("")) << ", &context.path);\n";
    out << "        let repository = " << rust_entity_repository_type(*entity) << ";\n";
    out << "        <" << rust_entity_repository_type(*entity) << " as "
        << rust_entity_repository_trait(*entity)
        << "<B>>::register_descriptor(&repository, &self.backend)?;\n";
    out << "        let mut tx = self.backend.begin()?;\n";
    out << "        let record = <" << rust_entity_repository_type(*entity) << " as "
        << rust_entity_repository_trait(*entity) << "<B>>::get_tx(\n";
    out << "            &repository,\n";
    out << "            &mut tx,\n";
    out << "            vec![\n";
    for (const auto& key_field : entity->key_fields)
    {
        out << "                EntityKeyValue { field: "
            << rust_entity_field_string_expr(*entity, key_field)
            << ", value: path_parameter_json(&path_parameters, "
            << rust_entity_field_expr(*entity, key_field) << ") },\n";
    }
    out << "            ],\n";
    out << "        )?;\n";
    out << "        self.backend.commit(tx)?;\n";
    out << "        let Some(record) = record else {\n";
    out << "            return Ok(ApiResponse { status_code: 404, body: "
           "Json::Object(std::collections::BTreeMap::new()) });\n";
    out << "        };\n";
    out << "        let mut body = std::collections::BTreeMap::new();\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            for (const auto& field : shape->fields)
            {
                out << "        if let Json::Object(document) = &record.document {\n";
                out << "            if let Some(value) = document.get("
                    << rust_entity_field_expr(*entity, field.name) << ") {\n";
                out << "                body.insert("
                    << rust_api_body_field_expr(*entity, field.name)
                    << ".to_string(), value.clone());\n";
                out << "            }\n";
                out << "        }\n";
            }
            out << "        Ok(ApiResponse { status_code: 200, body: Json::Object(body) })\n";
        }
        else
        {
            out << "        Ok(ApiResponse { status_code: 200, body: record.document })\n";
        }
    }
    else
    {
        out << "        Ok(ApiResponse { status_code: 200, body: record.document })\n";
    }
    return true;
}

bool write_rust_list_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = list_entity_for_api_rs(system, api);
    const auto* shape = api.output ? find_shape(system, *api.output) : nullptr;
    const auto* list_field = shape == nullptr ? nullptr : list_response_field_rs(*shape);
    if (entity == nullptr || shape == nullptr || list_field == nullptr)
    {
        return false;
    }
    const auto* index = select_entity_list_index_for_selector(*entity, api.list_selector);
    if (index == nullptr)
    {
        out << "        return Err(BackendError::InvalidSchema { message: "
            << rust_string(
                   "generated list handler for " + api.name +
                   " requires a key or index backed selector"
               )
            << ".to_string() });\n";
        return true;
    }
    out << "        let path_parameters = extract_api_path_parameters("
        << rust_string(api.path.value_or("")) << ", &context.path);\n";
    out << "        let repository = " << rust_entity_repository_type(*entity) << ";\n";
    out << "        <" << rust_entity_repository_type(*entity) << " as "
        << rust_entity_repository_trait(*entity)
        << "<B>>::register_descriptor(&repository, &self.backend)?;\n";
    out << "        let mut tx = self.backend.begin()?;\n";
    out << "        let records = <" << rust_entity_repository_type(*entity) << " as "
        << rust_entity_repository_trait(*entity) << "<B>>::";
    out << rust_entity_index_repository_method_name(index->name) << "(\n";
    out << "            &repository,\n";
    out << "            &mut tx,\n";
    out << "            vec![\n";
    for (const auto& field_name : api.list_selector)
    {
        out << "                IndexValue::String(path_parameters.get("
            << rust_entity_field_expr(*entity, field_name)
            << ").cloned().unwrap_or_else(String::new)),\n";
    }
    out << "            ],\n";
    out << "        )?;\n";
    out << "        self.backend.commit(tx)?;\n";
    out << "        let mut body = std::collections::BTreeMap::new();\n";
    for (const auto& field : shape->fields)
    {
        if (&field == list_field)
        {
            out << "        body.insert(\n";
            out << "            " << rust_api_body_field_expr(*entity, field.name)
                << ".to_string(),\n";
            out << "            Json::Array(records.into_iter().map(|record| "
                   "record.document).collect()),\n";
            out << "        );\n";
        }
        else
        {
            out << "        body.insert(" << rust_api_body_field_expr(*entity, field.name)
                << ".to_string(), path_parameter_json(&path_parameters, "
                << rust_entity_field_expr(*entity, field.name) << "));\n";
        }
    }
    out << "        Ok(ApiResponse { status_code: 200, body: Json::Object(body) })\n";
    return true;
}

bool write_rust_update_status_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = update_status_entity_for_api_rs(system, api);
    const auto* request = api.input ? find_shape(system, *api.input) : nullptr;
    if (entity == nullptr || request == nullptr)
    {
        return false;
    }
    if (!status_update_has_required_request_fields_rs(*request))
    {
        out << "        return Err(BackendError::InvalidSchema { message: "
            << rust_string(
                   "generated status update handler for " + api.name +
                   " requires status and entity key request fields"
               )
            << ".to_string() });\n";
        return true;
    }
    out << "        let request = crate::api_codecs::decode_" << snake_identifier(api.name)
        << "_request(context)?;\n";
    out << "        let path_parameters = extract_api_path_parameters("
        << rust_string(api.path.value_or("")) << ", &context.path);\n";
    out << "        let repository = " << rust_entity_repository_type(*entity) << ";\n";
    out << "        <" << rust_entity_repository_type(*entity) << " as "
        << rust_entity_repository_trait(*entity)
        << "<B>>::register_descriptor(&repository, &self.backend)?;\n";
    out << "        let mut tx = self.backend.begin()?;\n";
    out << "        let key_values = vec![\n";
    for (const auto& key_field : entity->key_fields)
    {
        out << "            EntityKeyValue { field: "
            << rust_entity_field_string_expr(*entity, key_field)
            << ", value: " << rust_request_or_path_json_expr(*entity, *request, key_field)
            << " },\n";
    }
    out << "        ];\n";
    out << "        let record = <" << rust_entity_repository_type(*entity) << " as "
        << rust_entity_repository_trait(*entity) << "<B>>::get_tx(\n";
    out << "            &repository,\n";
    out << "            &mut tx,\n";
    out << "            key_values.clone(),\n";
    out << "        )?;\n";
    out << "        let Some(record) = record else {\n";
    out << "            self.backend.commit(tx)?;\n";
    out << "            return Ok(ApiResponse { status_code: 404, body: "
           "Json::Object(std::collections::BTreeMap::new()) });\n";
    out << "        };\n";
    out << "        let Json::Object(mut document) = record.document.clone() else {\n";
    out << "            return Err(BackendError::InvalidSchema { message: \"entity document must "
           "be "
           "an object\".to_string() });\n";
    out << "        };\n";
    out << "        let current_status = match document.get("
        << rust_entity_field_expr(*entity, std::string{EntityStatusFieldName}) << ") {\n";
    out << "            Some(Json::String(value)) => value.clone(),\n";
    out << "            _ => return Err(BackendError::InvalidSchema { message: \"missing entity "
           "field "
           "status\".to_string() }),\n";
    out << "        };\n";
    out << "        let requested_status = request.status.clone();\n";
    out << "        let transition_allowed = current_status == requested_status";
    for (const auto& transition : entity->transitions)
    {
        out << " ||\n";
        out << "            (current_status == " << rust_entity_state_expr(*entity, transition.from)
            << " && requested_status == " << rust_entity_state_expr(*entity, transition.to) << ")";
    }
    out << ";\n";
    out << "        if !transition_allowed {\n";
    out << "            return Err(BackendError::InvalidSchema { message: \"invalid entity status "
           "transition\".to_string() });\n";
    out << "        }\n";
    out << "        document.insert("
        << rust_entity_field_string_expr(*entity, std::string{EntityStatusFieldName})
        << ", Json::String(requested_status));\n";
    out << "        document.insert("
        << rust_entity_field_string_expr(*entity, std::string{EntityUpdatedAtFieldName})
        << ", "
           "Json::String(generated_api_timestamp()));\n";
    out << "        let updated = <" << rust_entity_repository_type(*entity) << " as "
        << rust_entity_repository_trait(*entity) << "<B>>::update_tx(\n";
    out << "            &repository,\n";
    out << "            &mut tx,\n";
    out << "            key_values,\n";
    out << "            Json::Object(document),\n";
    out << "            record.version,\n";
    out << "        )?;\n";
    out << "        let Some(updated) = updated else {\n";
    out << "            return Err(BackendError::Internal { message: \"entity update "
           "failed\".to_string() });\n";
    out << "        };\n";
    out << "        self.backend.commit(tx)?;\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            out << "        let mut body = std::collections::BTreeMap::new();\n";
            for (const auto& field : shape->fields)
            {
                out << "        if let Json::Object(document) = &updated.document {\n";
                out << "            if let Some(value) = document.get("
                    << rust_entity_field_expr(*entity, field.name) << ") {\n";
                out << "                body.insert("
                    << rust_api_body_field_expr(*entity, field.name)
                    << ".to_string(), value.clone());\n";
                out << "            }\n";
                out << "        }\n";
            }
            out << "        Ok(ApiResponse { status_code: 200, body: Json::Object(body) })\n";
        }
        else
        {
            out << "        Ok(ApiResponse { status_code: 200, body: updated.document })\n";
        }
    }
    else
    {
        out << "        Ok(ApiResponse { status_code: 200, body: updated.document })\n";
    }
    return true;
}

bool write_rust_delete_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = delete_entity_for_api_rs(system, api);
    const auto* delete_state =
        entity == nullptr ? nullptr : conventional_soft_delete_terminal_state_rs(*entity);
    if (entity == nullptr || delete_state == nullptr)
    {
        return false;
    }
    out << "        let path_parameters = extract_api_path_parameters("
        << rust_string(api.path.value_or("")) << ", &context.path);\n";
    out << "        let repository = " << rust_entity_repository_type(*entity) << ";\n";
    out << "        <" << rust_entity_repository_type(*entity) << " as "
        << rust_entity_repository_trait(*entity)
        << "<B>>::register_descriptor(&repository, &self.backend)?;\n";
    out << "        let mut tx = self.backend.begin()?;\n";
    out << "        let key_values = vec![\n";
    for (const auto& key_field : entity->key_fields)
    {
        out << "            EntityKeyValue { field: "
            << rust_entity_field_string_expr(*entity, key_field)
            << ", value: path_parameter_json(&path_parameters, "
            << rust_entity_field_expr(*entity, key_field) << ") },\n";
    }
    out << "        ];\n";
    out << "        let record = <" << rust_entity_repository_type(*entity) << " as "
        << rust_entity_repository_trait(*entity) << "<B>>::get_tx(\n";
    out << "            &repository,\n";
    out << "            &mut tx,\n";
    out << "            key_values.clone(),\n";
    out << "        )?;\n";
    out << "        let Some(record) = record else {\n";
    out << "            self.backend.commit(tx)?;\n";
    out << "            return Ok(ApiResponse { status_code: 404, body: "
           "Json::Object(std::collections::BTreeMap::new()) });\n";
    out << "        };\n";
    out << "        let Json::Object(mut document) = record.document.clone() else {\n";
    out << "            return Err(BackendError::InvalidSchema { message: \"entity document must "
           "be an object\".to_string() });\n";
    out << "        };\n";
    out << "        let current_status = match document.get("
        << rust_entity_field_expr(*entity, std::string{EntityStatusFieldName}) << ") {\n";
    out << "            Some(Json::String(value)) => value.clone(),\n";
    out << "            _ => return Err(BackendError::InvalidSchema { message: \"missing entity "
           "field status\".to_string() }),\n";
    out << "        };\n";
    out << "        let requested_status = " << rust_entity_state_expr(*entity, *delete_state)
        << ".to_string();\n";
    out << "        let transition_allowed = current_status == requested_status";
    for (const auto& transition : entity->transitions)
    {
        out << " ||\n";
        out << "            (current_status == " << rust_entity_state_expr(*entity, transition.from)
            << " && requested_status == " << rust_entity_state_expr(*entity, transition.to) << ")";
    }
    out << ";\n";
    out << "        if !transition_allowed {\n";
    out << "            return Err(BackendError::InvalidSchema { message: \"invalid entity delete "
           "transition\".to_string() });\n";
    out << "        }\n";
    out << "        document.insert("
        << rust_entity_field_string_expr(*entity, std::string{EntityStatusFieldName})
        << ", Json::String(requested_status));\n";
    out << "        document.insert("
        << rust_entity_field_string_expr(*entity, std::string{EntityUpdatedAtFieldName})
        << ", "
           "Json::String(generated_api_timestamp()));\n";
    out << "        let updated = <" << rust_entity_repository_type(*entity) << " as "
        << rust_entity_repository_trait(*entity) << "<B>>::update_tx(\n";
    out << "            &repository,\n";
    out << "            &mut tx,\n";
    out << "            key_values,\n";
    out << "            Json::Object(document),\n";
    out << "            record.version,\n";
    out << "        )?;\n";
    out << "        if updated.is_none() {\n";
    out << "            return Err(BackendError::Internal { message: \"entity delete update "
           "failed\".to_string() });\n";
    out << "        }\n";
    out << "        self.backend.commit(tx)?;\n";
    out << "        Ok(ApiResponse { status_code: 204, body: "
           "Json::Object(std::collections::BTreeMap::new()) })\n";
    return true;
}

} // namespace

std::string generate_descriptors_rs(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    std::ostringstream out;
    out << generate_rust_descriptor_prelude(
        system, templates.load("generated/external_system_runtime.rs.tmpl"),
        templates.load("generated/external_system_metadata_runtime.rs.tmpl"), {}
    );
    out << "pub use crate::entity_repository::*;\n";
    out << "pub use crate::runtime_registration::*;\n\n";
    out << generate_rust_feature_flag_descriptors(system);
    out << generate_rust_declaration_descriptors(system);
    out << generate_rust_policy_descriptors(system);
    out << generate_rust_observability_descriptors(system);
    out << generate_rust_runtime_descriptors(system);
    out << generate_rust_observability_registration(system);
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

std::string generate_default_workflow_step_handler_methods_rs(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "    fn handle_" << snake_identifier(workflow.name + "_" + step.name)
                << "(&self, _context: &WorkflowStepHandlerContext) -> BackendResult<()> {\n";
            out << "        Err(BackendError::Internal {\n";
            out << "            message: \"generated workflow step handler " << workflow.name << "."
                << step.name << " is not implemented\".to_string(),\n";
            out << "        })\n";
            out << "    }\n";
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

std::string generate_business_api_operation_handler_methods_rs(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        if (api.entity.has_value() && api.repository_operation.has_value())
        {
            continue;
        }
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

std::string generate_api_operation_default_handler_methods_rs_impl(
    const IrSystem& system,
    std::string_view method_visibility
)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "    " << method_visibility << "fn handle_" << snake_identifier(api.name)
            << "(&self, context: &ApiRequestContext) -> BackendResult<ApiResponse> {\n";
        if (write_rust_create_handler_body(out, system, api))
        {
            out << "    }\n\n";
            continue;
        }
        if (write_rust_get_handler_body(out, system, api))
        {
            out << "    }\n\n";
            continue;
        }
        if (write_rust_list_handler_body(out, system, api))
        {
            out << "    }\n\n";
            continue;
        }
        if (write_rust_update_status_handler_body(out, system, api))
        {
            out << "    }\n\n";
            continue;
        }
        if (write_rust_delete_handler_body(out, system, api))
        {
            out << "    }\n\n";
            continue;
        }
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
                out << "        let response = shapes::" << pascal_identifier(shape->name)
                    << " {\n";
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

std::string generate_api_operation_default_handler_methods_rs(const IrSystem& system)
{
    return generate_api_operation_default_handler_methods_rs_impl(system, {});
}

std::string generate_api_operation_default_handler_domain_methods_rs(const IrSystem& system)
{
    return generate_api_operation_default_handler_methods_rs_impl(system, "pub(super) ");
}

std::string generate_api_codec_helpers_rs()
{
    std::ostringstream out;
    out << "pub(crate) fn codec_error(message: impl Into<String>) -> BackendError {\n";
    out << "    BackendError::Unsupported { message: message.into() }\n";
    out << "}\n\n";
    out << "pub(crate) fn require_member<'a>(value: &'a Json, field_name: &str) -> "
           "BackendResult<&'a Json> "
           "{\n";
    out << "    match value.find(field_name) {\n";
    out << "        Some(Json::Null) | None => Err(codec_error(format!(\"missing required API "
           "field {}\", field_name))),\n";
    out << "        Some(member) => Ok(member),\n";
    out << "    }\n";
    out << "}\n\n";
    out << "pub(crate) fn decode_string(value: &Json, field_name: &str) -> BackendResult<String> "
           "{\n";
    out << "    match value { Json::String(value) => Ok(value.clone()), _ => "
           "Err(codec_error(format!(\"API field {} must be a string\", field_name))) }\n";
    out << "}\n\n";
    out << "pub(crate) fn decode_bool(value: &Json, field_name: &str) -> BackendResult<bool> {\n";
    out << "    match value { Json::Bool(value) => Ok(*value), _ => Err(codec_error(format!(\"API "
           "field {} must be a bool\", field_name))) }\n";
    out << "}\n\n";
    out << "pub(crate) fn decode_i64(value: &Json, field_name: &str) -> BackendResult<i64> {\n";
    out << "    match value { Json::Integer(value) => Ok(*value), _ => "
           "Err(codec_error(format!(\"API field {} must be an integer\", field_name))) }\n";
    out << "}\n\n";
    out << "pub(crate) fn decode_i32(value: &Json, field_name: &str) -> BackendResult<i32> {\n";
    out << "    i32::try_from(decode_i64(value, field_name)?).map_err(|_| "
           "codec_error(format!(\"API field {} exceeds i32 range\", field_name)))\n";
    out << "}\n\n";
    out << "pub(crate) fn decode_f64(value: &Json, field_name: &str) -> BackendResult<f64> {\n";
    out << "    match value { Json::Decimal(value) => Ok(*value), Json::Integer(value) => "
           "Ok(*value as f64), _ => Err(codec_error(format!(\"API field {} must be a number\", "
           "field_name))) }\n";
    out << "}\n\n";
    out << "pub(crate) fn decode_json(value: &Json, _field_name: &str) -> BackendResult<Json> {\n";
    out << "    Ok(value.clone())\n";
    out << "}\n\n";
    return out.str();
}

std::string generate_api_codec_operations_rs(const IrSystem& system)
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
                out << "pub fn decode_" << snake_identifier(api.name)
                    << "_request(request: &ApiRequestContext) -> BackendResult<shapes::"
                    << pascal_identifier(shape->name) << "> {\n";
                out << "    Ok(shapes::" << pascal_identifier(shape->name) << " {\n";
                for (const auto& field : shape->fields)
                {
                    const auto field_name =
                        entity != nullptr ? rust_api_codec_field_name_expr(*entity, field.name)
                                          : rust_string(field.name);
                    out << "        " << field.name << ": ";
                    if (is_optional_type(field.type))
                    {
                        out << "match request.body.find(" << field_name << ") {\n";
                        out << "            Some(Json::Null) | None => None,\n";
                        out << "            Some(member) => Some(" << rust_decode_func(field)
                            << "(member, " << field_name << ")?),\n";
                        out << "        },\n";
                    }
                    else
                    {
                        out << rust_decode_func(field) << "(require_member(&request.body, "
                            << field_name << ")?, " << field_name << ")?,\n";
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
                    << "_response(response: &ApiResponse) -> BackendResult<shapes::"
                    << pascal_identifier(shape->name) << "> {\n";
                out << "    Ok(shapes::" << pascal_identifier(shape->name) << " {\n";
                for (const auto& field : shape->fields)
                {
                    const auto field_name =
                        entity != nullptr ? rust_api_codec_field_name_expr(*entity, field.name)
                                          : rust_string(field.name);
                    out << "        " << field.name << ": ";
                    if (is_optional_type(field.type))
                    {
                        out << "match response.body.find(" << field_name << ") {\n";
                        out << "            Some(Json::Null) | None => None,\n";
                        out << "            Some(member) => Some(" << rust_decode_func(field)
                            << "(member, " << field_name << ")?),\n";
                        out << "        },\n";
                    }
                    else
                    {
                        out << rust_decode_func(field) << "(require_member(&response.body, "
                            << field_name << ")?, " << field_name << ")?,\n";
                    }
                }
                out << "    })\n";
                out << "}\n\n";

                out << "pub fn encode_" << snake_identifier(api.name)
                    << "_response(response: &shapes::" << pascal_identifier(shape->name)
                    << ") -> ApiResponse {\n";
                out << "    encode_" << snake_identifier(api.name)
                    << "_response_with_status(response, 200)\n";
                out << "}\n\n";
                out << "pub fn encode_" << snake_identifier(api.name)
                    << "_response_with_status(response: &shapes::" << pascal_identifier(shape->name)
                    << ", status_code: i32) -> ApiResponse {\n";
                out << "    let mut body = BTreeMap::new();\n";
                for (const auto& field : shape->fields)
                {
                    const auto field_name =
                        entity != nullptr ? rust_api_codec_field_name_expr(*entity, field.name)
                                          : rust_string(field.name);
                    if (is_optional_type(field.type))
                    {
                        const auto base = strip_optional_type(field.type);
                        const auto accessor = (base == "bool" || base == "int" || base == "int32" ||
                                               base == "int64" || base == "long" ||
                                               base == "double" || base == "decimal")
                                                  ? "*value"
                                                  : "value";
                        out << "    if let Some(value) = &response." << field.name << " {\n";
                        out << "        body.insert(" << field_name << ".to_string(), "
                            << rust_encode_expr(field, accessor) << ");\n";
                        out << "    }\n";
                    }
                    else
                    {
                        out << "    body.insert(" << field_name << ".to_string(), "
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

std::string generate_api_codecs_rs(const IrSystem& system)
{
    return generate_api_codec_helpers_rs() + generate_api_codec_operations_rs(system);
}

} // namespace statespec
