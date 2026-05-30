#include "generator_cpp_descriptors.hpp"

#include "generator_cpp_descriptor_areas.hpp"
#include "generator_cpp_descriptor_support.hpp"
#include "generator_entity_index_helpers.hpp"
#include "identifier_case.hpp"
#include "statespec/language_constants.hpp"
#include "statespec/runtime_usage.hpp"
#include "type_syntax.hpp"

#include <algorithm>
#include <sstream>
#include <string_view>

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

bool shape_is_api_contract(
    const IrSystem& system,
    std::string_view shape_name
)
{
    return std::any_of(
        system.apis.begin(), system.apis.end(),
        [&](const auto& api)
        {
            return (api.input.has_value() && *api.input == shape_name) ||
                   (api.output.has_value() && *api.output == shape_name);
        }
    );
}

bool has_common_shapes(const IrSystem& system)
{
    return std::any_of(
        system.shapes.begin(), system.shapes.end(),
        [&](const auto& shape) { return !shape_is_api_contract(system, shape.name); }
    );
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

const IrField* find_entity_field(
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

std::string cpp_entity_field_expr(
    const IrEntity& entity,
    const std::string& field_name
)
{
    if (find_entity_field(entity, field_name) == nullptr)
    {
        return cpp_string(field_name);
    }
    return "::statespec_generated::entities::" + snake_identifier(entity.name) +
           "::constants::" + cpp_entity_field_constant_name(entity.name, field_name);
}

std::string cpp_entity_state_expr(
    const IrEntity& entity,
    const std::string& state_name
)
{
    return "::statespec_generated::entities::" + snake_identifier(entity.name) +
           "::constants::" + cpp_entity_state_constant_name(entity.name, state_name);
}

std::string cpp_api_body_field_expr(
    const IrEntity& entity,
    const std::string& field_name
)
{
    if (find_entity_field(entity, field_name) == nullptr)
    {
        return cpp_string(field_name);
    }
    return cpp_entity_field_expr(entity, field_name);
}

std::string cpp_entity_repository_expr(const IrEntity& entity)
{
    return "::statespec_generated::entities::" + snake_identifier(entity.name) + "::Default" +
           pascal_identifier(entity.name) + "Repository";
}

bool api_repository_operation_is(
    const IrApi& api,
    std::string_view operation
)
{
    return api.repository_operation.has_value() && *api.repository_operation == operation &&
           api.entity.has_value();
}

const IrEntity* create_entity_for_api(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api_repository_operation_is(api, "create") || !api.input)
    {
        return nullptr;
    }
    return find_entity(system, *api.entity);
}

const IrEntity* get_entity_for_api(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api_repository_operation_is(api, "get"))
    {
        return nullptr;
    }
    return find_entity(system, *api.entity);
}

bool ends_with_suffix(
    const std::string& value,
    std::string_view suffix
)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string list_item_type(const std::string& type)
{
    if (ends_with_suffix(type, "[]"))
    {
        return type.substr(0, type.size() - 2);
    }
    return {};
}

const IrField* list_response_field(const IrShape& shape)
{
    for (const auto& field : shape.fields)
    {
        if (!list_item_type(field.type).empty())
        {
            return &field;
        }
    }
    return nullptr;
}

const IrEntity* list_entity_for_api(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api_repository_operation_is(api, "list") || !api.output)
    {
        return nullptr;
    }
    const auto* output = find_shape(system, *api.output);
    if (output == nullptr)
    {
        return nullptr;
    }
    const auto* list_field = list_response_field(*output);
    if (list_field == nullptr)
    {
        return nullptr;
    }
    auto item_type = list_item_type(list_field->type);
    if (ends_with_suffix(item_type, "Response"))
    {
        item_type = item_type.substr(0, item_type.size() - std::string_view{"Response"}.size());
    }
    const auto* entity = find_entity(system, *api.entity);
    return entity != nullptr && entity->name == item_type ? entity : nullptr;
}

const IrEntity* update_status_entity_for_api(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api_repository_operation_is(api, "update_status") || !api.input)
    {
        return nullptr;
    }
    return find_entity(system, *api.entity);
}

const IrEntity* delete_entity_for_api(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api_repository_operation_is(api, "delete"))
    {
        return nullptr;
    }
    return find_entity(system, *api.entity);
}

const std::string* conventional_soft_delete_terminal_state(const IrEntity& entity)
{
    const auto found = std::find(
        entity.terminal_states.begin(), entity.terminal_states.end(),
        std::string{ConventionalSoftDeleteTerminalStateName}
    );
    return found == entity.terminal_states.end() ? nullptr : &*found;
}

bool entity_needs_status_transition_helper(
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

bool status_update_has_required_request_fields(const IrShape& request)
{
    return find_field(request, std::string{EntityStatusFieldName}) != nullptr;
}

bool create_api_has_required_request_fields(
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

std::string cpp_decode_expr(
    const IrField& field,
    std::string value_expr,
    std::string label_expr
)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return "decode_bool(" + value_expr + ", " + label_expr + ")";
    }
    if (base == "int" || base == "int32")
    {
        return "static_cast<std::int32_t>(decode_int64(" + value_expr + ", " + label_expr + "))";
    }
    if (base == "int64" || base == "long")
    {
        return "decode_int64(" + value_expr + ", " + label_expr + ")";
    }
    if (base == "double" || base == "decimal")
    {
        return "decode_double(" + value_expr + ", " + label_expr + ")";
    }
    if (base == "json")
    {
        return value_expr;
    }
    return "decode_string(" + value_expr + ", " + label_expr + ")";
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

std::string cpp_default_response_assignment(
    const IrField& field,
    const std::string& context_expr
)
{
    const auto base = strip_optional_type(field.type);
    const std::string prefix = is_optional_type(field.type) ? "std::optional{" : "";
    const std::string suffix = is_optional_type(field.type) ? "}" : "";
    if (base == "bool")
    {
        return prefix + std::string(field.name == "accepted" ? "true" : "false") + suffix;
    }
    if (base == "int" || base == "int32")
    {
        return prefix + "std::int32_t{0}" + suffix;
    }
    if (base == "int64" || base == "long")
    {
        return prefix + "std::int64_t{0}" + suffix;
    }
    if (base == "double" || base == "decimal")
    {
        return prefix + "0.0" + suffix;
    }
    if (base == "json")
    {
        return prefix + context_expr + ".body" + suffix;
    }
    if (field.name == "workflow_execution_id")
    {
        return prefix + context_expr + ".api_name + \":\" + " + context_expr +
               ".body.canonical_string()" + suffix;
    }
    if (field.name == "message_id")
    {
        return prefix + context_expr + ".api_name + \":\" + " + context_expr +
               ".body.canonical_string()" + suffix;
    }
    if (field.name == EntityStatusFieldName)
    {
        return prefix + "std::string{\"Accepted\"}" + suffix;
    }
    return prefix + "std::string{}" + suffix;
}

std::string cpp_create_response_assignment(
    const IrField& field,
    const IrShape& request,
    const IrEntity& entity,
    const std::string& context_expr
)
{
    if (find_field(request, field.name) != nullptr)
    {
        return "request." + field.name;
    }
    if (field.name == EntityStatusFieldName)
    {
        if (entity.initial_state.has_value())
        {
            return std::string{"std::string{"} +
                   cpp_entity_state_expr(entity, *entity.initial_state) + "}";
        }
        return "std::string{}";
    }
    return cpp_default_response_assignment(field, context_expr);
}

std::string cpp_record_response_assignment(
    const IrField& field,
    const IrEntity& entity,
    const std::string& record_expr
)
{
    return cpp_decode_expr(
        field,
        "require_member(" + record_expr + ".document, " +
            cpp_entity_field_expr(entity, field.name) + ")",
        cpp_entity_field_expr(entity, field.name)
    );
}

std::string cpp_path_value(
    const IrEntity& entity,
    const std::string& field_name
)
{
    return "path_parameter_json(path_parameters, " + cpp_entity_field_expr(entity, field_name) +
           ")";
}

std::string cpp_request_or_path_json_expr(
    const IrEntity& entity,
    const IrShape& request,
    const std::string& field_name
)
{
    if (const auto* request_field = find_field(request, field_name); request_field != nullptr)
    {
        return cpp_json_expr(*request_field, "request." + field_name);
    }
    return cpp_path_value(entity, field_name);
}

void write_cpp_parent_validation(
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
        out << "            " << cpp_entity_repository_expr(*parent) << " parent;\n";
        out << "            const auto parent_record = parent.getTx(\n";
        out << "                *tx,\n";
        out << "                {\n";
        for (const auto& key_field : parent->key_fields)
        {
            out << "                    EntityKeyValue{"
                << cpp_entity_field_expr(*parent, key_field) << ", "
                << cpp_request_or_path_json_expr(entity, request, key_field) << "},\n";
        }
        out << "                }\n";
        out << "            );\n";
        out << "            if (!parent_record.has_value())\n";
        out << "            {\n";
        out << "                throw statespec::backend::BackendError(\"missing required parent "
            << parent->name << "\");\n";
        out << "            }\n";
        out << "        }\n";
    }
}

bool write_cpp_create_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = create_entity_for_api(system, api);
    const auto* request = api.input ? find_shape(system, *api.input) : nullptr;
    if (entity == nullptr || request == nullptr)
    {
        return false;
    }
    if (!create_api_has_required_request_fields(*entity, *request, api))
    {
        out << "        throw statespec::backend::BackendError("
            << cpp_string(
                   "generated create handler for " + api.name +
                   " requires request fields for every non-foundational " + entity->name +
                   " entity field"
               )
            << ");\n";
        return true;
    }

    const auto status = entity->initial_state.value_or("Created");
    out << "        const auto request = decode_" << snake_identifier(api.name)
        << "_request(context);\n";
    out << "        const auto path_parameters = extract_api_path_parameters("
        << cpp_string(api.path.value_or("")) << ", context.path.value_or(std::string{}));\n";
    out << "        " << cpp_entity_repository_expr(*entity) << " repository;\n";
    out << "        repository.register_descriptor(backend_);\n";
    out << "        auto tx = backend_.begin();\n";
    out << "        try\n";
    out << "        {\n";
    write_cpp_parent_validation(out, system, *entity, *request);
    out << "            const auto create_timestamp = generated_api_timestamp();\n";
    out << "            statespec::backend::Json::Object document;\n";
    for (const auto& field : entity->fields)
    {
        if (field.name == EntityCreatedAtFieldName || field.name == EntityUpdatedAtFieldName)
        {
            out << "            document[" << cpp_entity_field_expr(*entity, field.name)
                << "] = statespec::backend::Json{create_timestamp};\n";
        }
        else if (field.name == EntityStatusFieldName)
        {
            out << "            document[" << cpp_entity_field_expr(*entity, field.name)
                << "] = statespec::backend::Json{" << cpp_entity_state_expr(*entity, status)
                << "};\n";
        }
        else if (const auto* request_field = find_field(*request, field.name);
                 request_field != nullptr)
        {
            out << "            document[" << cpp_entity_field_expr(*entity, field.name)
                << "] = " << cpp_json_expr(*request_field, "request." + field.name) << ";\n";
        }
        else if (api.path.value_or("").find("{" + field.name + "}") != std::string::npos)
        {
            out << "            document[" << cpp_entity_field_expr(*entity, field.name)
                << "] = " << cpp_path_value(*entity, field.name) << ";\n";
        }
    }
    out << "            const auto record = repository.createTx(\n";
    out << "                *tx, statespec::backend::Json::object(std::move(document))\n";
    out << "            );\n";
    out << "            if (!record.has_value())\n";
    out << "            {\n";
    out << "                throw statespec::backend::BackendError(\"entity create failed\");\n";
    out << "            }\n";
    out << "            backend_.commit(*tx);\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            out << "            ::statespec_generated::" << pascal_identifier(shape->name)
                << " response{};\n";
            for (const auto& field : shape->fields)
            {
                out << "            response." << field.name << " = "
                    << cpp_create_response_assignment(field, *request, *entity, "context") << ";\n";
            }
            out << "            return encode_" << snake_identifier(api.name)
                << "_response(response, 201);\n";
        }
        else
        {
            out << "            return ApiResponse{201, statespec::backend::Json::object({})};\n";
        }
    }
    else
    {
        out << "            return ApiResponse{201, statespec::backend::Json::object({})};\n";
    }
    out << "        }\n";
    out << "        catch (...)\n";
    out << "        {\n";
    out << "            tx->abort();\n";
    out << "            throw;\n";
    out << "        }\n";
    return true;
}

bool write_cpp_get_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = get_entity_for_api(system, api);
    if (entity == nullptr)
    {
        return false;
    }
    out << "        const auto path_parameters = extract_api_path_parameters("
        << cpp_string(api.path.value_or("")) << ", context.path.value_or(std::string{}));\n";
    out << "        " << cpp_entity_repository_expr(*entity) << " repository;\n";
    out << "        repository.register_descriptor(backend_);\n";
    out << "        auto tx = backend_.begin();\n";
    out << "        try\n";
    out << "        {\n";
    out << "            const auto record = repository.getTx(\n";
    out << "                *tx,\n";
    out << "                {\n";
    for (const auto& key_field : entity->key_fields)
    {
        out << "                    EntityKeyValue{" << cpp_entity_field_expr(*entity, key_field)
            << ", " << cpp_path_value(*entity, key_field) << "},\n";
    }
    out << "                }\n";
    out << "            );\n";
    out << "            backend_.commit(*tx);\n";
    out << "            if (!record.has_value())\n";
    out << "            {\n";
    out << "                return ApiResponse{404, statespec::backend::Json::object({})};\n";
    out << "            }\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            out << "            ::statespec_generated::" << pascal_identifier(shape->name)
                << " response{};\n";
            for (const auto& field : shape->fields)
            {
                out << "            response." << field.name << " = "
                    << cpp_record_response_assignment(field, *entity, "record.value()") << ";\n";
            }
            out << "            return encode_" << snake_identifier(api.name)
                << "_response(response, 200);\n";
        }
        else
        {
            out << "            return ApiResponse{200, record->document};\n";
        }
    }
    else
    {
        out << "            return ApiResponse{200, record->document};\n";
    }
    out << "        }\n";
    out << "        catch (...)\n";
    out << "        {\n";
    out << "            tx->abort();\n";
    out << "            throw;\n";
    out << "        }\n";
    return true;
}

bool write_cpp_list_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = list_entity_for_api(system, api);
    const auto* shape = api.output ? find_shape(system, *api.output) : nullptr;
    const auto* list_field = shape == nullptr ? nullptr : list_response_field(*shape);
    if (entity == nullptr || shape == nullptr || list_field == nullptr)
    {
        return false;
    }
    const auto* index = select_entity_list_index_for_selector(*entity, api.list_selector);
    if (index == nullptr)
    {
        out << "        throw statespec::backend::BackendError("
            << cpp_string(
                   "generated list handler for " + api.name +
                   " requires a key or index backed selector"
               )
            << ");\n";
        return true;
    }
    out << "        const auto path_parameters = extract_api_path_parameters("
        << cpp_string(api.path.value_or("")) << ", context.path.value_or(std::string{}));\n";
    out << "        " << cpp_entity_repository_expr(*entity) << " repository;\n";
    out << "        repository.register_descriptor(backend_);\n";
    out << "        auto tx = backend_.begin();\n";
    out << "        try\n";
    out << "        {\n";
    out << "            const auto records = repository.";
    out << entity_index_repository_method_name(index->name) << "(\n";
    out << "                *tx,\n";
    out << "                {\n";
    for (const auto& field_name : api.list_selector)
    {
        out << "                    statespec::backend::IndexValue::string_value("
               "path_parameter_json(path_parameters, "
            << cpp_entity_field_expr(*entity, field_name) << ").as_string()),\n";
    }
    out << "                }\n";
    out << "            );\n";
    out << "            backend_.commit(*tx);\n";
    out << "            statespec::backend::Json::Object body;\n";
    for (const auto& field : shape->fields)
    {
        if (&field == list_field)
        {
            out << "            statespec::backend::Json::Array items;\n";
            out << "            for (const auto& record : records)\n";
            out << "            {\n";
            out << "                items.push_back(record.document);\n";
            out << "            }\n";
            out << "            body.emplace(" << cpp_api_body_field_expr(*entity, field.name)
                << ", statespec::backend::Json::array(std::move(items)));\n";
        }
        else
        {
            out << "            body.emplace(" << cpp_api_body_field_expr(*entity, field.name)
                << ", " << cpp_path_value(*entity, field.name) << ");\n";
        }
    }
    out << "            return ApiResponse{200, "
           "statespec::backend::Json::object(std::move(body))};\n";
    out << "        }\n";
    out << "        catch (...)\n";
    out << "        {\n";
    out << "            tx->abort();\n";
    out << "            throw;\n";
    out << "        }\n";
    return true;
}

bool write_cpp_update_status_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = update_status_entity_for_api(system, api);
    const auto* request = api.input ? find_shape(system, *api.input) : nullptr;
    if (entity == nullptr || request == nullptr)
    {
        return false;
    }
    if (!status_update_has_required_request_fields(*request))
    {
        out << "        throw statespec::backend::BackendError("
            << cpp_string(
                   "generated status update handler for " + api.name +
                   " requires status and entity key request fields"
               )
            << ");\n";
        return true;
    }

    out << "        const auto request = decode_" << snake_identifier(api.name)
        << "_request(context);\n";
    out << "        const auto path_parameters = extract_api_path_parameters("
        << cpp_string(api.path.value_or("")) << ", context.path.value_or(std::string{}));\n";
    out << "        " << cpp_entity_repository_expr(*entity) << " repository;\n";
    out << "        repository.register_descriptor(backend_);\n";
    out << "        auto tx = backend_.begin();\n";
    out << "        try\n";
    out << "        {\n";
    out << "            std::vector<EntityKeyValue> key_values{\n";
    for (const auto& key_field : entity->key_fields)
    {
        out << "                EntityKeyValue{" << cpp_entity_field_expr(*entity, key_field)
            << ", " << cpp_request_or_path_json_expr(*entity, *request, key_field) << "},\n";
    }
    out << "            };\n";
    out << "            const auto record = repository.getTx(*tx, key_values);\n";
    out << "            if (!record.has_value())\n";
    out << "            {\n";
    out << "                backend_.commit(*tx);\n";
    out << "                return ApiResponse{404, statespec::backend::Json::object({})};\n";
    out << "            }\n";
    out << "            const auto current_status = decode_string(\n";
    out << "                require_member(record->document, "
        << cpp_entity_field_expr(*entity, std::string{EntityStatusFieldName}) << "), "
        << cpp_entity_field_expr(*entity, std::string{EntityStatusFieldName}) << "\n";
    out << "            );\n";
    out << "            const auto requested_status = request.status;\n";
    out << "            if (!" << snake_identifier(entity->name)
        << "_status_transition_allowed(current_status, requested_status))\n";
    out << "            {\n";
    out << "                throw statespec::backend::BackendError(\"invalid entity status "
           "transition\");\n";
    out << "            }\n";
    out << "            auto document = record->document.as_object();\n";
    out << "            document["
        << cpp_entity_field_expr(*entity, std::string{EntityStatusFieldName})
        << "] = statespec::backend::Json{requested_status};\n";
    out << "            document["
        << cpp_entity_field_expr(*entity, std::string{EntityUpdatedAtFieldName})
        << "] = "
           "statespec::backend::Json{generated_api_timestamp()};\n";
    out << "            const auto updated = repository.updateTx(\n";
    out << "                *tx,\n";
    out << "                std::move(key_values),\n";
    out << "                statespec::backend::Json::object(std::move(document)),\n";
    out << "                record->version\n";
    out << "            );\n";
    out << "            if (!updated.has_value())\n";
    out << "            {\n";
    out << "                throw statespec::backend::BackendError(\"entity update failed\");\n";
    out << "            }\n";
    out << "            backend_.commit(*tx);\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            out << "            ::statespec_generated::" << pascal_identifier(shape->name)
                << " response{};\n";
            for (const auto& field : shape->fields)
            {
                out << "            response." << field.name << " = "
                    << cpp_record_response_assignment(field, *entity, "updated.value()") << ";\n";
            }
            out << "            return encode_" << snake_identifier(api.name)
                << "_response(response, 200);\n";
        }
        else
        {
            out << "            return ApiResponse{200, updated->document};\n";
        }
    }
    else
    {
        out << "            return ApiResponse{200, updated->document};\n";
    }
    out << "        }\n";
    out << "        catch (...)\n";
    out << "        {\n";
    out << "            tx->abort();\n";
    out << "            throw;\n";
    out << "        }\n";
    return true;
}

bool write_cpp_delete_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = delete_entity_for_api(system, api);
    const auto* delete_state =
        entity == nullptr ? nullptr : conventional_soft_delete_terminal_state(*entity);
    if (entity == nullptr || delete_state == nullptr)
    {
        return false;
    }

    out << "        const auto path_parameters = extract_api_path_parameters("
        << cpp_string(api.path.value_or("")) << ", context.path.value_or(std::string{}));\n";
    out << "        " << cpp_entity_repository_expr(*entity) << " repository;\n";
    out << "        repository.register_descriptor(backend_);\n";
    out << "        auto tx = backend_.begin();\n";
    out << "        try\n";
    out << "        {\n";
    out << "            std::vector<EntityKeyValue> key_values{\n";
    for (const auto& key_field : entity->key_fields)
    {
        out << "                EntityKeyValue{" << cpp_entity_field_expr(*entity, key_field)
            << ", " << cpp_path_value(*entity, key_field) << "},\n";
    }
    out << "            };\n";
    out << "            const auto record = repository.getTx(*tx, key_values);\n";
    out << "            if (!record.has_value())\n";
    out << "            {\n";
    out << "                backend_.commit(*tx);\n";
    out << "                return ApiResponse{404, statespec::backend::Json::object({})};\n";
    out << "            }\n";
    out << "            const auto current_status = decode_string(\n";
    out << "                require_member(record->document, "
        << cpp_entity_field_expr(*entity, std::string{EntityStatusFieldName}) << "), "
        << cpp_entity_field_expr(*entity, std::string{EntityStatusFieldName}) << "\n";
    out << "            );\n";
    out << "            const auto requested_status = std::string{"
        << cpp_entity_state_expr(*entity, *delete_state) << "};\n";
    out << "            if (!" << snake_identifier(entity->name)
        << "_status_transition_allowed(current_status, requested_status))\n";
    out << "            {\n";
    out << "                throw statespec::backend::BackendError(\"invalid entity delete "
           "transition\");\n";
    out << "            }\n";
    out << "            auto document = record->document.as_object();\n";
    out << "            document["
        << cpp_entity_field_expr(*entity, std::string{EntityStatusFieldName})
        << "] = statespec::backend::Json{requested_status};\n";
    out << "            document["
        << cpp_entity_field_expr(*entity, std::string{EntityUpdatedAtFieldName})
        << "] = "
           "statespec::backend::Json{generated_api_timestamp()};\n";
    out << "            const auto updated = repository.updateTx(\n";
    out << "                *tx,\n";
    out << "                std::move(key_values),\n";
    out << "                statespec::backend::Json::object(std::move(document)),\n";
    out << "                record->version\n";
    out << "            );\n";
    out << "            if (!updated.has_value())\n";
    out << "            {\n";
    out << "                throw statespec::backend::BackendError(\"entity delete update "
           "failed\");\n";
    out << "            }\n";
    out << "            backend_.commit(*tx);\n";
    out << "            return ApiResponse{204, statespec::backend::Json::object({})};\n";
    out << "        }\n";
    out << "        catch (...)\n";
    out << "        {\n";
    out << "            tx->abort();\n";
    out << "            throw;\n";
    out << "        }\n";
    return true;
}

} // namespace

std::string generate_cpp_runtime_registration_includes(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream out;
    auto add = [&](bool used, std::string_view path)
    {
        if (used)
        {
            out << "#include \"" << path << "\"\n";
        }
    };
    add(usage.uses_feature_flags, "descriptors/runtime/feature_flags.hpp");
    add(usage.uses_queues, "descriptors/runtime/queues.hpp");
    add(usage.uses_leases, "descriptors/runtime/leases.hpp");
    add(usage.uses_logs, "descriptors/runtime/logs.hpp");
    add(usage.uses_metrics, "descriptors/runtime/metrics.hpp");
    add(usage.uses_logs && usage.uses_metrics, "descriptors/runtime/observability.hpp");
    add(usage.uses_workflows, "descriptors/runtime/workflows.hpp");
    if (usage.uses_any_runtime_domain)
    {
        out << "\n";
    }
    return out.str();
}

std::string generate_system_descriptors_header(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    std::ostringstream out;
    out << generate_cpp_descriptor_prelude(
        system, templates.load("generated/external_system_runtime.hpp.tmpl"),
        templates.load("generated/external_system_metadata_runtime.hpp.tmpl"), {}
    );
    out << "#include \"descriptors/values_enums.hpp\"\n\n";
    out << "#include \"descriptors/events.hpp\"\n\n";
    out << "#include \"descriptors/external_systems.hpp\"\n\n";
    out << "#include \"descriptors/policies.hpp\"\n\n";
    if (has_common_shapes(system))
    {
        out << "#include \"descriptors/shapes.hpp\"\n\n";
    }
    out << "#include \"descriptors/runtime.hpp\"\n\n";
    out << "} // namespace statespec_generated\n";
    out << "\n#include \"runtime_registration.hpp\"\n";
    return out.str();
}

std::string generate_business_api_operation_handler_methods(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        if (api.entity.has_value() && api.repository_operation.has_value())
        {
            continue;
        }
        out << "    virtual ApiResponse handle_" << snake_identifier(api.name)
            << "(const ApiRequestContext& context) = 0;\n";
    }
    return out.str();
}

std::string generate_api_operation_default_handler_methods_impl(
    const IrSystem& system,
    bool include_override
)
{
    std::ostringstream out;
    out << "    static std::string generated_api_timestamp()\n";
    out << "    {\n";
    out << "        return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(\n";
    out << "            std::chrono::system_clock::now().time_since_epoch()).count());\n";
    out << "    }\n\n";
    for (const auto& entity : system.entities)
    {
        if (!entity_needs_status_transition_helper(system, entity))
        {
            continue;
        }
        out << "    static bool " << snake_identifier(entity.name)
            << "_status_transition_allowed(\n";
        out << "        const std::string& current_status,\n";
        out << "        const std::string& requested_status\n";
        out << "    )\n";
        out << "    {\n";
        out << "        return current_status == requested_status";
        for (const auto& transition : entity.transitions)
        {
            out << " ||\n";
            out << "            (current_status == "
                << cpp_entity_state_expr(entity, transition.from)
                << " && requested_status == " << cpp_entity_state_expr(entity, transition.to)
                << ")";
        }
        out << ";\n";
        out << "    }\n\n";
    }
    for (const auto& api : system.apis)
    {
        out << "    ApiResponse handle_" << snake_identifier(api.name)
            << "(const ApiRequestContext& context)";
        if (include_override)
        {
            out << " override";
        }
        out << "\n";
        out << "    {\n";
        if (write_cpp_create_handler_body(out, system, api))
        {
            out << "    }\n\n";
            continue;
        }
        if (write_cpp_get_handler_body(out, system, api))
        {
            out << "    }\n\n";
            continue;
        }
        if (write_cpp_list_handler_body(out, system, api))
        {
            out << "    }\n\n";
            continue;
        }
        if (write_cpp_update_status_handler_body(out, system, api))
        {
            out << "    }\n\n";
            continue;
        }
        if (write_cpp_delete_handler_body(out, system, api))
        {
            out << "    }\n\n";
            continue;
        }
        if (api.input.has_value())
        {
            out << "        const auto request = decode_" << snake_identifier(api.name)
                << "_request(context);\n";
            out << "        (void)request;\n";
        }
        if (api.output.has_value())
        {
            if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
            {
                out << "        ::statespec_generated::" << pascal_identifier(shape->name)
                    << " response{};\n";
                for (const auto& field : shape->fields)
                {
                    out << "        response." << field.name << " = "
                        << cpp_default_response_assignment(field, "context") << ";\n";
                }
                out << "        return encode_" << snake_identifier(api.name)
                    << "_response(response, "
                    << (api.starts_workflow.has_value() || api.enqueues.has_value() ? 202 : 200)
                    << ");\n";
            }
            else
            {
                out << "        return ApiResponse{"
                    << (api.starts_workflow.has_value() || api.enqueues.has_value() ? 202 : 200)
                    << ", statespec::backend::Json::object({})};\n";
            }
        }
        else
        {
            out << "        return ApiResponse{"
                << (api.starts_workflow.has_value() || api.enqueues.has_value() ? 202 : 200)
                << ", statespec::backend::Json::object({})};\n";
        }
        out << "    }\n\n";
    }
    return out.str();
}

std::string generate_api_operation_default_handler_domain_methods(const IrSystem& system)
{
    return generate_api_operation_default_handler_methods_impl(system, false);
}

std::string generate_api_codec_helpers()
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
    return out.str();
}

std::string generate_api_codec_operations(const IrSystem& system)
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
                out << "inline ::statespec_generated::" << pascal_identifier(shape->name)
                    << " decode_" << snake_identifier(api.name)
                    << "_request(const ApiRequestContext& context)\n";
                out << "{\n";
                out << "    ::statespec_generated::" << pascal_identifier(shape->name)
                    << " decoded{};\n";
                for (const auto& field : shape->fields)
                {
                    const auto field_name = entity != nullptr
                                                ? cpp_api_codec_field_name_expr(*entity, field.name)
                                                : cpp_string(field.name);
                    if (is_optional_type(field.type))
                    {
                        out << "    if (const auto* member = context.body.find(" << field_name
                            << "); member != nullptr && !member->is_null())\n";
                        out << "    {\n";
                        out << "        decoded." << field.name << " = "
                            << cpp_decode_expr(field, "*member", field_name) << ";\n";
                        out << "    }\n";
                    }
                    else
                    {
                        out << "    decoded." << field.name << " = "
                            << cpp_decode_expr(
                                   field, "require_member(context.body, " + field_name + ")",
                                   field_name
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
                    const auto field_name = entity != nullptr
                                                ? cpp_api_codec_field_name_expr(*entity, field.name)
                                                : cpp_string(field.name);
                    if (is_optional_type(field.type))
                    {
                        out << "    if (response." << field.name << ".has_value())\n";
                        out << "    {\n";
                        out << "        body.emplace(" << field_name << ", "
                            << cpp_json_expr(field, "*response." + field.name) << ");\n";
                        out << "    }\n";
                    }
                    else
                    {
                        out << "    body.emplace(" << field_name << ", "
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
                    const auto field_name = entity != nullptr
                                                ? cpp_api_codec_field_name_expr(*entity, field.name)
                                                : cpp_string(field.name);
                    if (is_optional_type(field.type))
                    {
                        out << "    if (const auto* member = response.body.find(" << field_name
                            << "); member != nullptr && !member->is_null())\n";
                        out << "    {\n";
                        out << "        decoded." << field.name << " = "
                            << cpp_decode_expr(field, "*member", field_name) << ";\n";
                        out << "    }\n";
                    }
                    else
                    {
                        out << "    decoded." << field.name << " = "
                            << cpp_decode_expr(
                                   field, "require_member(response.body, " + field_name + ")",
                                   field_name
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

std::string generate_api_codecs(const IrSystem& system)
{
    return generate_api_codec_helpers() + generate_api_codec_operations(system);
}

} // namespace statespec
