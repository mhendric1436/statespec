#include "generator_java_descriptors.hpp"

#include "generator_entity_index_helpers.hpp"
#include "generator_java_descriptor_areas.hpp"
#include "generator_java_descriptor_support.hpp"
#include "identifier_case.hpp"
#include "statespec/language_constants.hpp"
#include "type_syntax.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

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

const IrField* find_entity_field_java(
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

std::string java_entity_descriptor_module_ref(std::string_view entity_name)
{
    return "com.statespec.generated.entities." + snake_identifier(std::string{entity_name}) +
           ".Persistence";
}

std::string java_entity_model_ref(std::string_view entity_name)
{
    return "com.statespec.generated.entities." + snake_identifier(std::string{entity_name}) +
           ".Model";
}

std::string java_entity_field_expr(
    const IrEntity& entity,
    const std::string& field_name
)
{
    if (find_entity_field_java(entity, field_name) == nullptr)
    {
        return java_string(field_name);
    }
    return java_entity_model_ref(entity.name) + "." +
           java_entity_field_constant_name(entity.name, field_name);
}

std::string java_entity_state_expr(
    const IrEntity& entity,
    const std::string& state_name
)
{
    return java_entity_model_ref(entity.name) + "." +
           java_entity_state_constant_name(entity.name, state_name);
}

const IrEntity* create_entity_for_api_java(
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

const IrEntity* get_entity_for_api_java(
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

bool ends_with_suffix_java(
    const std::string& value,
    std::string_view suffix
)
{
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string list_item_type_java(const std::string& type)
{
    if (ends_with_suffix_java(type, "[]"))
    {
        return type.substr(0, type.size() - 2);
    }
    return {};
}

const IrField* list_response_field_java(const IrShape& shape)
{
    for (const auto& field : shape.fields)
    {
        if (!list_item_type_java(field.type).empty())
        {
            return &field;
        }
    }
    return nullptr;
}

const IrEntity* list_entity_for_api_java(
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
    const auto* list_field = list_response_field_java(*output);
    if (list_field == nullptr)
    {
        return nullptr;
    }
    auto item_type = list_item_type_java(list_field->type);
    if (ends_with_suffix_java(item_type, "Response"))
    {
        item_type = item_type.substr(0, item_type.size() - std::string_view{"Response"}.size());
    }
    return find_entity(system, item_type);
}

const IrEntity* update_status_entity_for_api_java(
    const IrSystem& system,
    const IrApi& api
)
{
    constexpr std::string_view prefix = "Update";
    constexpr std::string_view suffix = "Status";
    if (api.method.value_or("") != "PATCH" || api.name.rfind(prefix, 0) != 0 ||
        !ends_with_suffix_java(api.name, suffix) || !api.input)
    {
        return nullptr;
    }
    return find_entity(
        system, api.name.substr(prefix.size(), api.name.size() - prefix.size() - suffix.size())
    );
}

const IrEntity* delete_entity_for_api_java(
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

const std::string* conventional_soft_delete_terminal_state_java(const IrEntity& entity)
{
    const auto found = std::find(
        entity.terminal_states.begin(), entity.terminal_states.end(),
        std::string{ConventionalSoftDeleteTerminalStateName}
    );
    return found == entity.terminal_states.end() ? nullptr : &*found;
}

bool status_update_has_required_request_fields_java(
    const IrEntity& entity,
    const IrShape& request
)
{
    if (find_field(request, std::string{EntityStatusFieldName}) == nullptr)
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

bool create_api_has_required_request_fields_java(
    const IrEntity& entity,
    const IrShape& request
)
{
    for (const auto& field : entity.fields)
    {
        if (field.name == EntityCreatedAtFieldName || field.name == EntityUpdatedAtFieldName ||
            field.name == EntityStatusFieldName)
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

std::string java_decode_func(const IrField& field)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return "decodeBoolean";
    }
    if (base == "int" || base == "int32")
    {
        return "decodeInteger";
    }
    if (base == "int64" || base == "long")
    {
        return "decodeLong";
    }
    if (base == "double" || base == "decimal")
    {
        return "decodeDouble";
    }
    if (base == "json")
    {
        return "decodeJson";
    }
    return "decodeString";
}

std::string java_encode_expr(
    const IrField& field,
    const std::string& accessor
)
{
    const auto base = strip_optional_type(field.type);
    if (base == "bool")
    {
        return "Json.bool(" + accessor + ")";
    }
    if (base == "int" || base == "int32" || base == "int64" || base == "long")
    {
        return "Json.integer(" + accessor + ")";
    }
    if (base == "double" || base == "decimal")
    {
        return "Json.decimal(java.math.BigDecimal.valueOf(" + accessor + "))";
    }
    if (base == "json")
    {
        return accessor;
    }
    return "Json.string(" + accessor + ")";
}

std::string java_default_response_expr(
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
        return "0";
    }
    if (base == "int64" || base == "long")
    {
        return "0L";
    }
    if (base == "double" || base == "decimal")
    {
        return "0.0";
    }
    if (base == "json")
    {
        return context_expr + ".body()";
    }
    if (field.name == "workflow_execution_id" || field.name == "message_id")
    {
        return context_expr + ".apiName() + \":\" + " + context_expr + ".body().canonicalString()";
    }
    if (field.name == EntityStatusFieldName)
    {
        return "\"Accepted\"";
    }
    return "\"\"";
}

std::string java_create_response_expr(
    const IrField& field,
    const IrShape& request,
    const IrEntity& entity,
    const std::string& context_expr
)
{
    if (find_field(request, field.name) != nullptr)
    {
        return "request." + field.name + "()";
    }
    if (field.name == EntityStatusFieldName)
    {
        if (entity.initial_state.has_value())
        {
            return java_entity_state_expr(entity, *entity.initial_state);
        }
        return "\"\"";
    }
    return java_default_response_expr(field, context_expr);
}

std::string java_api_json_expr(
    const IrField& field,
    const std::string& accessor
)
{
    auto expression = java_encode_expr(field, accessor);
    const std::string from = "Json.";
    const std::string to = "com.statespec.backend.Json.";
    std::size_t pos = 0;
    while ((pos = expression.find(from, pos)) != std::string::npos)
    {
        expression.replace(pos, from.size(), to);
        pos += to.size();
    }
    return expression;
}

void write_java_parent_validation(
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
        out << "            {\n";
        out << "                var parent = new "
            << java_entity_descriptor_module_ref(parent->name) << ".Default"
            << pascal_identifier(parent->name) << "Repository();\n";
        out << "                var parentRecord = parent.getTx(\n";
        out << "                    tx,\n";
        out << "                    java.util.List.of(\n";
        for (std::size_t i = 0; i < parent->key_fields.size(); ++i)
        {
            const auto& key_field = parent->key_fields[i];
            const auto* request_field = find_field(request, key_field);
            out << "                        new com.statespec.generated.entities.EntityKeyValue("
                << java_entity_field_expr(*parent, key_field) << ", ";
            if (request_field == nullptr)
            {
                out << "com.statespec.backend.Json.nullValue()";
            }
            else
            {
                out << java_api_json_expr(*request_field, "request." + key_field + "()");
            }
            out << ")";
            out << (i + 1 < parent->key_fields.size() ? "," : "") << "\n";
        }
        out << "                    )\n";
        out << "                );\n";
        out << "                if (parentRecord.isEmpty()) {\n";
        out << "                    throw new "
               "com.statespec.backend.Backend.BackendException(\"missing required parent "
            << parent->name << "\");\n";
        out << "                }\n";
        out << "            }\n";
    }
}

bool write_java_create_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = create_entity_for_api_java(system, api);
    const auto* request = api.input ? find_shape(system, *api.input) : nullptr;
    if (entity == nullptr || request == nullptr)
    {
        return false;
    }
    if (!create_api_has_required_request_fields_java(*entity, *request))
    {
        out << "            throw new com.statespec.backend.Backend.BackendException("
            << java_string(
                   "generated create handler for " + api.name +
                   " requires request fields for every non-foundational " + entity->name +
                   " entity field"
               )
            << ");\n";
        return true;
    }
    const auto status = entity->initial_state.value_or("Created");
    out << "            var request = ApiCodecs.decode" << pascal_identifier(api.name)
        << "Request(context);\n";
    out << "            var repository = new " << java_entity_descriptor_module_ref(entity->name)
        << ".Default" << pascal_identifier(entity->name) << "Repository();\n";
    out << "            repository.registerDescriptor(backend);\n";
    out << "            var tx = backend.begin();\n";
    out << "            try {\n";
    write_java_parent_validation(out, system, *entity, *request);
    out << "                var document = new java.util.HashMap<String, "
           "com.statespec.backend.Json>();\n";
    for (const auto& field : entity->fields)
    {
        if (field.name == EntityCreatedAtFieldName || field.name == EntityUpdatedAtFieldName)
        {
            out << "                document.put(" << java_entity_field_expr(*entity, field.name)
                << ", com.statespec.backend.Json.string(java.time.Instant.now().toString()));\n";
        }
        else if (field.name == EntityStatusFieldName)
        {
            out << "                document.put(" << java_entity_field_expr(*entity, field.name)
                << ", com.statespec.backend.Json.string(" << java_entity_state_expr(*entity, status)
                << "));\n";
        }
        else if (const auto* request_field = find_field(*request, field.name);
                 request_field != nullptr)
        {
            out << "                document.put(" << java_entity_field_expr(*entity, field.name)
                << ", " << java_api_json_expr(*request_field, "request." + field.name + "()")
                << ");\n";
        }
    }
    out << "                var record = repository.createTx(\n";
    out << "                    tx, com.statespec.backend.Json.object(document)\n";
    out << "                );\n";
    out << "                if (record.isEmpty()) {\n";
    out << "                    throw new com.statespec.backend.Backend.BackendException(\"entity "
           "create failed\");\n";
    out << "                }\n";
    out << "                backend.commit(tx);\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            out << "                var response = new " << pascal_identifier(shape->name) << "(\n";
            for (std::size_t i = 0; i < shape->fields.size(); ++i)
            {
                const auto& field = shape->fields[i];
                out << "                    ";
                if (is_optional_type(field.type))
                {
                    out << "Optional.of("
                        << java_create_response_expr(field, *request, *entity, "context") << ")";
                }
                else
                {
                    out << java_create_response_expr(field, *request, *entity, "context");
                }
                out << (i + 1 < shape->fields.size() ? "," : "") << "\n";
            }
            out << "                );\n";
            out << "                return ApiCodecs.encode" << pascal_identifier(api.name)
                << "Response(response, 201);\n";
        }
        else
        {
            out << "                return new Descriptors.ApiResponse(201, "
                   "com.statespec.backend.Json.object(java.util.Map.of()));\n";
        }
    }
    else
    {
        out << "                return new Descriptors.ApiResponse(201, "
               "com.statespec.backend.Json.object(java.util.Map.of()));\n";
    }
    out << "            } catch (Exception error) {\n";
    out << "                tx.abort();\n";
    out << "                throw error;\n";
    out << "            }\n";
    return true;
}

bool write_java_get_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = get_entity_for_api_java(system, api);
    if (entity == nullptr)
    {
        return false;
    }
    out << "            var pathParameters = extractApiPathParameters("
        << java_string(api.path.value_or("")) << ", context.path());\n";
    out << "            var repository = new " << java_entity_descriptor_module_ref(entity->name)
        << ".Default" << pascal_identifier(entity->name) << "Repository();\n";
    out << "            repository.registerDescriptor(backend);\n";
    out << "            var tx = backend.begin();\n";
    out << "            try {\n";
    out << "                var record = repository.getTx(\n";
    out << "                    tx,\n";
    out << "                    java.util.List.of(\n";
    for (std::size_t i = 0; i < entity->key_fields.size(); ++i)
    {
        const auto& key_field = entity->key_fields[i];
        out << "                        new com.statespec.generated.entities.EntityKeyValue("
            << java_entity_field_expr(*entity, key_field) << ", pathParameterJson(pathParameters, "
            << java_entity_field_expr(*entity, key_field) << "))"
            << (i + 1 < entity->key_fields.size() ? "," : "") << "\n";
    }
    out << "                    )\n";
    out << "                );\n";
    out << "                backend.commit(tx);\n";
    out << "                if (record.isEmpty()) {\n";
    out << "                    return new Descriptors.ApiResponse(404, "
           "com.statespec.backend.Json.object(java.util.Map.of()));\n";
    out << "                }\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            out << "                var document = record.get().document();\n";
            out << "                var response = new " << pascal_identifier(shape->name) << "(\n";
            for (std::size_t i = 0; i < shape->fields.size(); ++i)
            {
                const auto& field = shape->fields[i];
                out << "                    ";
                if (is_optional_type(field.type))
                {
                    out << "Optional.of(ApiCodecs." << java_decode_func(field) << "(document.find("
                        << java_entity_field_expr(*entity, field.name) << ").orElseThrow(), "
                        << java_entity_field_expr(*entity, field.name) << "))";
                }
                else
                {
                    out << "ApiCodecs." << java_decode_func(field) << "(document.find("
                        << java_entity_field_expr(*entity, field.name) << ").orElseThrow(), "
                        << java_entity_field_expr(*entity, field.name) << ")";
                }
                out << (i + 1 < shape->fields.size() ? "," : "") << "\n";
            }
            out << "                );\n";
            out << "                return ApiCodecs.encode" << pascal_identifier(api.name)
                << "Response(response, 200);\n";
        }
        else
        {
            out << "                return new Descriptors.ApiResponse(200, "
                   "record.get().document());\n";
        }
    }
    else
    {
        out << "                return new Descriptors.ApiResponse(200, "
               "record.get().document());\n";
    }
    out << "            } catch (Exception error) {\n";
    out << "                if (tx.isOpen()) {\n";
    out << "                    tx.abort();\n";
    out << "                }\n";
    out << "                throw error;\n";
    out << "            }\n";
    return true;
}

bool write_java_list_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = list_entity_for_api_java(system, api);
    const auto* shape = api.output ? find_shape(system, *api.output) : nullptr;
    const auto* list_field = shape == nullptr ? nullptr : list_response_field_java(*shape);
    if (entity == nullptr || shape == nullptr || list_field == nullptr)
    {
        return false;
    }
    const auto* index = select_entity_list_index(*entity, api.path.value_or(""));
    out << "            var pathParameters = extractApiPathParameters("
        << java_string(api.path.value_or("")) << ", context.path());\n";
    out << "            var repository = new " << java_entity_descriptor_module_ref(entity->name)
        << ".Default" << pascal_identifier(entity->name) << "Repository();\n";
    out << "            repository.registerDescriptor(backend);\n";
    out << "            var tx = backend.begin();\n";
    out << "            try {\n";
    out << "                var records = repository.";
    if (index == nullptr)
    {
        out << "listByIndexTx(\n";
    }
    else
    {
        out << entity_index_repository_method_name(index->name) << "(\n";
    }
    out << "                    tx,\n";
    if (index == nullptr)
    {
        out << "                    " << java_string("") << ",\n";
    }
    out << "                    java.util.List.of(\n";
    bool wrote_value = false;
    if (index != nullptr)
    {
        for (const auto& field_name : index->fields)
        {
            if (api.path.value_or("").find("{" + field_name + "}") != std::string::npos)
            {
                if (wrote_value)
                {
                    out << ",\n";
                }
                out << "                        new "
                       "com.statespec.backend.Backend.IndexValue.StringValue("
                       "pathParameters.getOrDefault("
                    << java_entity_field_expr(*entity, field_name) << ", \"\"))";
                wrote_value = true;
            }
        }
    }
    out << "\n";
    out << "                    )\n";
    out << "                );\n";
    out << "                backend.commit(tx);\n";
    out << "                var body = new java.util.HashMap<String, "
           "com.statespec.backend.Json>();\n";
    for (const auto& field : shape->fields)
    {
        if (&field == list_field)
        {
            out << "                var items = new "
                   "java.util.ArrayList<com.statespec.backend.Json>();\n";
            out << "                for (var record : records) {\n";
            out << "                    items.add(record.document());\n";
            out << "                }\n";
            out << "                body.put(" << java_string(field.name)
                << ", com.statespec.backend.Json.array(items));\n";
        }
        else
        {
            out << "                body.put(" << java_string(field.name)
                << ", pathParameterJson(pathParameters, "
                << java_entity_field_expr(*entity, field.name) << "));\n";
        }
    }
    out << "                return new Descriptors.ApiResponse(200, "
           "com.statespec.backend.Json.object(body));\n";
    out << "            } catch (Exception error) {\n";
    out << "                if (tx.isOpen()) {\n";
    out << "                    tx.abort();\n";
    out << "                }\n";
    out << "                throw error;\n";
    out << "            }\n";
    return true;
}

bool write_java_update_status_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = update_status_entity_for_api_java(system, api);
    const auto* request = api.input ? find_shape(system, *api.input) : nullptr;
    if (entity == nullptr || request == nullptr)
    {
        return false;
    }
    if (!status_update_has_required_request_fields_java(*entity, *request))
    {
        out << "            throw new com.statespec.backend.Backend.BackendException("
            << java_string(
                   "generated status update handler for " + api.name +
                   " requires status and entity key request fields"
               )
            << ");\n";
        return true;
    }
    out << "            var request = ApiCodecs.decode" << pascal_identifier(api.name)
        << "Request(context);\n";
    out << "            var repository = new " << java_entity_descriptor_module_ref(entity->name)
        << ".Default" << pascal_identifier(entity->name) << "Repository();\n";
    out << "            repository.registerDescriptor(backend);\n";
    out << "            var tx = backend.begin();\n";
    out << "            try {\n";
    out << "                var keyValues = java.util.List.of(\n";
    for (std::size_t i = 0; i < entity->key_fields.size(); ++i)
    {
        const auto& key_field = entity->key_fields[i];
        const auto* field = find_field(*request, key_field);
        out << "                    new com.statespec.generated.entities.EntityKeyValue("
            << java_entity_field_expr(*entity, key_field) << ", "
            << java_api_json_expr(*field, "request." + key_field + "()") << ")"
            << (i + 1 < entity->key_fields.size() ? "," : "") << "\n";
    }
    out << "                );\n";
    out << "                var record = repository.getTx(tx, keyValues);\n";
    out << "                if (record.isEmpty()) {\n";
    out << "                    backend.commit(tx);\n";
    out << "                    return new Descriptors.ApiResponse(404, "
           "com.statespec.backend.Json.object(java.util.Map.of()));\n";
    out << "                }\n";
    out << "                var currentStatus = ApiCodecs.decodeString(\n";
    out << "                    record.get().document().find("
        << java_entity_field_expr(*entity, std::string{EntityStatusFieldName})
        << ").orElseThrow(), "
        << java_entity_field_expr(*entity, std::string{EntityStatusFieldName}) << "\n";
    out << "                );\n";
    out << "                var requestedStatus = request.status();\n";
    out << "                var transitionAllowed = currentStatus.equals(requestedStatus)";
    for (const auto& transition : entity->transitions)
    {
        out << " ||\n";
        out << "                    (currentStatus.equals("
            << java_entity_state_expr(*entity, transition.from) << ") && requestedStatus.equals("
            << java_entity_state_expr(*entity, transition.to) << "))";
    }
    out << ";\n";
    out << "                if (!transitionAllowed) {\n";
    out << "                    throw new com.statespec.backend.Backend.BackendException(\"invalid "
           "entity status transition\");\n";
    out << "                }\n";
    out << "                if (!(record.get().document() instanceof "
           "com.statespec.backend.Json.ObjectValue objectValue)) {\n";
    out << "                    throw new com.statespec.backend.Backend.BackendException(\"entity "
           "document must be an object\");\n";
    out << "                }\n";
    out << "                var document = new java.util.HashMap<String, "
           "com.statespec.backend.Json>(objectValue.values());\n";
    out << "                document.put("
        << java_entity_field_expr(*entity, std::string{EntityStatusFieldName})
        << ", "
           "com.statespec.backend.Json.string(requestedStatus));\n";
    out << "                document.put("
        << java_entity_field_expr(*entity, std::string{EntityUpdatedAtFieldName})
        << ", "
           "com.statespec.backend.Json.string(java.time.Instant.now().toString()));\n";
    out << "                var updated = repository.updateTx(\n";
    out << "                    tx,\n";
    out << "                    keyValues,\n";
    out << "                    com.statespec.backend.Json.object(document),\n";
    out << "                    record.get().version()\n";
    out << "                );\n";
    out << "                if (updated.isEmpty()) {\n";
    out << "                    throw new com.statespec.backend.Backend.BackendException(\"entity "
           "update failed\");\n";
    out << "                }\n";
    out << "                backend.commit(tx);\n";
    if (api.output.has_value())
    {
        if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
        {
            out << "                var responseDocument = updated.get().document();\n";
            out << "                var response = new " << pascal_identifier(shape->name) << "(\n";
            for (std::size_t i = 0; i < shape->fields.size(); ++i)
            {
                const auto& field = shape->fields[i];
                out << "                    ";
                if (is_optional_type(field.type))
                {
                    out << "Optional.of(ApiCodecs." << java_decode_func(field)
                        << "(responseDocument.find(" << java_entity_field_expr(*entity, field.name)
                        << ").orElseThrow(), " << java_entity_field_expr(*entity, field.name)
                        << "))";
                }
                else
                {
                    out << "ApiCodecs." << java_decode_func(field) << "(responseDocument.find("
                        << java_entity_field_expr(*entity, field.name) << ").orElseThrow(), "
                        << java_entity_field_expr(*entity, field.name) << ")";
                }
                out << (i + 1 < shape->fields.size() ? "," : "") << "\n";
            }
            out << "                );\n";
            out << "                return ApiCodecs.encode" << pascal_identifier(api.name)
                << "Response(response, 200);\n";
        }
        else
        {
            out << "                return new Descriptors.ApiResponse(200, "
                   "updated.get().document());\n";
        }
    }
    else
    {
        out << "                return new Descriptors.ApiResponse(200, "
               "updated.get().document());\n";
    }
    out << "            } catch (Exception error) {\n";
    out << "                if (tx.isOpen()) {\n";
    out << "                    tx.abort();\n";
    out << "                }\n";
    out << "                throw error;\n";
    out << "            }\n";
    return true;
}

bool write_java_delete_handler_body(
    std::ostringstream& out,
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = delete_entity_for_api_java(system, api);
    const auto* delete_state =
        entity == nullptr ? nullptr : conventional_soft_delete_terminal_state_java(*entity);
    if (entity == nullptr || delete_state == nullptr)
    {
        return false;
    }
    out << "            var pathParameters = extractApiPathParameters("
        << java_string(api.path.value_or("")) << ", context.path());\n";
    out << "            var repository = new " << java_entity_descriptor_module_ref(entity->name)
        << ".Default" << pascal_identifier(entity->name) << "Repository();\n";
    out << "            repository.registerDescriptor(backend);\n";
    out << "            var tx = backend.begin();\n";
    out << "            try {\n";
    out << "                var keyValues = java.util.List.of(\n";
    for (std::size_t i = 0; i < entity->key_fields.size(); ++i)
    {
        const auto& key_field = entity->key_fields[i];
        out << "                    new com.statespec.generated.entities.EntityKeyValue("
            << java_entity_field_expr(*entity, key_field) << ", pathParameterJson(pathParameters, "
            << java_entity_field_expr(*entity, key_field) << "))"
            << (i + 1 < entity->key_fields.size() ? "," : "") << "\n";
    }
    out << "                );\n";
    out << "                var record = repository.getTx(tx, keyValues);\n";
    out << "                if (record.isEmpty()) {\n";
    out << "                    backend.commit(tx);\n";
    out << "                    return new Descriptors.ApiResponse(404, "
           "com.statespec.backend.Json.object(java.util.Map.of()));\n";
    out << "                }\n";
    out << "                var currentStatus = ApiCodecs.decodeString(\n";
    out << "                    record.get().document().find("
        << java_entity_field_expr(*entity, std::string{EntityStatusFieldName})
        << ").orElseThrow(), "
        << java_entity_field_expr(*entity, std::string{EntityStatusFieldName}) << "\n";
    out << "                );\n";
    out << "                var requestedStatus = "
        << java_entity_state_expr(*entity, *delete_state) << ";\n";
    out << "                var transitionAllowed = currentStatus.equals(requestedStatus)";
    for (const auto& transition : entity->transitions)
    {
        out << " ||\n";
        out << "                    (currentStatus.equals("
            << java_entity_state_expr(*entity, transition.from) << ") && requestedStatus.equals("
            << java_entity_state_expr(*entity, transition.to) << "))";
    }
    out << ";\n";
    out << "                if (!transitionAllowed) {\n";
    out << "                    throw new com.statespec.backend.Backend.BackendException(\"invalid "
           "entity delete transition\");\n";
    out << "                }\n";
    out << "                if (!(record.get().document() instanceof "
           "com.statespec.backend.Json.ObjectValue objectValue)) {\n";
    out << "                    throw new com.statespec.backend.Backend.BackendException(\"entity "
           "document must be an object\");\n";
    out << "                }\n";
    out << "                var document = new java.util.HashMap<String, "
           "com.statespec.backend.Json>(objectValue.values());\n";
    out << "                document.put("
        << java_entity_field_expr(*entity, std::string{EntityStatusFieldName})
        << ", "
           "com.statespec.backend.Json.string(requestedStatus));\n";
    out << "                document.put("
        << java_entity_field_expr(*entity, std::string{EntityUpdatedAtFieldName})
        << ", "
           "com.statespec.backend.Json.string(java.time.Instant.now().toString()));\n";
    out << "                var updated = repository.updateTx(\n";
    out << "                    tx,\n";
    out << "                    keyValues,\n";
    out << "                    com.statespec.backend.Json.object(document),\n";
    out << "                    record.get().version()\n";
    out << "                );\n";
    out << "                if (updated.isEmpty()) {\n";
    out << "                    throw new com.statespec.backend.Backend.BackendException(\"entity "
           "delete update failed\");\n";
    out << "                }\n";
    out << "                backend.commit(tx);\n";
    out << "                return new Descriptors.ApiResponse(204, "
           "com.statespec.backend.Json.object(java.util.Map.of()));\n";
    out << "            } catch (Exception error) {\n";
    out << "                if (tx.isOpen()) {\n";
    out << "                    tx.abort();\n";
    out << "                }\n";
    out << "                throw error;\n";
    out << "            }\n";
    return true;
}

} // namespace

std::string generate_java_external_system_descriptor_delegates()
{
    std::ostringstream out;
    out << "    public static List<ExternalSystemDescriptor> externalSystemDescriptors() {\n";
    out << "        return ExternalSystemDescriptorModule.externalSystemDescriptors();\n";
    out << "    }\n\n";
    out << "    public static ExternalSystemMetadataMappingPlan "
           "externalSystemMetadataMappingPlan(\n";
    out << "        ExternalSystemDescriptor descriptor\n";
    out << "    ) {\n";
    out << "        return ExternalSystemDescriptorModule.externalSystemMetadataMappingPlan("
           "descriptor);\n";
    out << "    }\n\n";
    out << "    public static Optional<ExternalSystem.MetadataLookup> "
           "externalSystemMetadataLookup(\n";
    out << "        ExternalSystemDescriptor descriptor,\n";
    out << "        List<ExternalSystem.MetadataKeyValue> keyValues\n";
    out << "    ) {\n";
    out << "        return ExternalSystemDescriptorModule.externalSystemMetadataLookup("
           "descriptor, keyValues);\n";
    out << "    }\n\n";
    out << "    public static Optional<ExternalSystem.MetadataLookup> "
           "externalSystemMetadataLookup(\n";
    out << "        String externalSystem,\n";
    out << "        List<ExternalSystem.MetadataKeyValue> keyValues\n";
    out << "    ) {\n";
    out << "        return ExternalSystemDescriptorModule.externalSystemMetadataLookup("
           "externalSystem, keyValues);\n";
    out << "    }\n\n";
    out << "    public static Optional<ExternalSystem.MetadataResolution> "
           "resolveExternalSystemMetadataTx(\n";
    out << "        ExternalSystem resolver,\n";
    out << "        Backend.Transaction tx,\n";
    out << "        ExternalSystemDescriptor descriptor,\n";
    out << "        List<ExternalSystem.MetadataKeyValue> keyValues\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        return ExternalSystemDescriptorModule.resolveExternalSystemMetadataTx(\n";
    out << "            resolver, tx, descriptor, keyValues\n";
    out << "        );\n";
    out << "    }\n\n";
    out << "    public static Optional<ExternalSystem.MetadataResolution> "
           "resolveExternalSystemMetadataTx(\n";
    out << "        ExternalSystem resolver,\n";
    out << "        Backend.Transaction tx,\n";
    out << "        String externalSystem,\n";
    out << "        List<ExternalSystem.MetadataKeyValue> keyValues\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        return ExternalSystemDescriptorModule.resolveExternalSystemMetadataTx(\n";
    out << "            resolver, tx, externalSystem, keyValues\n";
    out << "        );\n";
    out << "    }\n\n";
    out << "    public static ExternalSystemCallRequest buildExternalSystemCallRequest(\n";
    out << "        ExternalSystemMetadataMappingApplicator applicator,\n";
    out << "        ExternalSystemDescriptor descriptor,\n";
    out << "        ExternalSystemMetadataMappingInputs inputs\n";
    out << "    ) throws Exception {\n";
    out << "        return ExternalSystemDescriptorModule.buildExternalSystemCallRequest(\n";
    out << "            applicator, descriptor, inputs\n";
    out << "        );\n";
    out << "    }\n\n";
    out << "    public static ExternalSystemCallResponse callExternalSystem(\n";
    out << "        ExternalSystemClient client,\n";
    out << "        ExternalSystemMetadataMappingApplicator applicator,\n";
    out << "        ExternalSystemDescriptor descriptor,\n";
    out << "        ExternalSystemMetadataMappingInputs inputs\n";
    out << "    ) throws Exception {\n";
    out << "        return ExternalSystemDescriptorModule.callExternalSystem(\n";
    out << "            client, applicator, descriptor, inputs\n";
    out << "        );\n";
    out << "    }\n\n";
    out << "    public static Optional<ExternalSystemCallResponse> callExternalSystem(\n";
    out << "        ExternalSystemClient client,\n";
    out << "        ExternalSystemMetadataMappingApplicator applicator,\n";
    out << "        String externalSystem,\n";
    out << "        ExternalSystemMetadataMappingInputs inputs\n";
    out << "    ) throws Exception {\n";
    out << "        return ExternalSystemDescriptorModule.callExternalSystem(\n";
    out << "            client, applicator, externalSystem, inputs\n";
    out << "        );\n";
    out << "    }\n\n";
    return out.str();
}

std::string generate_descriptors_java(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    std::ostringstream out;
    out << generate_java_descriptor_prelude(
        system, templates.load("generated/ExternalSystemRuntime.java.tmpl"),
        templates.load("generated/ExternalSystemMetadataRuntime.java.tmpl"), {}
    );
    out << generate_java_feature_flag_descriptors(system);
    out << generate_java_declaration_descriptors(system);
    out << generate_java_external_system_descriptor_delegates();
    out << generate_java_api_descriptors(system);
    out << generate_java_worker_descriptors(system);
    out << generate_java_policy_descriptors(system);
    out << "    public static List<ShapeDescriptor> shapeDescriptors() {\n";
    out << "        return ShapeDescriptorModule.shapeDescriptors();\n";
    out << "    }\n\n";
    out << generate_java_observability_descriptors(system);
    out << generate_java_runtime_descriptors(system);
    out << generate_java_observability_registration(system);
    out << "}\n";
    return out.str();
}

std::string generate_workflow_step_handler_keys_java(const IrSystem& system)
{
    std::ostringstream out;
    std::vector<std::string> keys;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            keys.push_back(workflow.name + "." + step.name);
        }
    }
    for (std::size_t i = 0; i < keys.size(); ++i)
    {
        out << "            " << java_string(keys[i]) << (i + 1 < keys.size() ? "," : "") << "\n";
    }
    return out.str();
}

std::string generate_workflow_step_handler_methods_java(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "        void handle" << pascal_identifier(workflow.name + "_" + step.name)
                << "(Context context) throws Exception;\n";
        }
    }
    return out.str();
}

std::string generate_default_workflow_step_handler_methods_java(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "        @Override\n";
            out << "        public void handle"
                << pascal_identifier(workflow.name + "_" + step.name) << "(Context context) {\n";
            out << "            throw new UnsupportedOperationException(\"generated workflow step "
                   "handler "
                << workflow.name << "." << step.name << " is not implemented\");\n";
            out << "        }\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_dispatch_cases_java(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        for (const auto& step : workflow.steps)
        {
            out << "            if (record.workflowName().equals(" << java_string(workflow.name)
                << ") && record.currentStep().equals(" << java_string(step.name) << ")) {\n";
            out << "                handler.handle"
                << pascal_identifier(workflow.name + "_" + step.name) << "(context);\n";
            out << "                handled = true;\n";
            out << "            }\n";
        }
    }
    return out.str();
}

std::string generate_workflow_step_next_cases_java(const IrSystem& system)
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
                out << "            if (record.workflowName().equals(" << java_string(workflow.name)
                    << ") && record.currentStep().equals(" << java_string(step.name) << ")) {\n";
                out << "                nextStep = Optional.of(" << java_string(*statement.target)
                    << ");\n";
                out << "            }\n";
            }
        }
    }
    return out.str();
}

std::string generate_api_operation_handler_methods_java(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "        Descriptors.ApiResponse handle" << pascal_identifier(api.name)
            << "(Descriptors.ApiRequestContext context) throws Exception;\n";
    }
    return out.str();
}

std::string generate_api_operation_dispatch_cases_java(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        out << "        if (route.get().apiName().equals(" << java_string(api.name) << ")) {\n";
        out << "            return Optional.of(handler.handle" << pascal_identifier(api.name)
            << "(context));\n";
        out << "        }\n";
    }
    return out.str();
}

std::string generate_api_operation_default_handler_methods_java_impl(
    const IrSystem& system,
    bool include_override
)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        if (include_override)
        {
            out << "        @Override\n";
        }
        out << "        public Descriptors.ApiResponse handle" << pascal_identifier(api.name)
            << "(Descriptors.ApiRequestContext context) throws Exception {\n";
        if (write_java_create_handler_body(out, system, api))
        {
            out << "        }\n\n";
            continue;
        }
        if (write_java_get_handler_body(out, system, api))
        {
            out << "        }\n\n";
            continue;
        }
        if (write_java_list_handler_body(out, system, api))
        {
            out << "        }\n\n";
            continue;
        }
        if (write_java_update_status_handler_body(out, system, api))
        {
            out << "        }\n\n";
            continue;
        }
        if (write_java_delete_handler_body(out, system, api))
        {
            out << "        }\n\n";
            continue;
        }
        if (api.input.has_value())
        {
            out << "            var request = ApiCodecs.decode" << pascal_identifier(api.name)
                << "Request(context);\n";
            out << "            java.util.Objects.requireNonNull(request);\n";
        }
        const auto status_code =
            api.starts_workflow.has_value() || api.enqueues.has_value() ? 202 : 200;
        if (api.output.has_value())
        {
            if (const auto* shape = find_shape(system, *api.output); shape != nullptr)
            {
                out << "            var response = new " << pascal_identifier(shape->name) << "(\n";
                for (std::size_t i = 0; i < shape->fields.size(); ++i)
                {
                    const auto& field = shape->fields[i];
                    out << "                ";
                    if (is_optional_type(field.type))
                    {
                        out << "Optional.of(" << java_default_response_expr(field, "context")
                            << ")";
                    }
                    else
                    {
                        out << java_default_response_expr(field, "context");
                    }
                    out << (i + 1 < shape->fields.size() ? "," : "") << "\n";
                }
                out << "            );\n";
                out << "            return ApiCodecs.encode" << pascal_identifier(api.name)
                    << "Response(response, " << status_code << ");\n";
            }
            else
            {
                out << "            return new Descriptors.ApiResponse(" << status_code
                    << ", com.statespec.backend.Json.object(java.util.Map.of()));\n";
            }
        }
        else
        {
            out << "            return new Descriptors.ApiResponse(" << status_code
                << ", com.statespec.backend.Json.object(java.util.Map.of()));\n";
        }
        out << "        }\n\n";
    }
    return out.str();
}

std::string generate_api_operation_default_handler_methods_java(const IrSystem& system)
{
    return generate_api_operation_default_handler_methods_java_impl(system, true);
}

std::string generate_api_operation_default_handler_domain_methods_java(const IrSystem& system)
{
    return generate_api_operation_default_handler_methods_java_impl(system, false);
}

std::string generate_api_codec_helpers_java()
{
    std::ostringstream out;
    out << "    public static Json requireMember(Json value, String fieldName) {\n";
    out << "        return value.find(fieldName).filter(member -> !(member instanceof "
           "Json.NullValue)).orElseThrow(() -> new IllegalArgumentException(\"missing required "
           "API field \" + fieldName));\n";
    out << "    }\n\n";
    out << "    public static String decodeString(Json value, String fieldName) {\n";
    out << "        if (value instanceof Json.StringValue stringValue) { return "
           "stringValue.value(); }\n";
    out << "        throw new IllegalArgumentException(\"API field \" + fieldName + \" must be a "
           "string\");\n";
    out << "    }\n\n";
    out << "    public static Boolean decodeBoolean(Json value, String fieldName) {\n";
    out << "        if (value instanceof Json.BooleanValue booleanValue) { return "
           "booleanValue.value(); }\n";
    out << "        throw new IllegalArgumentException(\"API field \" + fieldName + \" must be a "
           "bool\");\n";
    out << "    }\n\n";
    out << "    public static Long decodeLong(Json value, String fieldName) {\n";
    out << "        if (value instanceof Json.IntegerValue integerValue) { return "
           "integerValue.value(); }\n";
    out << "        throw new IllegalArgumentException(\"API field \" + fieldName + \" must be an "
           "integer\");\n";
    out << "    }\n\n";
    out << "    public static Integer decodeInteger(Json value, String fieldName) {\n";
    out << "        return Math.toIntExact(decodeLong(value, fieldName));\n";
    out << "    }\n\n";
    out << "    public static Double decodeDouble(Json value, String fieldName) {\n";
    out << "        if (value instanceof Json.DecimalValue decimalValue) { return "
           "decimalValue.value().doubleValue(); }\n";
    out << "        if (value instanceof Json.IntegerValue integerValue) { return "
           "(double) integerValue.value(); }\n";
    out << "        throw new IllegalArgumentException(\"API field \" + fieldName + \" must be a "
           "number\");\n";
    out << "    }\n\n";
    out << "    public static Json decodeJson(Json value, String fieldName) {\n";
    out << "        return value;\n";
    out << "    }\n\n";
    return out.str();
}

std::string generate_api_codec_operations_java(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        if (api.input.has_value())
        {
            const auto* shape = find_shape(system, *api.input);
            if (shape != nullptr)
            {
                out << "    public static " << pascal_identifier(shape->name) << " decode"
                    << pascal_identifier(api.name)
                    << "Request(Descriptors.ApiRequestContext request) {\n";
                out << "        return new " << pascal_identifier(shape->name) << "(\n";
                for (std::size_t i = 0; i < shape->fields.size(); ++i)
                {
                    const auto& field = shape->fields[i];
                    out << "            ";
                    if (is_optional_type(field.type))
                    {
                        out << "request.body().find(" << java_string(field.name)
                            << ").filter(member -> !(member instanceof Json.NullValue)).map(member "
                               "-> Optional.of("
                            << java_decode_func(field) << "(member, " << java_string(field.name)
                            << "))).orElse(Optional.empty())";
                    }
                    else
                    {
                        out << java_decode_func(field) << "(requireMember(request.body(), "
                            << java_string(field.name) << "), " << java_string(field.name) << ")";
                    }
                    out << (i + 1 < shape->fields.size() ? "," : "") << "\n";
                }
                out << "        );\n";
                out << "    }\n\n";
            }
        }
        if (api.output.has_value())
        {
            const auto* shape = find_shape(system, *api.output);
            if (shape != nullptr)
            {
                out << "    public static " << pascal_identifier(shape->name) << " decode"
                    << pascal_identifier(api.name)
                    << "Response(Descriptors.ApiResponse response) {\n";
                out << "        return new " << pascal_identifier(shape->name) << "(\n";
                for (std::size_t i = 0; i < shape->fields.size(); ++i)
                {
                    const auto& field = shape->fields[i];
                    out << "            ";
                    if (is_optional_type(field.type))
                    {
                        out << "response.body().find(" << java_string(field.name)
                            << ").filter(member -> !(member instanceof Json.NullValue)).map(member "
                               "-> Optional.of("
                            << java_decode_func(field) << "(member, " << java_string(field.name)
                            << "))).orElse(Optional.empty())";
                    }
                    else
                    {
                        out << java_decode_func(field) << "(requireMember(response.body(), "
                            << java_string(field.name) << "), " << java_string(field.name) << ")";
                    }
                    out << (i + 1 < shape->fields.size() ? "," : "") << "\n";
                }
                out << "        );\n";
                out << "    }\n\n";

                out << "    public static Descriptors.ApiResponse encode"
                    << pascal_identifier(api.name) << "Response(" << pascal_identifier(shape->name)
                    << " response) {\n";
                out << "        return encode" << pascal_identifier(api.name)
                    << "Response(response, 200);\n";
                out << "    }\n\n";
                out << "    public static Descriptors.ApiResponse encode"
                    << pascal_identifier(api.name) << "Response(" << pascal_identifier(shape->name)
                    << " response, int statusCode) {\n";
                out << "        Map<String, Json> body = new TreeMap<>();\n";
                for (const auto& field : shape->fields)
                {
                    const auto accessor = "response." + field.name + "()";
                    if (is_optional_type(field.type))
                    {
                        out << "        " << accessor << ".ifPresent(value -> body.put("
                            << java_string(field.name) << ", " << java_encode_expr(field, "value")
                            << "));\n";
                    }
                    else
                    {
                        out << "        body.put(" << java_string(field.name) << ", "
                            << java_encode_expr(field, accessor) << ");\n";
                    }
                }
                out << "        return new Descriptors.ApiResponse(statusCode, "
                       "Json.object(body));\n";
                out << "    }\n\n";
            }
        }
    }
    return out.str();
}

std::string generate_api_codecs_java(const IrSystem& system)
{
    return generate_api_codec_helpers_java() + generate_api_codec_operations_java(system);
}

} // namespace statespec
