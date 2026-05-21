#include "ir_declarations.hpp"

#include "ir_lowering_helpers.hpp"

#include <utility>

namespace statespec
{

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
}

} // namespace statespec
