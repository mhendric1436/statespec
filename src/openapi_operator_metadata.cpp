#include "openapi_operator_metadata.hpp"

#include "openapi_paths.hpp"
#include "statespec/language_constants.hpp"
#include "type_syntax.hpp"

#include <algorithm>

namespace statespec
{

namespace
{

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

bool has_shape(
    const IrSystem& system,
    const std::string& name
)
{
    for (const auto& shape : system.shapes)
    {
        if (shape.name == name)
        {
            return true;
        }
    }
    return false;
}

bool contains(
    const std::vector<std::string>& values,
    const std::string& value
)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

bool is_foundational_entity_field(const std::string& name)
{
    return name == EntityCreatedAtFieldName || name == EntityUpdatedAtFieldName ||
           name == EntityStatusFieldName;
}

std::string operator_metadata_base_path(const IrExternalSystemMetadata& metadata)
{
    std::string path;
    if (metadata.tenant_field.has_value())
    {
        path += "/v1/tenants/{" + *metadata.tenant_field + "}";
    }
    else
    {
        path += "/v1";
    }
    path += "/operators/external-systems/{external_system_id}/profiles/{";
    path += metadata.profile_field;
    path += "}";
    return path;
}

void append_operator_metadata_shapes(
    IrSystem& system,
    const IrExternalSystemMetadata& metadata
)
{
    const auto* entity = find_entity(system, metadata.entity);
    if (entity == nullptr)
    {
        return;
    }

    const auto request_name = metadata.entity + "Request";
    if (!has_shape(system, request_name))
    {
        IrShape request_shape;
        request_shape.name = request_name;
        for (const auto& field : entity->fields)
        {
            if (!contains(entity->key_fields, field.name) &&
                !is_foundational_entity_field(field.name))
            {
                auto request_field = field;
                if (!contains(metadata.required_fields, field.name) &&
                    !is_optional_type(request_field.type))
                {
                    request_field.type = "optional<" + request_field.type + ">";
                }
                request_shape.fields.push_back(request_field);
            }
        }
        system.shapes.push_back(request_shape);
    }

    const auto response_name = metadata.entity + "Response";
    if (!has_shape(system, response_name))
    {
        IrShape response_shape;
        response_shape.name = response_name;
        response_shape.fields = entity->fields;
        system.shapes.push_back(response_shape);
    }
}

void append_operator_metadata_apis(
    IrSystem& system,
    const IrExternalSystemMetadata& metadata
)
{
    const auto base_path = operator_metadata_base_path(metadata);
    const auto request_name = metadata.entity + "Request";
    const auto response_name = metadata.entity + "Response";

    if (!openapi_has_api_operation(system, "PUT", base_path))
    {
        system.apis.push_back(
            IrApi{
                "Upsert" + metadata.entity,
                std::string{"PUT"},
                base_path,
                request_name,
                response_name,
                std::nullopt,
                std::nullopt,
                std::nullopt,
            }
        );
    }
    if (!openapi_has_api_operation(system, "GET", base_path))
    {
        system.apis.push_back(
            IrApi{
                "Get" + metadata.entity,
                std::string{"GET"},
                base_path,
                std::nullopt,
                response_name,
                std::nullopt,
                std::nullopt,
                std::nullopt,
            }
        );
    }
    if (!openapi_has_api_operation(system, "POST", base_path + "/disable") &&
        !openapi_has_api_operation(system, "PATCH", base_path + "/disable"))
    {
        system.apis.push_back(
            IrApi{
                "Disable" + metadata.entity,
                std::string{"POST"},
                base_path + "/disable",
                std::nullopt,
                response_name,
                std::nullopt,
                std::nullopt,
                std::nullopt,
            }
        );
    }
    if (!openapi_has_api_operation(system, "DELETE", base_path))
    {
        system.apis.push_back(
            IrApi{
                "Delete" + metadata.entity,
                std::string{"DELETE"},
                base_path,
                std::nullopt,
                response_name,
                std::nullopt,
                std::nullopt,
                std::nullopt,
            }
        );
    }
}

} // namespace

IrSystem with_generated_operator_metadata_openapi(IrSystem system)
{
    for (const auto& external_system : system.external_systems)
    {
        if (external_system.metadata.has_value())
        {
            append_operator_metadata_shapes(system, *external_system.metadata);
            append_operator_metadata_apis(system, *external_system.metadata);
        }
    }
    return system;
}

} // namespace statespec
