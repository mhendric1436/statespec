#include "generator_crud_contract.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_set>

namespace statespec
{

namespace
{

inline constexpr std::string_view CrudContractDiagnostic = "SSPEC5105";
inline constexpr std::string_view OperationCreate = "create";
inline constexpr std::string_view OperationGet = "get";
inline constexpr std::string_view OperationList = "list";
inline constexpr std::string_view OperationUpdateStatus = "update_status";
inline constexpr std::string_view OperationDelete = "delete";

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

bool has_field(
    const IrEntity& entity,
    const std::string& name
)
{
    return std::any_of(
        entity.fields.begin(), entity.fields.end(),
        [&](const IrField& field) { return field.name == name; }
    );
}

bool is_prefix(
    const std::vector<std::string>& prefix,
    const std::vector<std::string>& values
)
{
    return prefix.size() <= values.size() &&
           std::equal(prefix.begin(), prefix.end(), values.begin());
}

bool selector_is_key_or_index_backed(
    const IrEntity& entity,
    const std::vector<std::string>& selector
)
{
    if (is_prefix(selector, entity.key_fields))
    {
        return true;
    }

    for (const auto& index : entity.indexes)
    {
        if (selector.size() == 1 && selector[0] == index.name)
        {
            return true;
        }
        if (is_prefix(selector, index.fields))
        {
            return true;
        }
    }
    return false;
}

std::unordered_set<std::string> path_parameter_set(const std::optional<std::string>& path)
{
    std::unordered_set<std::string> parameters;
    if (!path.has_value())
    {
        return parameters;
    }

    std::size_t offset = 0;
    while (offset < path->size())
    {
        const auto open = path->find('{', offset);
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
            parameters.insert(path->substr(open + 1, close - open - 1));
        }
        offset = close + 1;
    }
    return parameters;
}

bool has_all_fields(
    const std::unordered_set<std::string>& actual,
    const std::vector<std::string>& required
)
{
    return std::all_of(
        required.begin(), required.end(),
        [&](const std::string& field) { return actual.find(field) != actual.end(); }
    );
}

void crud_contract_error(
    DiagnosticBag& diagnostics,
    const IrApi& api,
    const std::string& message
)
{
    diagnostics.error(
        SourceRange{}, std::string{CrudContractDiagnostic},
        "generated CRUD API '" + api.name + "' " + message
    );
}

bool require_shape(
    const IrSystem& system,
    DiagnosticBag& diagnostics,
    const IrApi& api,
    const std::optional<std::string>& shape_name,
    std::string_view role
)
{
    if (!shape_name.has_value())
    {
        crud_contract_error(diagnostics, api, "requires " + std::string{role} + " shape");
        return false;
    }
    if (find_shape(system, *shape_name) == nullptr)
    {
        crud_contract_error(
            diagnostics, api,
            "references missing " + std::string{role} + " shape '" + *shape_name + "'"
        );
        return false;
    }
    return true;
}

bool validate_key_route(
    DiagnosticBag& diagnostics,
    const IrApi& api,
    const IrEntity& entity
)
{
    const auto route_parameters = path_parameter_set(api.path);
    if (api.path.has_value() && has_all_fields(route_parameters, entity.key_fields))
    {
        return true;
    }
    crud_contract_error(diagnostics, api, "route path must include every entity key field");
    return false;
}

bool validate_list_selector(
    DiagnosticBag& diagnostics,
    const IrApi& api,
    const IrEntity& entity
)
{
    bool valid = true;
    if (api.list_selector.empty())
    {
        crud_contract_error(diagnostics, api, "requires a non-empty list selector");
        return false;
    }

    for (const auto& field : api.list_selector)
    {
        if (!has_field(entity, field))
        {
            crud_contract_error(
                diagnostics, api, "list selector references unknown entity field '" + field + "'"
            );
            valid = false;
        }
    }

    if (!selector_is_key_or_index_backed(entity, api.list_selector))
    {
        crud_contract_error(
            diagnostics, api,
            "list selector must be an entity key prefix, index name, or index field prefix"
        );
        valid = false;
    }

    const auto route_parameters = path_parameter_set(api.path);
    if (!has_all_fields(route_parameters, api.list_selector))
    {
        crud_contract_error(diagnostics, api, "route path must include every list selector field");
        valid = false;
    }
    return valid;
}

bool validate_crud_api(
    const IrSystem& system,
    DiagnosticBag& diagnostics,
    const IrApi& api
)
{
    bool valid = true;
    if (api.entity.has_value() != api.repository_operation.has_value())
    {
        crud_contract_error(
            diagnostics, api,
            "requires both api.entity and api.repository_operation metadata or neither"
        );
        return false;
    }
    if (!api.entity.has_value())
    {
        return true;
    }

    const auto* entity = find_entity(system, *api.entity);
    if (entity == nullptr)
    {
        crud_contract_error(diagnostics, api, "references unknown entity '" + *api.entity + "'");
        return false;
    }

    if (!api.method.has_value())
    {
        crud_contract_error(diagnostics, api, "requires an HTTP method");
        valid = false;
    }
    if (!api.path.has_value())
    {
        crud_contract_error(diagnostics, api, "requires a route path");
        valid = false;
    }

    const auto& operation = *api.repository_operation;
    if (operation == OperationCreate)
    {
        valid = require_shape(system, diagnostics, api, api.input, "request") && valid;
        valid = require_shape(system, diagnostics, api, api.output, "response") && valid;
        valid = validate_key_route(diagnostics, api, *entity) && valid;
        if (api.method.value_or(std::string{}) != "POST")
        {
            crud_contract_error(diagnostics, api, "create operation must use POST");
            valid = false;
        }
    }
    else if (operation == OperationGet)
    {
        valid = require_shape(system, diagnostics, api, api.output, "response") && valid;
        valid = validate_key_route(diagnostics, api, *entity) && valid;
        if (api.method.value_or(std::string{}) != "GET")
        {
            crud_contract_error(diagnostics, api, "get operation must use GET");
            valid = false;
        }
    }
    else if (operation == OperationList)
    {
        valid = require_shape(system, diagnostics, api, api.output, "response") && valid;
        valid = validate_list_selector(diagnostics, api, *entity) && valid;
        if (api.method.value_or(std::string{}) != "GET")
        {
            crud_contract_error(diagnostics, api, "list operation must use GET");
            valid = false;
        }
    }
    else if (operation == OperationUpdateStatus)
    {
        valid = require_shape(system, diagnostics, api, api.input, "request") && valid;
        valid = require_shape(system, diagnostics, api, api.output, "response") && valid;
        valid = validate_key_route(diagnostics, api, *entity) && valid;
        if (api.method.value_or(std::string{}) != "PATCH")
        {
            crud_contract_error(diagnostics, api, "update_status operation must use PATCH");
            valid = false;
        }
    }
    else if (operation == OperationDelete)
    {
        valid = validate_key_route(diagnostics, api, *entity) && valid;
        if (api.method.value_or(std::string{}) != "DELETE")
        {
            crud_contract_error(diagnostics, api, "delete operation must use DELETE");
            valid = false;
        }
    }
    else
    {
        crud_contract_error(
            diagnostics, api, "uses unsupported repository operation '" + operation + "'"
        );
        valid = false;
    }

    return valid;
}

} // namespace

bool validate_crud_handler_inputs(
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    bool valid = true;
    for (const auto& api : system.apis)
    {
        valid = validate_crud_api(system, diagnostics, api) && valid;
    }
    return valid;
}

} // namespace statespec
