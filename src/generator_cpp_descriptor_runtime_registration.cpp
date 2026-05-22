#include "generator_cpp_descriptor_areas.hpp"

#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string cpp_feature_flag_helpers()
{
    return R"cpp(inline statespec::backend::FeatureFlagType feature_flag_type_from_string(std::string_view type)
{
    if (type == "string") { return statespec::backend::FeatureFlagType::String; }
    if (type == "int") { return statespec::backend::FeatureFlagType::Integer; }
    if (type == "decimal") { return statespec::backend::FeatureFlagType::Decimal; }
    return statespec::backend::FeatureFlagType::Bool;
}

inline statespec::backend::FeatureFlagScopeKind feature_flag_scope_from_string(std::string_view scope)
{
    if (scope == "system") { return statespec::backend::FeatureFlagScopeKind::System; }
    if (scope == "user") { return statespec::backend::FeatureFlagScopeKind::User; }
    if (scope.rfind("entity ", 0) == 0) { return statespec::backend::FeatureFlagScopeKind::Entity; }
    return statespec::backend::FeatureFlagScopeKind::Tenant;
}

inline statespec::backend::FeatureFlagValue feature_flag_value_from_descriptor(const FeatureFlagDefinition& definition)
{
    if (definition.type == "string")
    {
        return statespec::backend::FeatureFlagValue::string_value(definition.default_value);
    }
    if (definition.type == "int")
    {
        return statespec::backend::FeatureFlagValue::integer_value(std::stoll(definition.default_value));
    }
    if (definition.type == "decimal")
    {
        return statespec::backend::FeatureFlagValue::decimal_value(std::stod(definition.default_value));
    }
    return statespec::backend::FeatureFlagValue::bool_value(definition.default_value == "true");
}

)cpp";
}

std::string cpp_lease_helpers()
{
    return R"cpp(inline statespec::backend::LeaseDefinition lease_definition_from_descriptor(const LeaseDefinition& definition)
{
    return statespec::backend::LeaseDefinition{
        statespec::backend::LeaseDefinitionId{definition.name, 1},
        definition.resource.value_or(definition.name),
        definition.ttl,
        definition.renew_every.value_or(definition.ttl),
        definition.max_ttl,
        definition.fencing_token,
    };
}

)cpp";
}

std::string cpp_feature_flag_registration()
{
    return R"cpp(inline void register_feature_flag_definitionsTx(
    statespec::backend::ITransaction& tx,
    statespec::backend::IFeatureFlagStore& store
)
{
    for (const auto& definition : feature_flag_definitions())
    {
        store.register_definitionTx(
            tx,
            statespec::backend::FeatureFlagDefinition{
                definition.name,
                feature_flag_type_from_string(definition.type),
                feature_flag_value_from_descriptor(definition),
                feature_flag_scope_from_string(definition.scope),
                definition.owner,
                definition.description,
                definition.expires,
            }
        );
    }
}

)cpp";
}

std::string cpp_queue_registration()
{
    return R"cpp(inline void register_queue_definitionsTx(
    statespec::backend::ITransaction& tx,
    statespec::backend::IQueueStore& store
)
{
    for (const auto& definition : queue_definitions())
    {
        store.register_definitionTx(
            tx,
            statespec::backend::RegisterQueueDefinitionRequest{definition}
        );
    }
}

)cpp";
}

std::string cpp_lease_registration()
{
    return R"cpp(inline void register_lease_definitionsTx(
    statespec::backend::ITransaction& tx,
    statespec::backend::ILeaseStore& store
)
{
    for (const auto& definition : lease_definitions())
    {
        store.register_definitionTx(tx, lease_definition_from_descriptor(definition));
    }
}

)cpp";
}

std::string cpp_log_registration()
{
    return R"cpp(inline void register_log_definitionsTx(
    statespec::backend::ITransaction& tx,
    statespec::backend::ILogSink& sink
)
{
    for (const auto& definition : log_definitions())
    {
        sink.register_definitionTx(
            tx,
            statespec::backend::LogDefinition{
                definition.name,
                log_level_from_string(definition.level),
                definition.event_name,
                definition.fields,
            }
        );
    }
}

)cpp";
}

std::string cpp_metric_registration()
{
    return R"cpp(inline void register_metric_definitionsTx(
    statespec::backend::ITransaction& tx,
    statespec::backend::IMetricSink& sink
)
{
    for (const auto& definition : metric_definitions())
    {
        sink.register_definitionTx(
            tx,
            statespec::backend::MetricDefinition{
                definition.name,
                metric_kind_from_string(definition.kind),
                definition.backend_name,
                definition.unit,
                definition.labels,
            }
        );
    }
}

)cpp";
}

std::string cpp_workflow_registration()
{
    return R"cpp(inline void register_workflow_definitionsTx(
    statespec::backend::ITransaction& tx,
    statespec::backend::IWorkflowStore& workflow_store
)
{
    for (const auto& definition : workflow_definitions())
    {
        workflow_store.register_definitionTx(
            tx,
            statespec::backend::RegisterWorkflowDefinitionRequest{definition}
        );
    }
}

)cpp";
}

std::string cpp_observability_registration(const RuntimeDomainUsage& usage)
{
    std::ostringstream out;
    if (usage.uses_logs)
    {
        out << cpp_log_registration();
    }
    if (usage.uses_metrics)
    {
        out << cpp_metric_registration();
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        out << R"cpp(inline void register_observability_catalogTx(
    statespec::backend::ITransaction& tx,
    statespec::backend::ILogSink& log_sink,
    statespec::backend::IMetricSink& metric_sink
)
{
    register_log_definitionsTx(tx, log_sink);
    register_metric_definitionsTx(tx, metric_sink);
}

)cpp";
    }
    return out.str();
}

} // namespace

std::string generate_cpp_runtime_registration(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream helpers;
    std::ostringstream registrations;
    std::ostringstream tx_parameters;
    std::ostringstream runtime_parameters;
    std::ostringstream runtime_arguments;
    std::ostringstream tx_calls;

    auto add_domain =
        [&](std::string_view tx_type, std::string_view name, std::string_view register_call)
    {
        tx_parameters << ",\n    " << tx_type << "& " << name;
        runtime_parameters << ",\n    " << tx_type << "& " << name;
        runtime_arguments << ", " << name;
        tx_calls << "    " << register_call << "(tx, " << name << ");\n";
    };

    if (usage.uses_feature_flags)
    {
        helpers << cpp_feature_flag_helpers();
        registrations << cpp_feature_flag_registration();
        add_domain(
            "statespec::backend::IFeatureFlagStore", "feature_flag_store",
            "register_feature_flag_definitionsTx"
        );
    }
    if (usage.uses_queues)
    {
        registrations << cpp_queue_registration();
        add_domain(
            "statespec::backend::IQueueStore", "queue_store", "register_queue_definitionsTx"
        );
    }
    if (usage.uses_leases)
    {
        helpers << cpp_lease_helpers();
        registrations << cpp_lease_registration();
        add_domain(
            "statespec::backend::ILeaseStore", "lease_store", "register_lease_definitionsTx"
        );
    }
    if (usage.uses_workflows)
    {
        registrations << cpp_workflow_registration();
        add_domain(
            "statespec::backend::IWorkflowStore", "workflow_store",
            "register_workflow_definitionsTx"
        );
    }
    registrations << cpp_observability_registration(usage);
    if (usage.uses_logs && usage.uses_metrics)
    {
        tx_parameters << ",\n    statespec::backend::ILogSink& log_sink";
        runtime_parameters << ",\n    statespec::backend::ILogSink& log_sink";
        tx_parameters << ",\n    statespec::backend::IMetricSink& metric_sink";
        runtime_parameters << ",\n    statespec::backend::IMetricSink& metric_sink";
        runtime_arguments << ", log_sink";
        runtime_arguments << ", metric_sink";
        tx_calls << "    register_observability_catalogTx(tx, log_sink, metric_sink);\n";
    }
    else
    {
        if (usage.uses_logs)
        {
            add_domain("statespec::backend::ILogSink", "log_sink", "register_log_definitionsTx");
        }
        if (usage.uses_metrics)
        {
            add_domain(
                "statespec::backend::IMetricSink", "metric_sink", "register_metric_definitionsTx"
            );
        }
    }

    return templates.render(
        "generated/runtime_registration.hpp.tmpl",
        TemplateRenderer::Values{
            {"feature_flag_helpers", helpers.str()},
            {"lease_helpers", {}},
            {"domain_registration_helpers", registrations.str()},
            {"tx_parameters", tx_parameters.str()},
            {"runtime_parameters", runtime_parameters.str()},
            {"runtime_arguments", runtime_arguments.str()},
            {"tx_calls", tx_calls.str()},
        }
    );
}

} // namespace statespec
