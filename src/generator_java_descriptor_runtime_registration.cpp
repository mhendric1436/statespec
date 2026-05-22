#include "generator_java_descriptor_areas.hpp"

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
    const TemplatePackage& templates
)
{
    std::ostringstream out;
    if (usage.uses_feature_flags)
    {
        out << java_runtime_registration_snippet(templates, "feature_flags");
    }
    if (usage.uses_queues)
    {
        out << java_runtime_registration_snippet(templates, "queues");
    }
    if (usage.uses_leases)
    {
        out << java_runtime_registration_snippet(templates, "leases");
    }
    if (usage.uses_logs)
    {
        out << java_runtime_registration_snippet(templates, "logs");
    }
    if (usage.uses_metrics)
    {
        out << java_runtime_registration_snippet(templates, "metrics");
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        out << java_runtime_registration_snippet(templates, "observability");
    }
    if (usage.uses_workflows)
    {
        out << java_runtime_registration_snippet(templates, "workflows");
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
            {"tx_calls", calls.str()},
        }
    );
}

} // namespace statespec
