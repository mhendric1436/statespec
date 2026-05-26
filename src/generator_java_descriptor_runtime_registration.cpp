#include "generator_java_descriptor_areas.hpp"

#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string java_runtime_registration_snippet(
    const TemplatePackage& templates,
    std::string_view name
)
{
    return templates.load("generated/runtime_registration_" + std::string(name) + ".java.tmpl");
}

std::string java_registration_helpers(
    const RuntimeDomainUsage& usage,
    const TemplatePackage&
)
{
    std::ostringstream out;
    if (usage.uses_feature_flags)
    {
        out << "    public static void registerFeatureFlagDefinitionsTx(\n";
        out << "        Backend.Transaction tx,\n";
        out << "        FeatureFlag featureFlagStore\n";
        out << "    ) throws Backend.BackendException {\n";
        out << "        com.statespec.generated.descriptors.RuntimeFeatureFlagRegistrationModule."
               "registerFeatureFlagDefinitionsTx(tx, featureFlagStore);\n";
        out << "    }\n\n";
    }
    if (usage.uses_queues)
    {
        out << "    public static void registerQueueDefinitionsTx(\n";
        out << "        Backend.Transaction tx,\n";
        out << "        Queue queueStore\n";
        out << "    ) throws Backend.BackendException {\n";
        out << "        com.statespec.generated.descriptors.RuntimeQueueRegistrationModule."
               "registerQueueDefinitionsTx(tx, queueStore);\n";
        out << "    }\n\n";
    }
    if (usage.uses_leases)
    {
        out << "    public static void registerLeaseDefinitionsTx(\n";
        out << "        Backend.Transaction tx,\n";
        out << "        Lease leaseStore\n";
        out << "    ) throws Backend.BackendException {\n";
        out << "        com.statespec.generated.descriptors.RuntimeLeaseRegistrationModule."
               "registerLeaseDefinitionsTx(tx, leaseStore);\n";
        out << "    }\n\n";
    }
    if (usage.uses_logs)
    {
        out << "    public static void registerLogDefinitionsTx(\n";
        out << "        Backend.Transaction tx,\n";
        out << "        Log logSink\n";
        out << "    ) throws Backend.BackendException {\n";
        out << "        com.statespec.generated.descriptors.RuntimeLogRegistrationModule."
               "registerLogDefinitionsTx(tx, logSink);\n";
        out << "    }\n\n";
    }
    if (usage.uses_metrics)
    {
        out << "    public static void registerMetricDefinitionsTx(\n";
        out << "        Backend.Transaction tx,\n";
        out << "        Metric metricSink\n";
        out << "    ) throws Backend.BackendException {\n";
        out << "        com.statespec.generated.descriptors.RuntimeMetricRegistrationModule."
               "registerMetricDefinitionsTx(tx, metricSink);\n";
        out << "    }\n\n";
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        out << "    public static void registerObservabilityCatalogTx(\n";
        out << "        Backend.Transaction tx,\n";
        out << "        Log logSink,\n";
        out << "        Metric metricSink\n";
        out << "    ) throws Backend.BackendException {\n";
        out << "        com.statespec.generated.descriptors.RuntimeObservabilityRegistrationModule."
               "registerObservabilityCatalogTx(tx, logSink, metricSink);\n";
        out << "    }\n\n";
    }
    if (usage.uses_workflows)
    {
        out << "    public static void registerWorkflowDefinitionsTx(\n";
        out << "        Backend.Transaction tx,\n";
        out << "        Workflow workflowStore\n";
        out << "    ) throws Backend.BackendException {\n";
        out << "        com.statespec.generated.descriptors.RuntimeWorkflowRegistrationModule."
               "registerWorkflowDefinitionsTx(tx, workflowStore);\n";
        out << "    }\n\n";
    }
    return out.str();
}

} // namespace

std::string generate_java_runtime_registration_domain(
    const TemplatePackage& templates,
    std::string_view name
)
{
    if (name == "observability")
    {
        return R"(    public static void registerObservabilityCatalogTx(
        Backend.Transaction tx,
        Log logSink,
        Metric metricSink
    ) throws Backend.BackendException {
        RuntimeLogRegistrationModule.registerLogDefinitionsTx(tx, logSink);
        RuntimeMetricRegistrationModule.registerMetricDefinitionsTx(tx, metricSink);
    }
)";
    }
    return java_runtime_registration_snippet(templates, name);
}

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
    std::ostringstream ensure_collections_body;
    ensure_collections_body << "        backend.ensureCollections(List.of(\n";
    for (std::size_t i = 0; i < system.entities.size(); ++i)
    {
        const auto& entity = system.entities[i];
        ensure_collections_body << "            com.statespec.generated.entities."
                                << snake_identifier(entity.name) << ".Schema."
                                << lower_camel_identifier(entity.name) << "CollectionDescriptor()";
        ensure_collections_body << (i + 1 < system.entities.size() ? ",\n" : "\n");
    }
    ensure_collections_body << "        ));\n";
    auto add = [&](std::string_view type, std::string_view name, std::string_view fn)
    {
        params << ",\n        " << type << " " << name;
        args << ", " << name;
        calls << "        " << fn << "(tx, " << name << ");\n";
    };
    if (usage.uses_feature_flags)
    {
        add("FeatureFlag", "featureFlagStore", "registerFeatureFlagDefinitionsTx");
    }
    if (usage.uses_queues)
    {
        add("Queue", "queueStore", "registerQueueDefinitionsTx");
    }
    if (usage.uses_leases)
    {
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
            {"domain_registration_helpers", java_registration_helpers(usage, templates)},
            {"tx_parameters", params.str()},
            {"runtime_parameters", params.str()},
            {"runtime_arguments", args.str()},
            {"ensure_system_collections_body", ensure_collections_body.str()},
            {"tx_calls", calls.str()},
        }
    );
}

} // namespace statespec
