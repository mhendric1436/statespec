#include "generator_java_descriptor_areas.hpp"

#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>
#include <vector>

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

struct JavaRuntimeRegistrationDomain
{
    std::string_view parameters;
    std::string_view arguments;
    std::string_view calls;
};

struct JavaRuntimeRegistrationHelper
{
    std::string_view method_name;
    std::string_view parameters;
    std::string_view call;
};

std::vector<JavaRuntimeRegistrationDomain>
java_runtime_registration_domains(const RuntimeDomainUsage& usage)
{
    std::vector<JavaRuntimeRegistrationDomain> domains;
    if (usage.uses_feature_flags)
    {
        domains.push_back(
            JavaRuntimeRegistrationDomain{
                ",\n        FeatureFlag featureFlagStore",
                ", featureFlagStore",
                "        registerFeatureFlagDefinitionsTx(tx, featureFlagStore);\n",
            }
        );
    }
    if (usage.uses_queues)
    {
        domains.push_back(
            JavaRuntimeRegistrationDomain{
                ",\n        Queue queueStore",
                ", queueStore",
                "        registerQueueDefinitionsTx(tx, queueStore);\n",
            }
        );
    }
    if (usage.uses_leases)
    {
        domains.push_back(
            JavaRuntimeRegistrationDomain{
                ",\n        Lease leaseStore",
                ", leaseStore",
                "        registerLeaseDefinitionsTx(tx, leaseStore);\n",
            }
        );
    }
    if (usage.uses_workflows)
    {
        domains.push_back(
            JavaRuntimeRegistrationDomain{
                ",\n        Workflow workflowStore",
                ", workflowStore",
                "        registerWorkflowDefinitionsTx(tx, workflowStore);\n",
            }
        );
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        domains.push_back(
            JavaRuntimeRegistrationDomain{
                ",\n        Log logSink,\n        Metric metricSink",
                ", logSink, metricSink",
                "        registerObservabilityCatalogTx(tx, logSink, metricSink);\n",
            }
        );
    }
    else
    {
        if (usage.uses_logs)
        {
            domains.push_back(
                JavaRuntimeRegistrationDomain{
                    ",\n        Log logSink",
                    ", logSink",
                    "        registerLogDefinitionsTx(tx, logSink);\n",
                }
            );
        }
        if (usage.uses_metrics)
        {
            domains.push_back(
                JavaRuntimeRegistrationDomain{
                    ",\n        Metric metricSink",
                    ", metricSink",
                    "        registerMetricDefinitionsTx(tx, metricSink);\n",
                }
            );
        }
    }
    return domains;
}

std::vector<JavaRuntimeRegistrationHelper>
java_runtime_registration_helpers(const RuntimeDomainUsage& usage)
{
    std::vector<JavaRuntimeRegistrationHelper> helpers;
    if (usage.uses_feature_flags)
    {
        helpers.push_back(
            JavaRuntimeRegistrationHelper{
                "registerFeatureFlagDefinitionsTx",
                "        FeatureFlag featureFlagStore\n",
                "        com.statespec.generated.descriptors.RuntimeFeatureFlagRegistrationModule."
                "registerFeatureFlagDefinitionsTx(tx, featureFlagStore);\n",
            }
        );
    }
    if (usage.uses_queues)
    {
        helpers.push_back(
            JavaRuntimeRegistrationHelper{
                "registerQueueDefinitionsTx",
                "        Queue queueStore\n",
                "        com.statespec.generated.descriptors.RuntimeQueueRegistrationModule."
                "registerQueueDefinitionsTx(tx, queueStore);\n",
            }
        );
    }
    if (usage.uses_leases)
    {
        helpers.push_back(
            JavaRuntimeRegistrationHelper{
                "registerLeaseDefinitionsTx",
                "        Lease leaseStore\n",
                "        com.statespec.generated.descriptors.RuntimeLeaseRegistrationModule."
                "registerLeaseDefinitionsTx(tx, leaseStore);\n",
            }
        );
    }
    if (usage.uses_logs)
    {
        helpers.push_back(
            JavaRuntimeRegistrationHelper{
                "registerLogDefinitionsTx",
                "        Log logSink\n",
                "        com.statespec.generated.descriptors.RuntimeLogRegistrationModule."
                "registerLogDefinitionsTx(tx, logSink);\n",
            }
        );
    }
    if (usage.uses_metrics)
    {
        helpers.push_back(
            JavaRuntimeRegistrationHelper{
                "registerMetricDefinitionsTx",
                "        Metric metricSink\n",
                "        com.statespec.generated.descriptors.RuntimeMetricRegistrationModule."
                "registerMetricDefinitionsTx(tx, metricSink);\n",
            }
        );
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        helpers.push_back(
            JavaRuntimeRegistrationHelper{
                "registerObservabilityCatalogTx",
                "        Log logSink,\n"
                "        Metric metricSink\n",
                "        "
                "com.statespec.generated.descriptors.RuntimeObservabilityRegistrationModule."
                "registerObservabilityCatalogTx(tx, logSink, metricSink);\n",
            }
        );
    }
    if (usage.uses_workflows)
    {
        helpers.push_back(
            JavaRuntimeRegistrationHelper{
                "registerWorkflowDefinitionsTx",
                "        Workflow workflowStore\n",
                "        com.statespec.generated.descriptors.RuntimeWorkflowRegistrationModule."
                "registerWorkflowDefinitionsTx(tx, workflowStore);\n",
            }
        );
    }
    return helpers;
}

std::string java_registration_helpers(
    const RuntimeDomainUsage& usage,
    const TemplatePackage&
)
{
    std::ostringstream out;
    for (const auto& helper : java_runtime_registration_helpers(usage))
    {
        out << "    public static void " << helper.method_name << "(\n";
        out << "        Backend.Transaction tx,\n";
        out << helper.parameters;
        out << "    ) throws Backend.BackendException {\n";
        out << helper.call;
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
    auto add = [&](const JavaRuntimeRegistrationDomain& domain)
    {
        params << domain.parameters;
        args << domain.arguments;
        calls << domain.calls;
    };
    for (const auto& domain : java_runtime_registration_domains(usage))
    {
        add(domain);
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
