#include "generator_java_descriptor_areas.hpp"

#include "generator_java_descriptor_support.hpp"

#include <sstream>

namespace statespec
{

std::string generate_java_runtime_registration(const IrSystem&)
{
    std::ostringstream out;
    out << "    private static FeatureFlag.Type featureFlagTypeFromDescriptor(String type) {\n";
    out << "        return switch (type) {\n";
    out << "            case \"string\" -> FeatureFlag.Type.STRING;\n";
    out << "            case \"int\" -> FeatureFlag.Type.INT;\n";
    out << "            case \"decimal\" -> FeatureFlag.Type.DECIMAL;\n";
    out << "            default -> FeatureFlag.Type.BOOL;\n";
    out << "        };\n";
    out << "    }\n\n";

    out << "    private static FeatureFlag.ScopeKind featureFlagScopeFromDescriptor(String scope) "
           "{\n";
    out << "        if (scope.equals(\"system\")) {\n";
    out << "            return FeatureFlag.ScopeKind.SYSTEM;\n";
    out << "        }\n";
    out << "        if (scope.equals(\"user\")) {\n";
    out << "            return FeatureFlag.ScopeKind.USER;\n";
    out << "        }\n";
    out << "        if (scope.startsWith(\"entity \")) {\n";
    out << "            return FeatureFlag.ScopeKind.ENTITY;\n";
    out << "        }\n";
    out << "        return FeatureFlag.ScopeKind.TENANT;\n";
    out << "    }\n\n";

    out << "    private static FeatureFlag.Value featureFlagValueFromDescriptor(\n";
    out << "        FeatureFlagDefinition definition\n";
    out << "    ) {\n";
    out << "        return switch (definition.type()) {\n";
    out << "            case \"string\" -> new "
           "FeatureFlag.Value.StringValue(definition.defaultValue());\n";
    out << "            case \"int\" -> new "
           "FeatureFlag.Value.IntValue(Long.parseLong(definition.defaultValue()));\n";
    out << "            case \"decimal\" -> new "
           "FeatureFlag.Value.DecimalValue(definition.defaultValue());\n";
    out << "            default -> new "
           "FeatureFlag.Value.BoolValue(Boolean.parseBoolean(definition.defaultValue()));\n";
    out << "        };\n";
    out << "    }\n\n";

    out << "    private static Lease.LeaseDefinition leaseDefinitionFromDescriptor(\n";
    out << "        LeaseDefinition definition\n";
    out << "    ) {\n";
    out << "        return new Lease.LeaseDefinition(\n";
    out << "            new Lease.LeaseDefinitionId(definition.name(), 1L),\n";
    out << "            definition.resource().orElse(definition.name()),\n";
    out << "            definition.ttl(),\n";
    out << "            definition.renewEvery().orElse(definition.ttl()),\n";
    out << "            definition.maxTtl(),\n";
    out << "            definition.fencingToken()\n";
    out << "        );\n";
    out << "    }\n\n";

    out << "    public static void ensureSystemCollections(Backend backend)\n";
    out << "        throws Backend.BackendException {\n";
    out << "        backend.ensureCollections(collectionDescriptors());\n";
    out << "    }\n\n";

    out << "    public static void registerFeatureFlagDefinitionsTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        FeatureFlag store\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        for (var definition : featureFlagDefinitions()) {\n";
    out << "            store.registerDefinitionTx(\n";
    out << "                tx,\n";
    out << "                new FeatureFlag.Definition(\n";
    out << "                    definition.name(),\n";
    out << "                    featureFlagTypeFromDescriptor(definition.type()),\n";
    out << "                    featureFlagValueFromDescriptor(definition),\n";
    out << "                    featureFlagScopeFromDescriptor(definition.scope()),\n";
    out << "                    definition.owner(),\n";
    out << "                    definition.description(),\n";
    out << "                    definition.expires()\n";
    out << "                )\n";
    out << "            );\n";
    out << "        }\n";
    out << "    }\n\n";

    out << "    public static void registerQueueDefinitionsTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        Queue store\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        for (var definition : queueDefinitions()) {\n";
    out << "            store.registerDefinitionTx(\n";
    out << "                tx,\n";
    out << "                new Queue.RegisterQueueDefinitionRequest(definition)\n";
    out << "            );\n";
    out << "        }\n";
    out << "    }\n\n";

    out << "    public static void registerLeaseDefinitionsTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        Lease store\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        for (var definition : leaseDefinitions()) {\n";
    out << "            store.registerDefinitionTx(tx, "
           "leaseDefinitionFromDescriptor(definition));\n";
    out << "        }\n";
    out << "    }\n\n";

    out << "    public static void registerLogDefinitionsTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        Log sink\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        for (var definition : logDefinitions()) {\n";
    out << "            sink.registerDefinitionTx(\n";
    out << "                tx,\n";
    out << "                new Log.Definition(\n";
    out << "                    definition.name(),\n";
    out << "                    logLevelFromDescriptor(definition.level()),\n";
    out << "                    definition.eventName(),\n";
    out << "                    definition.fields()\n";
    out << "                )\n";
    out << "            );\n";
    out << "        }\n";
    out << "    }\n\n";

    out << "    public static void registerMetricDefinitionsTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        Metric sink\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        for (var definition : metricDefinitions()) {\n";
    out << "            sink.registerDefinitionTx(\n";
    out << "                tx,\n";
    out << "                new Metric.Definition(\n";
    out << "                    definition.name(),\n";
    out << "                    metricKindFromDescriptor(definition.kind()),\n";
    out << "                    definition.backendName(),\n";
    out << "                    definition.unit(),\n";
    out << "                    definition.labels()\n";
    out << "                )\n";
    out << "            );\n";
    out << "        }\n";
    out << "    }\n\n";

    out << "    public static void registerObservabilityCatalogTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        Log logSink,\n";
    out << "        Metric metricSink\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        registerLogDefinitionsTx(tx, logSink);\n";
    out << "        registerMetricDefinitionsTx(tx, metricSink);\n";
    out << "    }\n\n";

    out << "    public static void registerWorkflowDefinitionsTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        Workflow store\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        for (var definition : workflowDefinitions()) {\n";
    out << "            store.registerDefinitionTx(\n";
    out << "                tx,\n";
    out << "                new Workflow.RegisterWorkflowDefinitionRequest(definition)\n";
    out << "            );\n";
    out << "        }\n";
    out << "    }\n";
    out << "\n    public static void registerRuntimeCatalogTx(\n";
    out << "        Backend.Transaction tx,\n";
    out << "        FeatureFlag featureFlagStore,\n";
    out << "        Queue queueStore,\n";
    out << "        Lease leaseStore,\n";
    out << "        Workflow workflowStore,\n";
    out << "        Log logSink,\n";
    out << "        Metric metricSink\n";
    out << "    ) throws Backend.BackendException {\n";
    out << "        registerFeatureFlagDefinitionsTx(tx, featureFlagStore);\n";
    out << "        registerQueueDefinitionsTx(tx, queueStore);\n";
    out << "        registerLeaseDefinitionsTx(tx, leaseStore);\n";
    out << "        registerWorkflowDefinitionsTx(tx, workflowStore);\n";
    out << "        registerObservabilityCatalogTx(tx, logSink, metricSink);\n";
    out << "    }\n";
    return out.str();
}

} // namespace statespec
