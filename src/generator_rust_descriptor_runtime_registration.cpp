#include "generator_rust_descriptor_areas.hpp"

#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string rust_feature_flag_helpers()
{
    return R"rs(fn feature_flag_type_from_descriptor(flag_type: &str) -> FeatureFlagType {
    match flag_type {
        "string" => FeatureFlagType::String,
        "int" => FeatureFlagType::Integer,
        "decimal" => FeatureFlagType::Decimal,
        _ => FeatureFlagType::Bool,
    }
}

fn feature_flag_scope_from_descriptor(scope: &str) -> FeatureFlagScopeKind {
    match scope {
        "system" => FeatureFlagScopeKind::System,
        "user" => FeatureFlagScopeKind::User,
        _ if scope.starts_with("entity ") => FeatureFlagScopeKind::Entity,
        _ => FeatureFlagScopeKind::Tenant,
    }
}

fn feature_flag_value_from_descriptor(definition: &FeatureFlagDefinition) -> FeatureFlagValue {
    match definition.flag_type.as_str() {
        "string" => FeatureFlagValue::String(definition.default_value.clone()),
        "int" => FeatureFlagValue::Integer(definition.default_value.parse().unwrap_or(0)),
        "decimal" => FeatureFlagValue::Decimal(definition.default_value.parse().unwrap_or(0.0)),
        _ => FeatureFlagValue::Bool(definition.default_value == "true"),
    }
}

)rs";
}

std::string rust_lease_helpers()
{
    return R"rs(fn lease_definition_from_descriptor(definition: LeaseDefinition) -> RuntimeLeaseDefinition {
    RuntimeLeaseDefinition {
        id: LeaseDefinitionId {
            name: definition.name.clone(),
            version: 1,
        },
        resource_pattern: definition
            .resource
            .unwrap_or_else(|| definition.name.clone()),
        ttl: definition.ttl,
        renew_every: definition.renew_every.unwrap_or(definition.ttl),
        max_ttl: definition.max_ttl,
        fencing_token: definition.fencing_token,
    }
}

)rs";
}

std::string rust_registration_helpers(const RuntimeDomainUsage& usage)
{
    std::ostringstream out;
    if (usage.uses_feature_flags)
    {
        out << R"rs(pub fn register_feature_flag_definitions_tx<B: Backend, S: FeatureFlagStore<B>>(
    tx: &mut B::Tx,
    store: &S,
) -> BackendResult<()> {
    for definition in feature_flag_definitions() {
        store.register_definition_tx(
            tx,
            &RuntimeFeatureFlagDefinition {
                name: definition.name.clone(),
                flag_type: feature_flag_type_from_descriptor(&definition.flag_type),
                default_value: feature_flag_value_from_descriptor(&definition),
                scope: feature_flag_scope_from_descriptor(&definition.scope),
                owner: definition.owner.clone(),
                description: definition.description.clone(),
                expires: definition.expires.clone(),
            },
        )?;
    }
    Ok(())
}

)rs";
    }
    if (usage.uses_queues)
    {
        out << R"rs(pub fn register_queue_definitions_tx<B: Backend, S: QueueStore<B>>(
    tx: &mut B::Tx,
    store: &S,
) -> BackendResult<()> {
    for definition in queue_definitions() {
        store.register_definition_tx(tx, &RegisterQueueDefinitionRequest { definition })?;
    }
    Ok(())
}

)rs";
    }
    if (usage.uses_leases)
    {
        out << R"rs(pub fn register_lease_definitions_tx<B: Backend, S: LeaseStore<B>>(
    tx: &mut B::Tx,
    store: &S,
) -> BackendResult<()> {
    for definition in lease_definitions() {
        let runtime_definition = lease_definition_from_descriptor(definition);
        store.register_definition_tx(tx, &runtime_definition)?;
    }
    Ok(())
}

)rs";
    }
    if (usage.uses_logs)
    {
        out << R"rs(pub fn register_log_definitions_tx<B: Backend, S: LogSink<B>>(
    tx: &mut B::Tx,
    sink: &S,
) -> BackendResult<()> {
    for definition in log_definitions() {
        sink.register_definition_tx(
            tx,
            &RuntimeLogDefinition {
                name: definition.name,
                level: log_level_from_descriptor(&definition.level),
                event_name: definition.event_name,
                fields: definition.fields,
            },
        )?;
    }
    Ok(())
}

)rs";
    }
    if (usage.uses_metrics)
    {
        out << R"rs(pub fn register_metric_definitions_tx<B: Backend, S: MetricSink<B>>(
    tx: &mut B::Tx,
    sink: &S,
) -> BackendResult<()> {
    for definition in metric_definitions() {
        sink.register_definition_tx(
            tx,
            &RuntimeMetricDefinition {
                name: definition.name,
                kind: metric_kind_from_descriptor(&definition.kind),
                backend_name: definition.backend_name,
                unit: definition.unit,
                labels: definition.labels,
            },
        )?;
    }
    Ok(())
}

)rs";
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        out << R"rs(pub fn register_observability_catalog_tx<B, L, M>(
    tx: &mut B::Tx,
    log_sink: &L,
    metric_sink: &M,
) -> BackendResult<()>
where
    B: Backend,
    L: LogSink<B>,
    M: MetricSink<B>,
{
    register_log_definitions_tx(tx, log_sink)?;
    register_metric_definitions_tx(tx, metric_sink)
}

)rs";
    }
    if (usage.uses_workflows)
    {
        out << R"rs(pub fn register_workflow_definitions_tx<B: Backend, S: WorkflowStore<B>>(
    tx: &mut B::Tx,
    store: &S,
) -> BackendResult<()> {
    for definition in workflow_definitions() {
        store.register_definition_tx(tx, &RegisterWorkflowDefinitionRequest { definition })?;
    }
    Ok(())
}

)rs";
    }
    return out.str();
}

} // namespace

std::string generate_rust_runtime_registration(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream helpers;
    std::ostringstream generic_parameters;
    std::ostringstream generic_bounds;
    std::ostringstream params;
    std::ostringstream args;
    std::ostringstream calls;
    auto add = [&](std::string_view generic, std::string_view bound, std::string_view name,
                   std::string_view fn)
    {
        generic_parameters << ", " << generic;
        generic_bounds << ",\n    " << generic << ": " << bound << "<B>";
        params << ",\n    " << name << ": &" << generic;
        args << ", " << name;
        calls << "    " << fn << "(tx, " << name << ")?;\n";
    };
    if (usage.uses_feature_flags)
    {
        helpers << rust_feature_flag_helpers();
        add("F", "FeatureFlagStore", "feature_flag_store", "register_feature_flag_definitions_tx");
    }
    if (usage.uses_queues)
    {
        add("Q", "QueueStore", "queue_store", "register_queue_definitions_tx");
    }
    if (usage.uses_leases)
    {
        helpers << rust_lease_helpers();
        add("L", "LeaseStore", "lease_store", "register_lease_definitions_tx");
    }
    if (usage.uses_workflows)
    {
        add("W", "WorkflowStore", "workflow_store", "register_workflow_definitions_tx");
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        generic_parameters << ", LS, M";
        generic_bounds << ",\n    LS: LogSink<B>,\n    M: MetricSink<B>";
        params << ",\n    log_sink: &LS,\n    metric_sink: &M";
        args << ", log_sink, metric_sink";
        calls << "    register_observability_catalog_tx(tx, log_sink, metric_sink)?;\n";
    }
    else
    {
        if (usage.uses_logs)
        {
            add("LS", "LogSink", "log_sink", "register_log_definitions_tx");
        }
        if (usage.uses_metrics)
        {
            add("M", "MetricSink", "metric_sink", "register_metric_definitions_tx");
        }
    }

    return templates.render(
        "generated/runtime_registration.rs.tmpl",
        TemplateRenderer::Values{
            {"feature_flag_helpers", helpers.str()},
            {"lease_helpers", {}},
            {"domain_registration_helpers", rust_registration_helpers(usage)},
            {"generic_parameters", generic_parameters.str()},
            {"generic_bounds", generic_bounds.str()},
            {"tx_parameters", params.str()},
            {"runtime_parameters", params.str()},
            {"runtime_arguments", args.str()},
            {"tx_calls", calls.str()},
        }
    );
}

} // namespace statespec
