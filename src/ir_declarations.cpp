#include "ir_declarations.hpp"

#include "identifier_case.hpp"
#include "ir_lowering_helpers.hpp"
#include "statespec/language_constants.hpp"

#include <algorithm>
#include <optional>
#include <utility>

namespace statespec
{

namespace
{

bool has_shape(
    const IrSystem& ir,
    const std::string& name
)
{
    return std::any_of(
        ir.shapes.begin(), ir.shapes.end(), [&](const IrShape& shape) { return shape.name == name; }
    );
}

std::string plural_pascal_identifier(const std::string& value)
{
    const auto singular = pascal_identifier(value, "Entities");
    if (!singular.empty() && singular.back() == 'y')
    {
        return singular.substr(0, singular.size() - 1) + "ies";
    }
    if (!singular.empty() && singular.back() == 's')
    {
        return singular + "es";
    }
    return singular + "s";
}

std::string field_type(
    const SemanticEntity& entity,
    const std::string& field_name
)
{
    for (const auto& field : entity.fields)
    {
        if (field.name == field_name)
        {
            return field.type;
        }
    }
    return "string";
}

bool entity_has_field(
    const SemanticEntity& entity,
    const std::string& field_name
)
{
    return std::any_of(
        entity.fields.begin(), entity.fields.end(),
        [&](const SemanticField& field) { return field.name == field_name; }
    );
}

std::vector<std::string> path_parameters(const std::optional<std::string>& path)
{
    std::vector<std::string> parameters;
    if (!path.has_value())
    {
        return parameters;
    }

    std::size_t cursor = 0;
    while (cursor < path->size())
    {
        const auto open = path->find('{', cursor);
        if (open == std::string::npos)
        {
            break;
        }
        const auto close = path->find('}', open + 1);
        if (close == std::string::npos)
        {
            break;
        }
        if (close > open + 1)
        {
            parameters.push_back(path->substr(open + 1, close - open - 1));
        }
        cursor = close + 1;
    }
    return parameters;
}

bool contains_field(
    const std::vector<IrField>& fields,
    const std::string& name
)
{
    return std::any_of(
        fields.begin(), fields.end(), [&](const IrField& field) { return field.name == name; }
    );
}

void append_field(
    std::vector<IrField>& fields,
    const SemanticEntity& entity,
    const std::string& field_name
)
{
    if (!contains_field(fields, field_name))
    {
        fields.push_back(IrField{field_name, field_type(entity, field_name)});
    }
}

std::vector<IrField> entity_response_fields(const SemanticEntity& entity)
{
    std::vector<IrField> fields;
    for (const auto& key_field : entity.key_fields)
    {
        append_field(fields, entity, key_field);
    }

    for (const auto& field : entity.fields)
    {
        if (contains_field(fields, field.name) || field.name == EntityCreatedAtFieldName ||
            field.name == EntityUpdatedAtFieldName || field.name == EntityStatusFieldName)
        {
            continue;
        }
        fields.push_back(IrField{field.name, field.type});
    }

    if (entity_has_field(entity, std::string{EntityStatusFieldName}))
    {
        append_field(fields, entity, std::string{EntityStatusFieldName});
    }
    if (entity_has_field(entity, std::string{EntityCreatedAtFieldName}))
    {
        append_field(fields, entity, std::string{EntityCreatedAtFieldName});
    }
    if (entity_has_field(entity, std::string{EntityUpdatedAtFieldName}))
    {
        append_field(fields, entity, std::string{EntityUpdatedAtFieldName});
    }

    return fields;
}

std::vector<std::string> list_selector_fields(
    const SemanticEntity& entity,
    const SemanticEntityApiList& list
)
{
    if (list.by.size() == 1)
    {
        for (const auto& index : entity.indexes)
        {
            if (index.name == list.by[0])
            {
                return index.fields;
            }
        }
    }
    return list.by;
}

void append_entity_api_shapes(
    const SemanticEntity& entity,
    IrSystem& ir
)
{
    if (!entity.api.has_value())
    {
        return;
    }

    const auto entity_name = pascal_identifier(entity.name, "Entity");
    const auto entity_plural = plural_pascal_identifier(entity.name);
    const auto response_name = entity_name + "Response";
    if (!has_shape(ir, response_name))
    {
        ir.shapes.push_back(IrShape{response_name, entity_response_fields(entity)});
    }

    if (entity.api->create.has_value())
    {
        const auto create_name = entity.api->create->name.value_or("Create" + entity_name);
        const auto request_name = create_name + "Request";
        if (!has_shape(ir, request_name))
        {
            std::vector<IrField> fields;
            const auto route_fields = path_parameters(entity.api->resource);
            for (const auto& key_field : entity.key_fields)
            {
                if (std::find(route_fields.begin(), route_fields.end(), key_field) ==
                    route_fields.end())
                {
                    append_field(fields, entity, key_field);
                }
            }
            for (const auto& field : entity.api->create->fields)
            {
                append_field(fields, entity, field);
            }
            ir.shapes.push_back(IrShape{request_name, std::move(fields)});
        }
    }

    if (entity.api->update_status.has_value())
    {
        const auto update_name =
            entity.api->update_status->name.value_or("Update" + entity_name + "Status");
        const auto request_name = update_name + "Request";
        if (!has_shape(ir, request_name))
        {
            std::vector<IrField> fields;
            const auto route_fields = path_parameters(entity.api->resource);
            for (const auto& key_field : entity.key_fields)
            {
                if (std::find(route_fields.begin(), route_fields.end(), key_field) ==
                    route_fields.end())
                {
                    append_field(fields, entity, key_field);
                }
            }
            append_field(fields, entity, std::string{EntityStatusFieldName});
            ir.shapes.push_back(IrShape{request_name, std::move(fields)});
        }
    }

    for (const auto& list : entity.api->lists)
    {
        const auto list_name = list.name.value_or("List" + entity_plural);
        const auto response_shape = list_name + "Response";
        if (has_shape(ir, response_shape))
        {
            continue;
        }

        std::vector<IrField> fields;
        for (const auto& selector_field : list_selector_fields(entity, list))
        {
            append_field(fields, entity, selector_field);
        }
        fields.push_back(
            IrField{lower_camel_identifier(entity_plural, "entities"), response_name + "[]"}
        );
        ir.shapes.push_back(IrShape{response_shape, std::move(fields)});
    }
}

} // namespace

void lower_ir_declarations(
    const SemanticSystem& system,
    IrSystem& ir
)
{
    ir.name = system.name;

    if (system.tenant_scope.has_value())
    {
        ir.tenant_scope = IrTenantScope{system.tenant_scope->field_name};
    }

    if (system.system_tenant.has_value())
    {
        ir.system_tenant =
            IrSystemTenant{system.system_tenant->source, system.system_tenant->config_key};
    }

    for (const auto& value : system.values)
    {
        ir.values.push_back(IrValue{value.name, value.type, value.constraint});
    }

    for (const auto& enum_decl : system.enums)
    {
        IrEnum ir_enum;
        ir_enum.name = enum_decl.name;
        for (const auto& member : enum_decl.members)
        {
            ir_enum.members.push_back(IrEnumMember{member.name, member.value});
        }
        ir.enums.push_back(std::move(ir_enum));
    }

    for (const auto& event : system.events)
    {
        ir.events.push_back(IrEvent{event.name, lower_fields(event.fields)});
    }

    for (const auto& shape : system.shapes)
    {
        ir.shapes.push_back(IrShape{shape.name, lower_fields(shape.fields)});
    }

    for (const auto& entity : system.entities)
    {
        append_entity_api_shapes(entity, ir);
    }
}

} // namespace statespec
