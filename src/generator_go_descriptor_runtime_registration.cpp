#include "generator_go_descriptor_areas.hpp"

#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{
namespace
{

std::string go_runtime_registration_snippet(
    const TemplatePackage& templates,
    std::string_view name
)
{
    return templates.load("generated/runtime_registration_" + std::string(name) + ".go.tmpl");
}

} // namespace

std::string generate_go_runtime_registration_domain(
    const TemplatePackage& templates,
    std::string_view name
)
{
    return go_runtime_registration_snippet(templates, name);
}

std::string generate_go_runtime_registration(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream params;
    std::ostringstream args;
    std::ostringstream calls;
    std::ostringstream ensure_collections_body;
    ensure_collections_body << "\tvar descriptors []CollectionDescriptor\n";
    for (const auto& entity : system.entities)
    {
        ensure_collections_body << "\tdescriptors = append(descriptors, "
                                << pascal_identifier(entity.name)
                                << "CollectionDescriptors()...)\n";
    }
    ensure_collections_body << "\treturn backend.EnsureCollections(ctx, descriptors)\n";
    auto add = [&](std::string_view type, std::string_view name, std::string_view fn)
    {
        params << ", " << name << " " << type;
        args << ", " << name;
        calls << "\tif err := " << fn << "(ctx, tx, " << name << "); err != nil {\n";
        calls << "\t\treturn err\n";
        calls << "\t}\n";
    };
    if (usage.uses_feature_flags)
    {
        add("FeatureFlagStore", "featureFlagStore", "RegisterFeatureFlagDefinitionsTx");
    }
    if (usage.uses_queues)
    {
        add("QueueStore", "queueStore", "RegisterQueueDefinitionsTx");
    }
    if (usage.uses_leases)
    {
        add("LeaseStore", "leaseStore", "RegisterLeaseDefinitionsTx");
    }
    if (usage.uses_workflows)
    {
        add("WorkflowStore", "workflowStore", "RegisterWorkflowDefinitionsTx");
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        params << ", logSink LogSink, metricSink MetricSink";
        args << ", logSink, metricSink";
        calls << "\tif err := RegisterObservabilityCatalogTx(ctx, tx, logSink, metricSink); err != "
                 "nil {\n";
        calls << "\t\treturn err\n";
        calls << "\t}\n";
    }
    else
    {
        if (usage.uses_logs)
        {
            add("LogSink", "logSink", "RegisterLogDefinitionsTx");
        }
        if (usage.uses_metrics)
        {
            add("MetricSink", "metricSink", "RegisterMetricDefinitionsTx");
        }
    }
    return templates.render(
        "generated/runtime_registration.go.tmpl",
        TemplateRenderer::Values{
            {"feature_flag_helpers", {}},
            {"lease_helpers", {}},
            {"domain_registration_helpers", {}},
            {"tx_parameters", params.str()},
            {"runtime_parameters", params.str()},
            {"runtime_arguments", args.str()},
            {"ensure_system_collections_body", ensure_collections_body.str()},
            {"tx_calls", calls.str()},
        }
    );
}

} // namespace statespec
