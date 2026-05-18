#include "validator_declarations.hpp"

#include "validator_helpers.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace statespec::validator_detail
{

namespace
{

const FeatureFlagDecl* find_feature_flag(
    const SystemDecl& system,
    const std::string& name
)
{
    for (const auto& flag : system.feature_flags)
    {
        if (flag.name == name)
        {
            return &flag;
        }
    }
    return nullptr;
}

int policy_member_order_index(const std::string& kind)
{
    if (kind == "tenant")
    {
        return 0;
    }
    if (kind == "allow")
    {
        return 1;
    }
    if (kind == "deny")
    {
        return 2;
    }
    if (kind == "quota")
    {
        return 3;
    }
    if (kind == "audit")
    {
        return 4;
    }
    return 5;
}

void validate_policy_member_order(
    const PolicyDecl& policy,
    DiagnosticBag& diagnostics
)
{
    int previous_order = -1;
    for (const auto& member : policy.member_order)
    {
        const auto order = policy_member_order_index(member.kind);
        if (order < previous_order)
        {
            diagnostics.warning(
                member.range, "SSPEC6103",
                "policy '" + policy.name +
                    "' members should use canonical order: tenant, allow, deny, quota, audit"
            );
            return;
        }
        previous_order = order;
    }
}

std::vector<std::string> feature_flag_function_arguments(
    const std::string& expression,
    const std::string& function_name
)
{
    std::vector<std::string> arguments;
    std::size_t offset = 0;

    while ((offset = expression.find(function_name, offset)) != std::string::npos)
    {
        auto cursor = offset + function_name.size();
        while (cursor < expression.size() &&
               std::isspace(static_cast<unsigned char>(expression[cursor])) != 0)
        {
            ++cursor;
        }
        if (cursor >= expression.size() || expression[cursor] != '(')
        {
            offset = cursor;
            continue;
        }

        const auto arg_start = cursor + 1;
        const auto arg_end = expression.find(')', arg_start);
        if (arg_end == std::string::npos)
        {
            break;
        }

        auto argument = expression.substr(arg_start, arg_end - arg_start);
        argument.erase(
            std::remove_if(
                argument.begin(), argument.end(),
                [](char ch) { return std::isspace(static_cast<unsigned char>(ch)) != 0; }
            ),
            argument.end()
        );
        arguments.push_back(argument);
        offset = arg_end + 1;
    }

    return arguments;
}

} // namespace

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

void validate_feature_flag_expression(
    const SystemDecl& system,
    const SourceRange& range,
    const std::string& expression,
    DiagnosticBag& diagnostics
)
{
    for (const auto& flag_name : feature_flag_function_arguments(expression, "feature_enabled"))
    {
        const auto* flag = find_feature_flag(system, flag_name);
        if (flag == nullptr)
        {
            unknown_reference_error(diagnostics, range, "feature flag", flag_name);
            continue;
        }
        if (flag->type.value_or("") != "bool")
        {
            diagnostics.error(
                range, "SSPEC4204", "feature_enabled requires bool feature flag '" + flag_name + "'"
            );
        }
    }

    for (const auto& flag_name : feature_flag_function_arguments(expression, "feature_value"))
    {
        if (find_feature_flag(system, flag_name) == nullptr)
        {
            unknown_reference_error(diagnostics, range, "feature flag", flag_name);
        }
    }
}

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
                shape.range, "SSPEC3201", "shape '" + shape.name + "' must use PascalCase"
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
                value.range, "SSPEC4501", "value '" + value.name + "' must use PascalCase"
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
            validate_feature_flag_expression(system, value.range, *value.constraint, diagnostics);
        }
    }
}

void validate_namespaces(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& namespace_decl : system.namespaces)
    {
        if (!is_qualified_pascal_case_name(namespace_decl.name))
        {
            diagnostics.error(
                namespace_decl.range, "SSPEC4801",
                "namespace '" + namespace_decl.name + "' must use PascalCase segments"
            );
        }
        for (const auto& member : namespace_decl.members)
        {
            if (!symbols.find(member).has_value())
            {
                unknown_reference_error(
                    diagnostics, namespace_decl.range, "namespace member", member
                );
            }
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
                enum_decl.range, "SSPEC4601", "enum '" + enum_decl.name + "' must use PascalCase"
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
                    member.range, "SSPEC4602",
                    "enum member '" + enum_decl.name + "." + member.name + "' must use PascalCase"
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
                event.range, "SSPEC4701", "event '" + event.name + "' must use PascalCase"
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

void validate_external_systems(
    const SystemDecl& system,
    DiagnosticBag& diagnostics
)
{
    for (const auto& external_system : system.external_systems)
    {
        if (!is_qualified_pascal_case_name(external_system.name))
        {
            diagnostics.error(
                external_system.range, "SSPEC4901",
                "external_system '" + external_system.name + "' must use PascalCase segments"
            );
        }

        std::unordered_set<std::string> properties;
        for (const auto& property : external_system.properties)
        {
            if (!properties.insert(property.name).second)
            {
                duplicate_error(diagnostics, property.range, property.name);
            }
        }
    }
}

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
                flag.range, "SSPEC4201", "feature flag '" + flag.name + "' must use PascalCase"
            );
        }

        if (!flag.type.has_value())
        {
            required_error(diagnostics, flag.range, "feature_flag '" + flag.name + "'", "type");
        }
        else if (!is_supported_feature_flag_type(*flag.type))
        {
            diagnostics.error(
                flag.range, "SSPEC4202",
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
                flag.range, "SSPEC4203",
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

void validate_logs(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    std::unordered_set<std::string> event_names;
    for (const auto& log : system.logs)
    {
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
        }
    }
}

void validate_policies(
    const SystemDecl& system,
    const SymbolTable& symbols,
    DiagnosticBag& diagnostics
)
{
    for (const auto& policy : system.policies)
    {
        validate_policy_member_order(policy, diagnostics);

        if (!policy.tenant_scoped_by.has_value())
        {
            required_error(
                diagnostics, policy.range, "policy '" + policy.name + "'", "tenant scoped_by"
            );
        }
        if (policy.tenant_scoped_by.has_value() && system.tenant_scope.has_value() &&
            *policy.tenant_scoped_by != system.tenant_scope->field_name)
        {
            diagnostics.error(
                policy.range, "SSPEC3405",
                "policy '" + policy.name + "' tenant field '" + *policy.tenant_scoped_by +
                    "' must match system tenant field '" + system.tenant_scope->field_name + "'"
            );
        }
        if (policy.allows.empty() && policy.denies.empty() && policy.quotas.empty() &&
            policy.audits.empty())
        {
            required_error(
                diagnostics, policy.range, "policy '" + policy.name + "'",
                "at least one allow, deny, quota, or audit declaration"
            );
        }

        for (const auto& rule : policy.allows)
        {
            if (!symbols.find(rule.action).has_value())
            {
                unknown_reference_error(
                    diagnostics, rule.range, "policy allow action", rule.action
                );
            }
            validate_feature_flag_expression(system, rule.range, rule.condition, diagnostics);
        }
        for (const auto& rule : policy.denies)
        {
            if (!symbols.find(rule.action).has_value())
            {
                unknown_reference_error(diagnostics, rule.range, "policy deny action", rule.action);
            }
            validate_feature_flag_expression(system, rule.range, rule.condition, diagnostics);
        }
        for (const auto& quota : policy.quotas)
        {
            validate_feature_flag_expression(system, quota.range, quota.expression, diagnostics);
        }
        for (const auto& audit : policy.audits)
        {
            if (!symbols.find(audit).has_value())
            {
                unknown_reference_error(diagnostics, policy.range, "policy audit action", audit);
            }
        }
    }
}

} // namespace statespec::validator_detail
