#include "validator_declarations.hpp"

#include "validator_helpers.hpp"

#include <unordered_set>

namespace statespec::validator_detail
{

void validate_shapes(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& shape : system.shapes)
    {
        if (!is_qualified_pascal_case_name(shape.name))
        {
            diagnostics.error(
                shape.range, diagnostic_codes::ShapeInvalidName,
                "shape '" + shape.name + "'" + diagnostic_fragments::MustUsePascalCase
            );
        }
        if (shape.fields.empty())
        {
            required_error(diagnostics, shape.range, "shape '" + shape.name + "'", "fields");
        }
        validate_field_duplicates(shape.fields, diagnostics);
        validate_field_types(shape.fields, symbols, diagnostics);
    }
}

void validate_values(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& value : system.values)
    {
        if (!is_qualified_pascal_case_name(value.name))
        {
            diagnostics.error(
                value.range, diagnostic_codes::ValueInvalidName,
                "value '" + value.name + "'" + diagnostic_fragments::MustUsePascalCase
            );
        }
        if (value.type.empty())
        {
            required_error(diagnostics, value.range, "value '" + value.name + "'", "type");
        }
        else if (!is_known_type_reference(value.type, symbols))
        {
            unknown_reference_error(diagnostics, value.range, "value type", value.type);
        }
        if (value.constraint.has_value())
        {
            validate_expression(system, value.range, *value.constraint, diagnostics);
        }
    }
}

void validate_enums(
    DiagnosticBag& diagnostics,
    const SystemDecl& system
)
{
    for (const auto& enum_decl : system.enums)
    {
        if (!is_qualified_pascal_case_name(enum_decl.name))
        {
            diagnostics.error(
                enum_decl.range, diagnostic_codes::EnumInvalidName,
                "enum '" + enum_decl.name + "'" + diagnostic_fragments::MustUsePascalCase
            );
        }
        if (enum_decl.members.empty())
        {
            required_error(diagnostics, enum_decl.range, "enum '" + enum_decl.name + "'", "member");
        }

        std::unordered_set<std::string> members;
        for (const auto& member : enum_decl.members)
        {
            if (!is_pascal_case_name(member.name))
            {
                diagnostics.error(
                    member.range, diagnostic_codes::EnumInvalidMemberName,
                    "enum member '" + enum_decl.name + "." + member.name + "'" +
                        diagnostic_fragments::MustUsePascalCase
                );
            }
            if (!members.insert(member.name).second)
            {
                duplicate_error(diagnostics, member.range, enum_decl.name + "." + member.name);
            }
        }
    }
}

void validate_events(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& event : system.events)
    {
        if (!is_qualified_pascal_case_name(event.name))
        {
            diagnostics.error(
                event.range, diagnostic_codes::EventInvalidName,
                "event '" + event.name + "'" + diagnostic_fragments::MustUsePascalCase
            );
        }
        if (event.fields.empty())
        {
            required_error(diagnostics, event.range, "event '" + event.name + "'", "fields");
        }
        validate_field_duplicates(event.fields, diagnostics);
        validate_field_types(event.fields, symbols, diagnostics);
    }
}
} // namespace statespec::validator_detail
