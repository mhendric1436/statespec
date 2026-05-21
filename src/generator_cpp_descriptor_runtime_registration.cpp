#include "generator_cpp_descriptor_areas.hpp"

#include "generator_cpp_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_cpp_runtime_registration(const IrSystem&)
{
    std::ostringstream out;
    out << "inline statespec::backend::FeatureFlagType feature_flag_type_from_string(\n";
    out << "    std::string_view type\n";
    out << ")\n";
    out << "{\n";
    out << "    if (type == \"string\") { return statespec::backend::FeatureFlagType::String; }\n";
    out << "    if (type == \"int\") { return statespec::backend::FeatureFlagType::Integer; }\n";
    out << "    if (type == \"decimal\") { return statespec::backend::FeatureFlagType::Decimal; "
           "}\n";
    out << "    return statespec::backend::FeatureFlagType::Bool;\n";
    out << "}\n\n";

    out << "inline statespec::backend::FeatureFlagScopeKind feature_flag_scope_from_string(\n";
    out << "    std::string_view scope\n";
    out << ")\n";
    out << "{\n";
    out << "    if (scope == \"system\") { return "
           "statespec::backend::FeatureFlagScopeKind::System; }\n";
    out << "    if (scope == \"user\") { return statespec::backend::FeatureFlagScopeKind::User; "
           "}\n";
    out << "    if (scope.rfind(\"entity \", 0) == 0) { return "
           "statespec::backend::FeatureFlagScopeKind::Entity; }\n";
    out << "    return statespec::backend::FeatureFlagScopeKind::Tenant;\n";
    out << "}\n\n";

    out << "inline statespec::backend::FeatureFlagValue feature_flag_value_from_descriptor(\n";
    out << "    const FeatureFlagDefinition& definition\n";
    out << ")\n";
    out << "{\n";
    out << "    if (definition.type == \"string\")\n";
    out << "    {\n";
    out << "        return "
           "statespec::backend::FeatureFlagValue::string_value(definition.default_value);\n";
    out << "    }\n";
    out << "    if (definition.type == \"int\")\n";
    out << "    {\n";
    out << "        return "
           "statespec::backend::FeatureFlagValue::integer_value(std::stoll(definition.default_"
           "value));\n";
    out << "    }\n";
    out << "    if (definition.type == \"decimal\")\n";
    out << "    {\n";
    out << "        return "
           "statespec::backend::FeatureFlagValue::decimal_value(std::stod(definition.default_value)"
           ");\n";
    out << "    }\n";
    out << "    return statespec::backend::FeatureFlagValue::bool_value(definition.default_value "
           "== \"true\");\n";
    out << "}\n\n";

    out << "inline void ensure_system_collections(statespec::backend::IBackend& backend)\n";
    out << "{\n";
    out << "    backend.ensure_collections(collection_descriptors());\n";
    out << "}\n\n";

    out << "inline void register_feature_flag_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IFeatureFlagStore& store\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : feature_flag_definitions())\n";
    out << "    {\n";
    out << "        store.register_definitionTx(\n";
    out << "            tx,\n";
    out << "            statespec::backend::FeatureFlagDefinition{\n";
    out << "                definition.name,\n";
    out << "                feature_flag_type_from_string(definition.type),\n";
    out << "                feature_flag_value_from_descriptor(definition),\n";
    out << "                feature_flag_scope_from_string(definition.scope),\n";
    out << "                definition.owner,\n";
    out << "                definition.description,\n";
    out << "                definition.expires,\n";
    out << "            }\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline void register_queue_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IQueueStore& store\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : queue_definitions())\n";
    out << "    {\n";
    out << "        store.register_definitionTx(\n";
    out << "            tx,\n";
    out << "            statespec::backend::RegisterQueueDefinitionRequest{definition}\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline statespec::backend::LeaseDefinition lease_definition_from_descriptor(\n";
    out << "    const LeaseDefinition& definition\n";
    out << ")\n";
    out << "{\n";
    out << "    return statespec::backend::LeaseDefinition{\n";
    out << "        statespec::backend::LeaseDefinitionId{definition.name, 1},\n";
    out << "        definition.resource.value_or(definition.name),\n";
    out << "        definition.ttl,\n";
    out << "        definition.renew_every.value_or(definition.ttl),\n";
    out << "        definition.max_ttl,\n";
    out << "        definition.fencing_token,\n";
    out << "    };\n";
    out << "}\n\n";

    out << "inline void register_lease_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::ILeaseStore& store\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : lease_definitions())\n";
    out << "    {\n";
    out << "        store.register_definitionTx(tx, "
           "lease_definition_from_descriptor(definition));\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline void register_log_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::ILogSink& sink\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : log_definitions())\n";
    out << "    {\n";
    out << "        sink.register_definitionTx(\n";
    out << "            tx,\n";
    out << "            statespec::backend::LogDefinition{\n";
    out << "                definition.name,\n";
    out << "                log_level_from_string(definition.level),\n";
    out << "                definition.event_name,\n";
    out << "                definition.fields,\n";
    out << "            }\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline void register_metric_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IMetricSink& sink\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : metric_definitions())\n";
    out << "    {\n";
    out << "        sink.register_definitionTx(\n";
    out << "            tx,\n";
    out << "            statespec::backend::MetricDefinition{\n";
    out << "                definition.name,\n";
    out << "                metric_kind_from_string(definition.kind),\n";
    out << "                definition.backend_name,\n";
    out << "                definition.unit,\n";
    out << "                definition.labels,\n";
    out << "            }\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline void register_observability_catalogTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::ILogSink& log_sink,\n";
    out << "    statespec::backend::IMetricSink& metric_sink\n";
    out << ")\n";
    out << "{\n";
    out << "    register_log_definitionsTx(tx, log_sink);\n";
    out << "    register_metric_definitionsTx(tx, metric_sink);\n";
    out << "}\n\n";

    out << "inline void register_workflow_definitionsTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IWorkflowStore& workflow_store\n";
    out << ")\n";
    out << "{\n";
    out << "    for (const auto& definition : workflow_definitions())\n";
    out << "    {\n";
    out << "        workflow_store.register_definitionTx(\n";
    out << "            tx,\n";
    out << "            statespec::backend::RegisterWorkflowDefinitionRequest{definition}\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n\n";

    out << "inline void register_runtime_catalogTx(\n";
    out << "    statespec::backend::ITransaction& tx,\n";
    out << "    statespec::backend::IFeatureFlagStore& feature_flag_store,\n";
    out << "    statespec::backend::IQueueStore& queue_store,\n";
    out << "    statespec::backend::ILeaseStore& lease_store,\n";
    out << "    statespec::backend::IWorkflowStore& workflow_store,\n";
    out << "    statespec::backend::ILogSink& log_sink,\n";
    out << "    statespec::backend::IMetricSink& metric_sink\n";
    out << ")\n";
    out << "{\n";
    out << "    register_feature_flag_definitionsTx(tx, feature_flag_store);\n";
    out << "    register_queue_definitionsTx(tx, queue_store);\n";
    out << "    register_lease_definitionsTx(tx, lease_store);\n";
    out << "    register_workflow_definitionsTx(tx, workflow_store);\n";
    out << "    register_observability_catalogTx(tx, log_sink, metric_sink);\n";
    out << "}\n\n";

    return out.str();
}

} // namespace statespec
