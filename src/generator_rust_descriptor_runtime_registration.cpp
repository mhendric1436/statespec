#include "generator_rust_descriptor_areas.hpp"

#include "generator_rust_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_rust_runtime_registration(const IrSystem&)
{
    std::ostringstream out;
    out << "fn feature_flag_type_from_descriptor(flag_type: &str) -> FeatureFlagType {\n";
    out << "    match flag_type {\n";
    out << "        \"string\" => FeatureFlagType::String,\n";
    out << "        \"int\" => FeatureFlagType::Integer,\n";
    out << "        \"decimal\" => FeatureFlagType::Decimal,\n";
    out << "        _ => FeatureFlagType::Bool,\n";
    out << "    }\n";
    out << "}\n\n";

    out << "fn feature_flag_scope_from_descriptor(scope: &str) -> FeatureFlagScopeKind {\n";
    out << "    match scope {\n";
    out << "        \"system\" => FeatureFlagScopeKind::System,\n";
    out << "        \"user\" => FeatureFlagScopeKind::User,\n";
    out << "        _ if scope.starts_with(\"entity \") => FeatureFlagScopeKind::Entity,\n";
    out << "        _ => FeatureFlagScopeKind::Tenant,\n";
    out << "    }\n";
    out << "}\n\n";

    out << "fn feature_flag_value_from_descriptor(\n";
    out << "    definition: &FeatureFlagDefinition,\n";
    out << ") -> FeatureFlagValue {\n";
    out << "    match definition.flag_type.as_str() {\n";
    out << "        \"string\" => FeatureFlagValue::String(definition.default_value.clone()),\n";
    out << "        \"int\" => "
           "FeatureFlagValue::Integer(definition.default_value.parse().unwrap_or(0)),\n";
    out << "        \"decimal\" => "
           "FeatureFlagValue::Decimal(definition.default_value.parse().unwrap_or(0.0)),\n";
    out << "        _ => FeatureFlagValue::Bool(definition.default_value == \"true\"),\n";
    out << "    }\n";
    out << "}\n\n";

    out << "fn lease_definition_from_descriptor(\n";
    out << "    definition: LeaseDefinition,\n";
    out << ") -> RuntimeLeaseDefinition {\n";
    out << "    RuntimeLeaseDefinition {\n";
    out << "        id: LeaseDefinitionId { name: definition.name.clone(), version: 1 },\n";
    out << "        resource_pattern: definition.resource.unwrap_or_else(|| "
           "definition.name.clone()),\n";
    out << "        ttl: definition.ttl,\n";
    out << "        renew_every: definition.renew_every.unwrap_or(definition.ttl),\n";
    out << "        max_ttl: definition.max_ttl,\n";
    out << "        fencing_token: definition.fencing_token,\n";
    out << "    }\n";
    out << "}\n\n";

    out << "pub fn ensure_system_collections<B: Backend>(backend: &B) -> BackendResult<()> {\n";
    out << "    backend.ensure_collections(&collection_descriptors())\n";
    out << "}\n\n";

    out << "pub fn register_feature_flag_definitions_tx<B: Backend, S: FeatureFlagStore<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    store: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in feature_flag_definitions() {\n";
    out << "        store.register_definition_tx(\n";
    out << "            tx,\n";
    out << "            &RuntimeFeatureFlagDefinition {\n";
    out << "                name: definition.name.clone(),\n";
    out << "                flag_type: feature_flag_type_from_descriptor(&definition.flag_type),\n";
    out << "                default_value: feature_flag_value_from_descriptor(&definition),\n";
    out << "                scope: feature_flag_scope_from_descriptor(&definition.scope),\n";
    out << "                owner: definition.owner.clone(),\n";
    out << "                description: definition.description.clone(),\n";
    out << "                expires: definition.expires.clone(),\n";
    out << "            },\n";
    out << "        )?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_queue_definitions_tx<B: Backend, S: QueueStore<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    store: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in queue_definitions() {\n";
    out << "        store.register_definition_tx(tx, &RegisterQueueDefinitionRequest { definition "
           "})?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_lease_definitions_tx<B: Backend, S: LeaseStore<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    store: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in lease_definitions() {\n";
    out << "        let runtime_definition = lease_definition_from_descriptor(definition);\n";
    out << "        store.register_definition_tx(tx, &runtime_definition)?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_log_definitions_tx<B: Backend, S: LogSink<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    sink: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in log_definitions() {\n";
    out << "        sink.register_definition_tx(\n";
    out << "            tx,\n";
    out << "            &RuntimeLogDefinition {\n";
    out << "                name: definition.name,\n";
    out << "                level: log_level_from_descriptor(&definition.level),\n";
    out << "                event_name: definition.event_name,\n";
    out << "                fields: definition.fields,\n";
    out << "            },\n";
    out << "        )?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_metric_definitions_tx<B: Backend, S: MetricSink<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    sink: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in metric_definitions() {\n";
    out << "        sink.register_definition_tx(\n";
    out << "            tx,\n";
    out << "            &RuntimeMetricDefinition {\n";
    out << "                name: definition.name,\n";
    out << "                kind: metric_kind_from_descriptor(&definition.kind),\n";
    out << "                backend_name: definition.backend_name,\n";
    out << "                unit: definition.unit,\n";
    out << "                labels: definition.labels,\n";
    out << "            },\n";
    out << "        )?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_observability_catalog_tx<B, L, M>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    log_sink: &L,\n";
    out << "    metric_sink: &M,\n";
    out << ") -> BackendResult<()>\n";
    out << "where\n";
    out << "    B: Backend,\n";
    out << "    L: LogSink<B>,\n";
    out << "    M: MetricSink<B>,\n";
    out << "{\n";
    out << "    register_log_definitions_tx(tx, log_sink)?;\n";
    out << "    register_metric_definitions_tx(tx, metric_sink)\n";
    out << "}\n\n";

    out << "pub fn register_workflow_definitions_tx<B: Backend, S: WorkflowStore<B>>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    store: &S,\n";
    out << ") -> BackendResult<()> {\n";
    out << "    for definition in workflow_definitions() {\n";
    out << "        store.register_definition_tx(\n";
    out << "            tx,\n";
    out << "            &RegisterWorkflowDefinitionRequest { definition },\n";
    out << "        )?;\n";
    out << "    }\n";
    out << "    Ok(())\n";
    out << "}\n\n";

    out << "pub fn register_runtime_catalog_tx<B, F, Q, L, W, LS, M>(\n";
    out << "    tx: &mut B::Tx,\n";
    out << "    feature_flag_store: &F,\n";
    out << "    queue_store: &Q,\n";
    out << "    lease_store: &L,\n";
    out << "    workflow_store: &W,\n";
    out << "    log_sink: &LS,\n";
    out << "    metric_sink: &M,\n";
    out << ") -> BackendResult<()>\n";
    out << "where\n";
    out << "    B: Backend,\n";
    out << "    F: FeatureFlagStore<B>,\n";
    out << "    Q: QueueStore<B>,\n";
    out << "    L: LeaseStore<B>,\n";
    out << "    W: WorkflowStore<B>,\n";
    out << "    LS: LogSink<B>,\n";
    out << "    M: MetricSink<B>,\n";
    out << "{\n";
    out << "    register_feature_flag_definitions_tx(tx, feature_flag_store)?;\n";
    out << "    register_queue_definitions_tx(tx, queue_store)?;\n";
    out << "    register_lease_definitions_tx(tx, lease_store)?;\n";
    out << "    register_workflow_definitions_tx(tx, workflow_store)?;\n";
    out << "    register_observability_catalog_tx(tx, log_sink, metric_sink)\n";
    out << "}\n";
    return out.str();
}

} // namespace statespec
