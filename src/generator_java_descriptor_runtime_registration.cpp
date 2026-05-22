#include "generator_java_descriptor_areas.hpp"

#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string java_feature_flag_helpers()
{
    return R"java(    private static FeatureFlag.Type featureFlagTypeFromDescriptor(String type) {
        return switch (type) {
            case "string" -> FeatureFlag.Type.STRING;
            case "int" -> FeatureFlag.Type.INT;
            case "decimal" -> FeatureFlag.Type.DECIMAL;
            default -> FeatureFlag.Type.BOOL;
        };
    }

    private static FeatureFlag.ScopeKind featureFlagScopeFromDescriptor(String scope) {
        if (scope.equals("system")) {
            return FeatureFlag.ScopeKind.SYSTEM;
        }
        if (scope.equals("user")) {
            return FeatureFlag.ScopeKind.USER;
        }
        if (scope.startsWith("entity ")) {
            return FeatureFlag.ScopeKind.ENTITY;
        }
        return FeatureFlag.ScopeKind.TENANT;
    }

    private static FeatureFlag.Value featureFlagValueFromDescriptor(
        FeatureFlagDefinition definition
    ) {
        return switch (definition.type()) {
            case "string" -> new FeatureFlag.Value.StringValue(definition.defaultValue());
            case "int" -> new FeatureFlag.Value.IntValue(Long.parseLong(definition.defaultValue()));
            case "decimal" -> new FeatureFlag.Value.DecimalValue(definition.defaultValue());
            default -> new FeatureFlag.Value.BoolValue(Boolean.parseBoolean(definition.defaultValue()));
        };
    }

)java";
}

std::string java_lease_helpers()
{
    return R"java(    private static Lease.LeaseDefinition leaseDefinitionFromDescriptor(
        LeaseDefinition definition
    ) {
        return new Lease.LeaseDefinition(
            new Lease.LeaseDefinitionId(definition.name(), 1L),
            definition.resource().orElse(definition.name()),
            definition.ttl(),
            definition.renewEvery().orElse(definition.ttl()),
            definition.maxTtl(),
            definition.fencingToken()
        );
    }

)java";
}

std::string java_registration_helpers(const RuntimeDomainUsage& usage)
{
    std::ostringstream out;
    if (usage.uses_feature_flags)
    {
        out << R"java(    public static void registerFeatureFlagDefinitionsTx(
        Backend.Transaction tx,
        FeatureFlag store
    ) throws Backend.BackendException {
        for (var definition : featureFlagDefinitions()) {
            store.registerDefinitionTx(
                tx,
                new FeatureFlag.Definition(
                    definition.name(),
                    featureFlagTypeFromDescriptor(definition.type()),
                    featureFlagValueFromDescriptor(definition),
                    featureFlagScopeFromDescriptor(definition.scope()),
                    definition.owner(),
                    definition.description(),
                    definition.expires()
                )
            );
        }
    }

)java";
    }
    if (usage.uses_queues)
    {
        out << R"java(    public static void registerQueueDefinitionsTx(
        Backend.Transaction tx,
        Queue store
    ) throws Backend.BackendException {
        for (var definition : queueDefinitions()) {
            store.registerDefinitionTx(
                tx,
                new Queue.RegisterQueueDefinitionRequest(definition)
            );
        }
    }

)java";
    }
    if (usage.uses_leases)
    {
        out << R"java(    public static void registerLeaseDefinitionsTx(
        Backend.Transaction tx,
        Lease store
    ) throws Backend.BackendException {
        for (var definition : leaseDefinitions()) {
            store.registerDefinitionTx(tx, leaseDefinitionFromDescriptor(definition));
        }
    }

)java";
    }
    if (usage.uses_logs)
    {
        out << R"java(    public static void registerLogDefinitionsTx(
        Backend.Transaction tx,
        Log sink
    ) throws Backend.BackendException {
        for (var definition : logDefinitions()) {
            sink.registerDefinitionTx(
                tx,
                new Log.Definition(
                    definition.name(),
                    logLevelFromDescriptor(definition.level()),
                    definition.eventName(),
                    definition.fields()
                )
            );
        }
    }

)java";
    }
    if (usage.uses_metrics)
    {
        out << R"java(    public static void registerMetricDefinitionsTx(
        Backend.Transaction tx,
        Metric sink
    ) throws Backend.BackendException {
        for (var definition : metricDefinitions()) {
            sink.registerDefinitionTx(
                tx,
                new Metric.Definition(
                    definition.name(),
                    metricKindFromDescriptor(definition.kind()),
                    definition.backendName(),
                    definition.unit(),
                    definition.labels()
                )
            );
        }
    }

)java";
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        out << R"java(    public static void registerObservabilityCatalogTx(
        Backend.Transaction tx,
        Log logSink,
        Metric metricSink
    ) throws Backend.BackendException {
        registerLogDefinitionsTx(tx, logSink);
        registerMetricDefinitionsTx(tx, metricSink);
    }

)java";
    }
    if (usage.uses_workflows)
    {
        out << R"java(    public static void registerWorkflowDefinitionsTx(
        Backend.Transaction tx,
        Workflow store
    ) throws Backend.BackendException {
        for (var definition : workflowDefinitions()) {
            store.registerDefinitionTx(
                tx,
                new Workflow.RegisterWorkflowDefinitionRequest(definition)
            );
        }
    }

)java";
    }
    return out.str();
}

} // namespace

std::string generate_java_runtime_registration(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream helpers;
    std::ostringstream params;
    std::ostringstream args;
    std::ostringstream calls;
    auto add = [&](std::string_view type, std::string_view name, std::string_view fn)
    {
        params << ",\n        " << type << " " << name;
        args << ", " << name;
        calls << "        " << fn << "(tx, " << name << ");\n";
    };
    if (usage.uses_feature_flags)
    {
        helpers << java_feature_flag_helpers();
        add("FeatureFlag", "featureFlagStore", "registerFeatureFlagDefinitionsTx");
    }
    if (usage.uses_queues)
    {
        add("Queue", "queueStore", "registerQueueDefinitionsTx");
    }
    if (usage.uses_leases)
    {
        helpers << java_lease_helpers();
        add("Lease", "leaseStore", "registerLeaseDefinitionsTx");
    }
    if (usage.uses_workflows)
    {
        add("Workflow", "workflowStore", "registerWorkflowDefinitionsTx");
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        params << ",\n        Log logSink,\n        Metric metricSink";
        args << ", logSink, metricSink";
        calls << "        registerObservabilityCatalogTx(tx, logSink, metricSink);\n";
    }
    else
    {
        if (usage.uses_logs)
        {
            add("Log", "logSink", "registerLogDefinitionsTx");
        }
        if (usage.uses_metrics)
        {
            add("Metric", "metricSink", "registerMetricDefinitionsTx");
        }
    }

    return templates.render(
        "generated/RuntimeRegistration.java.tmpl",
        TemplateRenderer::Values{
            {"feature_flag_helpers", helpers.str()},
            {"lease_helpers", {}},
            {"domain_registration_helpers", java_registration_helpers(usage)},
            {"tx_parameters", params.str()},
            {"runtime_parameters", params.str()},
            {"runtime_arguments", args.str()},
            {"tx_calls", calls.str()},
        }
    );
}

} // namespace statespec
