#include "validator_declarations.hpp"

#include "validator_helpers.hpp"

#include <unordered_set>

namespace statespec::validator_detail
{

void validate_field_duplicates(
    const std::vector<FieldDecl>& fields,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> names;
    for (const auto& field : fields)
    {
        if (!names.insert(field.name).second)
        {
            duplicate_error(diagnostics, field.range, field.name);
        }
    }
}

void validate_field_types(
    const std::vector<FieldDecl>& fields,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& field : fields)
    {
        const auto base_type = base_type_name(field.type);
        if (!is_builtin_type(base_type) && !symbols.find(base_type).has_value())
        {
            unknown_reference_error(diagnostics, field.range, "type", base_type);
        }
    }
}

bool is_known_type_reference(
    const std::string& type,
    const SymbolTable& symbols
)
{
    const auto base_type = base_type_name(type);
    return is_builtin_type(base_type) || symbols.find(base_type).has_value();
}

void validate_system_tenancy(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    if (!system.tenant_scope.has_value())
    {
        required_error(
            diagnostics, system.range, "system '" + system.name + "'", "tenant scoped_by"
        );
    }
    if (!system.system_tenant.has_value())
    {
        required_error(
            diagnostics, system.range, "system '" + system.name + "'", "system_tenant configured"
        );
    }
}

} // namespace statespec::validator_detail
