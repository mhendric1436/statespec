#include "validator_declarations.hpp"

#include "statespec/language_constants.hpp"
#include "string_utils.hpp"
#include "validator_helpers.hpp"

#include <unordered_set>

namespace statespec::validator_detail
{

namespace
{

int log_member_order_index(const std::string& kind)
{
    if (kind == "level")
    {
        return 0;
    }
    if (kind == "event_name")
    {
        return 1;
    }
    if (kind == "fields")
    {
        return 2;
    }
    return 3;
}

int metric_member_order_index(const std::string& kind)
{
    if (kind == "kind")
    {
        return 0;
    }
    if (kind == "name")
    {
        return 1;
    }
    if (kind == "unit")
    {
        return 2;
    }
    if (kind == "labels")
    {
        return 3;
    }
    return 4;
}

void validate_log_member_order(
    const LogDecl& log,
    DiagnosticBag& diagnostics
)
{
    int previous_order = -1;
    for (const auto& member : log.member_order)
    {
        const auto order = log_member_order_index(member.kind);
        if (order < previous_order)
        {
            diagnostics.warning(
                member.range, "SSPEC6104",
                "log '" + log.name +
                    "' members should use canonical order: level, event_name, fields"
            );
            return;
        }
        previous_order = order;
    }
}

void validate_metric_member_order(
    const MetricDecl& metric,
    DiagnosticBag& diagnostics
)
{
    int previous_order = -1;
    for (const auto& member : metric.member_order)
    {
        const auto order = metric_member_order_index(member.kind);
        if (order < previous_order)
        {
            diagnostics.warning(
                member.range, "SSPEC6105",
                "metric '" + metric.name +
                    "' members should use canonical order: kind, name, unit, labels"
            );
            return;
        }
        previous_order = order;
    }
}

bool is_high_cardinality_metric_label_name(
    const SystemDecl& system,
    const std::string& label_name
)
{
    if (label_name == DefaultTenantIdFieldName)
    {
        return false;
    }
    if (system.tenant_scope.has_value() && label_name == system.tenant_scope->field_name)
    {
        return false;
    }

    const auto name = lower_ascii(label_name);
    return name == "id" || name == "uuid" || name == "guid" || ends_with(name, "_id") ||
           ends_with(name, "_ids") || ends_with(name, "_uuid") || ends_with(name, "_guid") ||
           ends_with(name, "_at") || ends_with(name, "_time") || ends_with(name, "_timestamp") ||
           name.find("payload") != std::string::npos ||
           name.find("error_message") != std::string::npos;
}

void validate_log_tenant_field(
    const SystemDecl& system,
    const LogDecl& log,
    DiagnosticBag& diagnostics
)
{
    if (!system.tenant_scope.has_value())
    {
        return;
    }

    const auto fields = field_names(log.fields);
    if (!contains(fields, system.tenant_scope->field_name))
    {
        diagnostics.error(
            log.range, "SSPEC3407",
            "log '" + log.name + "' fields must declare tenant field '" +
                system.tenant_scope->field_name + "'"
        );
    }
}

void validate_metric_tenant_label(
    const SystemDecl& system,
    const MetricDecl& metric,
    DiagnosticBag& diagnostics
)
{
    if (!system.tenant_scope.has_value())
    {
        return;
    }

    const auto labels = field_names(metric.labels);
    if (!contains(labels, system.tenant_scope->field_name))
    {
        diagnostics.error(
            metric.range, "SSPEC3408",
            "metric '" + metric.name + "' labels must declare tenant field '" +
                system.tenant_scope->field_name + "'"
        );
    }
}
} // namespace

void validate_logs(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> event_names;
    for (const auto& log : system.logs)
    {
        validate_log_member_order(log, diagnostics);

        if (!is_qualified_pascal_case_name(log.name))
        {
            diagnostics.error(log.range, "SSPEC4301", "log '" + log.name + "' must use PascalCase");
        }

        if (!log.level.has_value())
        {
            required_error(diagnostics, log.range, "log '" + log.name + "'", "level");
        }
        else if (!is_supported_log_level(*log.level))
        {
            diagnostics.error(
                log.range, "SSPEC4302",
                "log '" + log.name + "' has unsupported level '" + *log.level + "'"
            );
        }

        if (!log.event_name.has_value())
        {
            required_error(diagnostics, log.range, "log '" + log.name + "'", "event_name");
        }
        else if (!event_names.insert(*log.event_name).second)
        {
            diagnostics.error(
                log.range, "SSPEC4303", "duplicate log event_name '" + *log.event_name + "'"
            );
        }

        validate_field_duplicates(log.fields, diagnostics);
        validate_field_types(log.fields, symbols, diagnostics);
        validate_log_tenant_field(system, log, diagnostics);
    }
}

void validate_metrics(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> backend_names;
    for (const auto& metric : system.metrics)
    {
        validate_metric_member_order(metric, diagnostics);

        if (!is_qualified_pascal_case_name(metric.name))
        {
            diagnostics.error(
                metric.range, "SSPEC4401", "metric '" + metric.name + "' must use PascalCase"
            );
        }

        if (!metric.kind.has_value())
        {
            required_error(diagnostics, metric.range, "metric '" + metric.name + "'", "kind");
        }
        else if (!is_supported_metric_kind(*metric.kind))
        {
            diagnostics.error(
                metric.range, "SSPEC4402",
                "metric '" + metric.name + "' has unsupported kind '" + *metric.kind + "'"
            );
        }

        if (!metric.backend_name.has_value())
        {
            required_error(diagnostics, metric.range, "metric '" + metric.name + "'", "name");
        }
        else if (!backend_names.insert(*metric.backend_name).second)
        {
            diagnostics.error(
                metric.range, "SSPEC4403", "duplicate metric name '" + *metric.backend_name + "'"
            );
        }

        if (!metric.unit.has_value())
        {
            required_error(diagnostics, metric.range, "metric '" + metric.name + "'", "unit");
        }

        validate_field_duplicates(metric.labels, diagnostics);
        validate_field_types(metric.labels, symbols, diagnostics);
        validate_metric_tenant_label(system, metric, diagnostics);
        for (const auto& label : metric.labels)
        {
            if (!is_supported_metric_label_type(label.type))
            {
                diagnostics.error(
                    label.range, "SSPEC4404",
                    "metric '" + metric.name + "' label '" + label.name +
                        "' must use low-cardinality type string, bool, or int"
                );
            }
            if (is_high_cardinality_metric_label_name(system, label.name))
            {
                diagnostics.error(
                    label.range, "SSPEC4405",
                    "metric '" + metric.name + "' label '" + label.name +
                        "' looks high-cardinality; use a log field or aggregate dimension instead"
                );
            }
        }
    }
}
} // namespace statespec::validator_detail
