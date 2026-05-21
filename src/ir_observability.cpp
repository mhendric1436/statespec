#include "ir_observability.hpp"

#include "ir_lowering_helpers.hpp"

#include <utility>

namespace statespec
{

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
                flag.default_value,
                flag.scope,
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
        ir_log.event_name = log.event_name;
        ir_log.fields = lower_fields(log.fields);
        ir.logs.push_back(std::move(ir_log));
    }

    for (const auto& metric : system.metrics)
    {
        IrMetric ir_metric;
        ir_metric.name = metric.name;
        ir_metric.kind = metric.kind;
        ir_metric.backend_name = metric.backend_name;
        ir_metric.unit = metric.unit;
        ir_metric.labels = lower_fields(metric.labels);
        ir.metrics.push_back(std::move(ir_metric));
    }
}

} // namespace statespec
