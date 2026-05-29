#include "ir_observability.hpp"

#include "ir_lowering_helpers.hpp"

#include <utility>

namespace statespec
{

namespace
{

IrFeatureFlagType feature_flag_type_from_string(const std::string& type)
{
    if (type == "string")
    {
        return IrFeatureFlagType::String;
    }
    if (type == "int" || type == "integer")
    {
        return IrFeatureFlagType::Integer;
    }
    if (type == "decimal")
    {
        return IrFeatureFlagType::Decimal;
    }
    return IrFeatureFlagType::Bool;
}

IrFeatureFlagScopeKind feature_flag_scope_from_string(const std::string& scope)
{
    if (scope == "system")
    {
        return IrFeatureFlagScopeKind::System;
    }
    if (scope == "user")
    {
        return IrFeatureFlagScopeKind::User;
    }
    if (scope.rfind("entity ", 0) == 0)
    {
        return IrFeatureFlagScopeKind::Entity;
    }
    return IrFeatureFlagScopeKind::Tenant;
}

IrLogLevel log_level_from_string(const std::string& level)
{
    if (level == "debug")
    {
        return IrLogLevel::Debug;
    }
    if (level == "warn")
    {
        return IrLogLevel::Warn;
    }
    if (level == "error")
    {
        return IrLogLevel::Error;
    }
    return IrLogLevel::Info;
}

IrMetricKind metric_kind_from_string(const std::string& kind)
{
    if (kind == "gauge")
    {
        return IrMetricKind::Gauge;
    }
    if (kind == "histogram")
    {
        return IrMetricKind::Histogram;
    }
    return IrMetricKind::Counter;
}

} // namespace

void lower_ir_observability(
    const SemanticSystem& system,
    IrSystem& ir
)
{
    for (const auto& flag : system.feature_flags)
    {
        ir.feature_flags.push_back(
            IrFeatureFlag{
                flag.name,
                flag.type,
                feature_flag_type_from_string(flag.type),
                flag.default_value,
                flag.scope,
                feature_flag_scope_from_string(flag.scope),
                flag.owner,
                flag.description,
                flag.expires,
            }
        );
    }

    for (const auto& log : system.logs)
    {
        IrLog ir_log;
        ir_log.name = log.name;
        ir_log.level = log.level;
        ir_log.level_kind = log_level_from_string(log.level);
        ir_log.event_name = log.event_name;
        ir_log.fields = lower_fields(log.fields);
        ir.logs.push_back(std::move(ir_log));
    }

    for (const auto& metric : system.metrics)
    {
        IrMetric ir_metric;
        ir_metric.name = metric.name;
        ir_metric.kind = metric.kind;
        ir_metric.kind_value = metric_kind_from_string(metric.kind);
        ir_metric.backend_name = metric.backend_name;
        ir_metric.unit = metric.unit;
        ir_metric.labels = lower_fields(metric.labels);
        ir.metrics.push_back(std::move(ir_metric));
    }
}

} // namespace statespec
