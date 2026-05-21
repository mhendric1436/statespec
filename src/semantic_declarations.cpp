#include "semantic_declarations.hpp"

#include "semantic_symbols.hpp"

#include <utility>

namespace statespec
{

void resolve_semantic_declarations(
    const SystemDecl& system,
    SemanticSystem& resolved
)
{
    resolved.name = system.name;
    if (system.tenant_scope.has_value())
    {
        resolved.tenant_scope = SemanticTenantScope{system.tenant_scope->field_name};
    }
    if (system.system_tenant.has_value())
    {
        resolved.system_tenant =
            SemanticSystemTenant{"configured", system.system_tenant->config_key};
    }

    for (const auto& shape : system.shapes)
    {
        resolved.shapes.push_back(SemanticShape{shape.name, resolve_fields(shape.fields)});
    }

    for (const auto& value : system.values)
    {
        resolved.values.push_back(SemanticValue{value.name, value.type, value.constraint});
    }

    for (const auto& enum_decl : system.enums)
    {
        SemanticEnum resolved_enum;
        resolved_enum.name = enum_decl.name;
        for (const auto& member : enum_decl.members)
        {
            resolved_enum.members.push_back(SemanticEnumMember{member.name, member.value});
        }
        resolved.enums.push_back(std::move(resolved_enum));
    }

    for (const auto& event : system.events)
    {
        resolved.events.push_back(SemanticEvent{event.name, resolve_fields(event.fields)});
    }
}

} // namespace statespec
