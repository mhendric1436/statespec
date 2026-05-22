#include "generator_rust_descriptors.hpp"

#include "generator_rust_descriptor_areas.hpp"
#include "generator_rust_descriptor_support.hpp"
#include "identifier_case.hpp"
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

const IrEntity* create_entity_for_api_rs(
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

const IrEntity* get_entity_for_api_rs(
    const IrSystem& system,
    const IrApi& api
)
{
    constexpr std::string_view prefix = "Get";
    if (api.method.value_or("") != "GET" || api.name.rfind(prefix, 0) != 0)
    {
        return nullptr;
    }
    return find_entity(system, api.name.substr(prefix.size()));
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
    if (api.method.value_or("") != "GET" || api.name.rfind("List", 0) != 0 || !api.output)
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
    return find_entity(system, item_type);
}

const IrEntity* update_status_entity_for_api_rs(
    const IrSystem& system,
    const IrApi& api
)
{
    constexpr std::string_view prefix = "Update";
    constexpr std::string_view suffix = "Status";
    if (api.method.value_or("") != "PATCH" || api.name.rfind(prefix, 0) != 0 ||
        !ends_with_suffix_rs(api.name, suffix) || !api.input)
    {
        return nullptr;
    }
    return find_entity(
        system, api.name.substr(prefix.size(), api.name.size() - prefix.size() - suffix.size())
    );
}

const IrEntity* delete_entity_for_api_rs(
    const IrSystem& system,
    const IrApi& api
)
{
    constexpr std::string_view prefix = "Delete";
    if (api.method.value_or("") != "DELETE" || api.name.rfind(prefix, 0) != 0)
    {
        return nullptr;
    }
    return find_entity(system, api.name.substr(prefix.size()));
}

bool has_terminal_deleted_state_rs(const IrEntity& entity)
{
    return std::find(entity.terminal_states.begin(), entity.terminal_states.end(), "Deleted") !=
           entity.terminal_states.end();
}

const IrIndex* select_list_index_rs(const IrEntity& entity)
{
    if (!entity.indexes.empty())
    {
        return &entity.indexes.front();
    }
    return nullptr;
}

bool status_update_has_required_request_fields_rs(
    const IrEntity& entity,
    const IrShape& request
)
{
    if (find_field(request, "status") == nullptr)
    {
        return false;
    }
    for (const auto& key_field : entity.key_fields)
    {
        if (find_field(request, key_field) == nullptr)
        {
            return false;
        }
    }
    return true;
}

bool create_api_has_required_request_fields_rs(
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
    if (field.name == "status")
    {
        return rust_string(entity.initial_state.value_or("Created")) + ".to_string()";
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
        out << "            let parent = Default" << pascal_identifier(parent->name)
            << "Repository;\n";
        out << "            let parent_record = <Default" << pascal_identifier(parent->name)
            << "Repository as " << pascal_identifier(parent->name) << "Repository<B>>::get_tx(\n";
        out << "                &parent,\n";
        out << "                &mut tx,\n";
        out << "                vec![\n";
        for (const auto& key_field : parent->key_fields)
        {
            const auto* request_field = find_field(request, key_field);
            out << "                    EntityKeyValue { field: " << rust_string(key_field)
                << ".to_string(), value: ";
            if (request_field == nullptr)
            {
                out << "Json::Null";
            }
            else
            {
                out << rust_encode_expr(*request_field, "request." + key_field);
            }
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
    if (!create_api_has_required_request_fields_rs(*entity, *request))
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
    out << "        let repository = Default" << pascal_identifier(entity->name) << "Repository;\n";
    out << "        <Default" << pascal_identifier(entity->name) << "Repository as "
        << pascal_identifier(entity->name)
        << "Repository<B>>::register_descriptor(&repository, &self.backend)?;\n";
    out << "        let mut tx = self.backend.begin()?;\n";
    write_rust_parent_validation(out, system, *entity, *request);
    out << "        let mut document = std::collections::BTreeMap::new();\n";
    for (const auto& field : entity->fields)
    {
        if (field.name == "created_at" || field.name == "updated_at")
        {
            out << "        document.insert(" << rust_string(field.name)
                << ".to_string(), Json::String(\"0\".to_string()));\n";
        }
        else if (field.name == "status")
        {
            out << "        document.insert(" << rust_string(field.name)
                << ".to_string(), Json::String(" << rust_string(status) << ".to_string()));\n";
        }
        else if (const auto* request_field = find_field(*request, field.name);
                 request_field != nullptr)
        {
            out << "        document.insert(" << rust_string(field.name) << ".to_string(), "
                << rust_encode_expr(*request_field, "request." + field.name) << ");\n";
        }
    }
    out << "        let record = <Default" << pascal_identifier(entity->name) << "Repository as "
        << pascal_identifier(entity->name) << "Repository<B>>::create_tx(\n";
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
            out << "        let response = " << pascal_identifier(shape->name) << " {\n";
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
    out << "        let repository = Default" << pascal_identifier(entity->name) << "Repository;\n";
    out << "        <Default" << pascal_identifier(entity->name) << "Repository as "
        << pascal_identifier(entity->name)
        << "Repository<B>>::register_descriptor(&repository, &self.backend)?;\n";
    out << "        let mut tx = self.backend.begin()?;\n";
    out << "        let record = <Default" << pascal_identifier(entity->name) << "Repository as "
        << pascal_identifier(entity->name) << "Repository<B>>::get_tx(\n";
    out << "            &repository,\n";
    out << "            &mut tx,\n";
    out << "            vec![\n";
    for (const auto& key_field : entity->key_fields)
    {
        out << "                EntityKeyValue { field: " << rust_string(key_field)
            << ".to_string(), value: path_parameter_json(&path_parameters, "
            << rust_string(key_field) << ") },\n";
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
                out << "            if let Some(value) = document.get(" << rust_string(field.name)
                    << ") {\n";
                out << "                body.insert(" << rust_string(field.name)
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
    const auto* index = select_list_index_rs(*entity);
    out << "        let path_parameters = extract_api_path_parameters("
        << rust_string(api.path.value_or("")) << ", &context.path);\n";
    out << "        let repository = Default" << pascal_identifier(entity->name) << "Repository;\n";
    out << "        <Default" << pascal_identifier(entity->name) << "Repository as "
        << pascal_identifier(entity->name)
        << "Repository<B>>::register_descriptor(&repository, &self.backend)?;\n";
    out << "        let mut tx = self.backend.begin()?;\n";
    out << "        let records = <Default" << pascal_identifier(entity->name) << "Repository as "
        << pascal_identifier(entity->name) << "Repository<B>>::list_by_index_tx(\n";
    out << "            &repository,\n";
    out << "            &mut tx,\n";
    out << "            " << rust_string(index == nullptr ? "" : index->name) << ".to_string(),\n";
    out << "            vec![\n";
    if (index != nullptr)
    {
        for (const auto& field_name : index->fields)
        {
            if (api.path.value_or("").find("{" + field_name + "}") != std::string::npos)
            {
                out << "                IndexValue::String(path_parameters.get("
                    << rust_string(field_name) << ").cloned().unwrap_or_else(String::new)),\n";
            }
        }
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
            out << "            " << rust_string(field.name) << ".to_string(),\n";
            out << "            Json::Array(records.into_iter().map(|record| "
                   "record.document).collect()),\n";
            out << "        );\n";
        }
        else
        {
            out << "        body.insert(" << rust_string(field.name)
                << ".to_string(), path_parameter_json(&path_parameters, " << rust_string(field.name)
                << "));\n";
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
    if (!status_update_has_required_request_fields_rs(*entity, *request))
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
    out << "        let repository = Default" << pascal_identifier(entity->name) << "Repository;\n";
    out << "        <Default" << pascal_identifier(entity->name) << "Repository as "
        << pascal_identifier(entity->name)
        << "Repository<B>>::register_descriptor(&repository, &self.backend)?;\n";
    out << "        let mut tx = self.backend.begin()?;\n";
    out << "        let key_values = vec![\n";
    for (const auto& key_field : entity->key_fields)
    {
        const auto* field = find_field(*request, key_field);
        out << "            EntityKeyValue { field: " << rust_string(key_field)
            << ".to_string(), value: " << rust_encode_expr(*field, "request." + key_field)
            << " },\n";
    }
    out << "        ];\n";
    out << "        let record = <Default" << pascal_identifier(entity->name) << "Repository as "
        << pascal_identifier(entity->name) << "Repository<B>>::get_tx(\n";
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
    out << "        let current_status = match document.get(\"status\") {\n";
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
        out << "            (current_status == " << rust_string(transition.from)
            << " && requested_status == " << rust_string(transition.to) << ")";
    }
    out << ";\n";
    out << "        if !transition_allowed {\n";
    out << "            return Err(BackendError::InvalidSchema { message: \"invalid entity status "
           "transition\".to_string() });\n";
    out << "        }\n";
    out << "        document.insert(\"status\".to_string(), Json::String(requested_status));\n";
    out << "        document.insert(\"updated_at\".to_string(), "
           "Json::String(\"0\".to_string()));\n";
    out << "        let updated = <Default" << pascal_identifier(entity->name) << "Repository as "
        << pascal_identifier(entity->name) << "Repository<B>>::update_tx(\n";
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
                out << "            if let Some(value) = document.get(" << rust_string(field.name)
                    << ") {\n";
                out << "                body.insert(" << rust_string(field.name)
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
    if (entity == nullptr || !has_terminal_deleted_state_rs(*entity))
    {
        return false;
    }
    out << "        let path_parameters = crate::api_codecs::extract_api_path_parameters("
        << rust_string(api.path.value_or("")) << ", context.path.as_deref().unwrap_or(\"\"));\n";
    out << "        let repository = Default" << pascal_identifier(entity->name) << "Repository;\n";
    out << "        <Default" << pascal_identifier(entity->name) << "Repository as "
        << pascal_identifier(entity->name)
        << "Repository<B>>::register_descriptor(&repository, &self.backend)?;\n";
    out << "        let mut tx = self.backend.begin()?;\n";
    out << "        let key_values = vec![\n";
    for (const auto& key_field : entity->key_fields)
    {
        out << "            EntityKeyValue { field: " << rust_string(key_field)
            << ".to_string(), value: crate::api_codecs::path_parameter_json(&path_parameters, "
            << rust_string(key_field) << ") },\n";
    }
    out << "        ];\n";
    out << "        let record = <Default" << pascal_identifier(entity->name) << "Repository as "
        << pascal_identifier(entity->name) << "Repository<B>>::get_tx(\n";
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
    out << "        let current_status = match document.get(\"status\") {\n";
    out << "            Some(Json::String(value)) => value.clone(),\n";
    out << "            _ => return Err(BackendError::InvalidSchema { message: \"missing entity "
           "field status\".to_string() }),\n";
    out << "        };\n";
    out << "        let requested_status = \"Deleted\".to_string();\n";
    out << "        let transition_allowed = current_status == requested_status";
    for (const auto& transition : entity->transitions)
    {
        out << " ||\n";
        out << "            (current_status == " << rust_string(transition.from)
            << " && requested_status == " << rust_string(transition.to) << ")";
    }
    out << ";\n";
    out << "        if !transition_allowed {\n";
    out << "            return Err(BackendError::InvalidSchema { message: \"invalid entity delete "
           "transition\".to_string() });\n";
    out << "        }\n";
    out << "        document.insert(\"status\".to_string(), Json::String(requested_status));\n";
    out << "        document.insert(\"updated_at\".to_string(), "
           "Json::String(\"0\".to_string()));\n";
    out << "        let updated = <Default" << pascal_identifier(entity->name) << "Repository as "
        << pascal_identifier(entity->name) << "Repository<B>>::update_tx(\n";
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
        templates.load("generated/external_system_metadata_runtime.rs.tmpl"),
        templates.load("generated/entity_repository.rs.tmpl")
    );
    out << generate_rust_feature_flag_descriptors(system);
    out << generate_rust_declaration_descriptors(system);
    out << generate_rust_external_system_descriptors(
        system, templates.load("generated/external_system_call_adapters.rs.tmpl")
    );
    out << generate_rust_api_descriptors(system);
    out << generate_rust_worker_descriptors(system);
    out << generate_rust_policy_descriptors(system);
    out << generate_rust_shape_descriptors(system);
    out << generate_rust_observability_descriptors(system);
    out << generate_rust_entity_descriptors(system);
    out << generate_rust_runtime_descriptors(system);
    out << generate_rust_observability_registration(system);
    out << generate_rust_runtime_registration(system, templates);
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
