#include "semantic_observability.hpp"

#include "semantic_symbols.hpp"

#include <string_view>

namespace statespec
{

namespace
{

std::optional<SemanticReference> resolve_feature_flag_scope_target(
    const SymbolTable& symbols,
    const std::optional<std::string>& scope
)
{
    if (!scope.has_value())
    {
        return std::nullopt;
    }

    constexpr std::string_view prefix{"entity "};
    if (scope->rfind(std::string{prefix}, 0) != 0)
    {
        return std::nullopt;
    }

    return resolve_reference(symbols, scope->substr(prefix.size()));
}

} // namespace

void resolve_semantic_observability(
    const SystemDecl& system,
    const SymbolTable& symbols,
    SemanticSystem& resolved
)
{
    for (const auto& flag : system.feature_flags)
    {
        resolved.feature_flags.push_back(
            SemanticFeatureFlag{
                flag.name,
                flag.type.value_or(""),
                flag.default_value.value_or(""),
                flag.scope.value_or("system"),
                resolve_feature_flag_scope_target(symbols, flag.scope),
                flag.owner,
                flag.description,
                flag.expires,
            }
        );
    }

    for (const auto& log : system.logs)
    {
        resolved.logs.push_back(
            SemanticLog{
                log.name,
                log.level.value_or(""),
                log.event_name.value_or(""),
                resolve_fields(log.fields),
            }
        );
    }

    for (const auto& metric : system.metrics)
    {
        resolved.metrics.push_back(
            SemanticMetric{
                metric.name,
                metric.kind.value_or(""),
                metric.backend_name.value_or(""),
                metric.unit.value_or(""),
                resolve_fields(metric.labels),
            }
        );
    }
}

} // namespace statespec
