#include "validator_runtime.hpp"

#include "validator_declarations.hpp"
#include "validator_helpers.hpp"

#include <cctype>
#include <string>

namespace statespec::validator_detail
{

namespace
{

const ShapeDecl* find_shape(
    const SystemDecl& system,
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

void validate_api_input_tenant_field(
    const SystemDecl& system,
    const ApiDecl& api,
    DiagnosticBag& diagnostics
)
{
    if (!system.tenant_scope.has_value() || !api.input.has_value())
    {
        return;
    }

    const auto* input_shape = find_shape(system, *api.input);
    if (input_shape == nullptr)
    {
        return;
    }

    const auto fields = field_names(input_shape->fields);
    const auto& tenant_field = system.tenant_scope->field_name;
    if (!contains(fields, tenant_field))
    {
        diagnostics.error(
            input_shape->range, diagnostic_codes::TenantApiInputMissingTenantField,
            "API '" + api.name + "' input shape '" + input_shape->name +
                "' must declare tenant field '" + tenant_field + "'"
        );
    }
}

bool is_mutating_method(const std::string& method)
{
    return method == "POST" || method == "PUT" || method == "PATCH";
}

std::string lower_camel_field_name(const std::string& field_name)
{
    std::string result;
    bool uppercase_next = false;
    for (const auto ch : field_name)
    {
        if (ch == '_')
        {
            uppercase_next = true;
            continue;
        }
        if (uppercase_next)
        {
            result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            uppercase_next = false;
            continue;
        }
        result.push_back(ch);
    }
    return result;
}

void validate_api_tenant_path(
    const SystemDecl& system,
    const ApiDecl& api,
    DiagnosticBag& diagnostics
)
{
    if (!system.tenant_scope.has_value() || !api.path.has_value())
    {
        return;
    }

    const auto& tenant_field = system.tenant_scope->field_name;
    const auto lower_camel_tenant_field = lower_camel_field_name(tenant_field);
    const auto& path = *api.path;
    if (path.find("/tenants/") == std::string::npos &&
        path.find("{" + tenant_field + "}") == std::string::npos &&
        path.find("{" + lower_camel_tenant_field + "}") == std::string::npos)
    {
        diagnostics.error(
            api.range, diagnostic_codes::TenantApiPathMissingTenantIdentity,
            "API '" + api.name + "' path must include tenant identity for tenant field '" +
                tenant_field + "'"
        );
    }
}
} // namespace

void validate_apis(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& api : system.apis)
    {
        if (!api.method.has_value())
        {
            required_error(diagnostics, api.range, "api '" + api.name + "'", "method");
        }
        if (!api.path.has_value())
        {
            required_error(diagnostics, api.range, "api '" + api.name + "'", "path");
        }
        validate_api_tenant_path(system, api, diagnostics);
        if (api.method.has_value() && is_mutating_method(*api.method) && !api.input.has_value())
        {
            required_error(diagnostics, api.range, "api '" + api.name + "'", "input");
        }
        if (api.input.has_value() && !is_known_type_reference(*api.input, symbols))
        {
            unknown_reference_error(diagnostics, api.range, "API input", *api.input);
        }
        validate_api_input_tenant_field(system, api, diagnostics);
        if (api.method.has_value() && *api.method != "DELETE" && !api.output.has_value())
        {
            required_error(diagnostics, api.range, "api '" + api.name + "'", "output");
        }
        if (api.output.has_value() && !is_known_type_reference(*api.output, symbols))
        {
            unknown_reference_error(diagnostics, api.range, "API output", *api.output);
        }
        if (api.error.has_value() && !is_known_type_reference(*api.error, symbols))
        {
            unknown_reference_error(diagnostics, api.range, "API error", *api.error);
        }
        if (api.starts_workflow.has_value() && api.enqueues.has_value())
        {
            diagnostics.error(
                api.range, diagnostic_codes::ApiMultiplePrimaryActions,
                "api '" + api.name + "' must choose one primary action: starts workflow or enqueues"
            );
        }
        if (api.starts_workflow.has_value() && !symbols.find(*api.starts_workflow).has_value())
        {
            unknown_reference_error(
                diagnostics, api.range, "API starts workflow", *api.starts_workflow
            );
        }
        if (api.enqueues.has_value() && !symbols.find(*api.enqueues).has_value())
        {
            unknown_reference_error(diagnostics, api.range, "API enqueues target", *api.enqueues);
        }
    }
}
} // namespace statespec::validator_detail
