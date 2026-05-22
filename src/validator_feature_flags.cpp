#include "validator_declarations.hpp"

#include "validator_helpers.hpp"

namespace statespec::validator_detail
{

void validate_feature_flags(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& flag : system.feature_flags)
    {
        if (!is_qualified_pascal_case_name(flag.name))
        {
            diagnostics.error(
                flag.range, diagnostic_codes::FeatureFlagInvalidName,
                "feature flag '" + flag.name + "'" + diagnostic_fragments::MustUsePascalCase
            );
        }

        if (!flag.type.has_value())
        {
            required_error(diagnostics, flag.range, "feature_flag '" + flag.name + "'", "type");
        }
        else if (!is_supported_feature_flag_type(*flag.type))
        {
            diagnostics.error(
                flag.range, diagnostic_codes::FeatureFlagInvalidType,
                "feature flag '" + flag.name + "' has unsupported type '" + *flag.type + "'"
            );
        }

        if (!flag.default_value.has_value())
        {
            required_error(diagnostics, flag.range, "feature_flag '" + flag.name + "'", "default");
        }
        else if (!feature_flag_default_matches_type(flag))
        {
            diagnostics.error(
                flag.range, diagnostic_codes::FeatureFlagInvalidDefault,
                "feature flag '" + flag.name + "' default must match declared type"
            );
        }

        if (flag.scope.has_value())
        {
            const auto entity = entity_scope_target(*flag.scope);
            if (entity.has_value())
            {
                const auto symbol = symbols.find(*entity);
                if (!symbol.has_value() || symbol->kind != SymbolKind::Entity)
                {
                    unknown_reference_error(
                        diagnostics, flag.range, "feature flag entity scope", *entity
                    );
                }
            }
        }
    }
}
} // namespace statespec::validator_detail
