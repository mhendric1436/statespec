#include "generator_go_descriptor_areas.hpp"

#include "statespec/runtime_usage.hpp"

#include <sstream>
#include <vector>

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

struct GoRuntimeRegistrationDomain
{
    std::string_view parameters;
    std::string_view arguments;
    std::string_view calls;
};

std::vector<GoRuntimeRegistrationDomain>
go_runtime_registration_domains(const RuntimeDomainUsage& usage)
{
    std::vector<GoRuntimeRegistrationDomain> domains;
    if (usage.uses_feature_flags)
    {
        domains.push_back(
            GoRuntimeRegistrationDomain{
                ", featureFlagStore FeatureFlagStore",
                ", featureFlagStore",
                "\tif err := RegisterFeatureFlagDefinitionsTx(ctx, tx, featureFlagStore); err "
                "!= nil {\n"
                "\t\treturn err\n"
                "\t}\n",
            }
        );
    }
    if (usage.uses_queues)
    {
        domains.push_back(
            GoRuntimeRegistrationDomain{
                ", queueStore QueueStore",
                ", queueStore",
                "\tif err := RegisterQueueDefinitionsTx(ctx, tx, queueStore); err != nil {\n"
                "\t\treturn err\n"
                "\t}\n",
            }
        );
    }
    if (usage.uses_leases)
    {
        domains.push_back(
            GoRuntimeRegistrationDomain{
                ", leaseStore LeaseStore",
                ", leaseStore",
                "\tif err := RegisterLeaseDefinitionsTx(ctx, tx, leaseStore); err != nil {\n"
                "\t\treturn err\n"
                "\t}\n",
            }
        );
    }
    if (usage.uses_workflows)
    {
        domains.push_back(
            GoRuntimeRegistrationDomain{
                ", workflowStore WorkflowStore",
                ", workflowStore",
                "\tif err := RegisterWorkflowDefinitionsTx(ctx, tx, workflowStore); err != nil "
                "{\n"
                "\t\treturn err\n"
                "\t}\n",
            }
        );
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        domains.push_back(
            GoRuntimeRegistrationDomain{
                ", logSink LogSink, metricSink MetricSink",
                ", logSink, metricSink",
                "\tif err := RegisterObservabilityCatalogTx(ctx, tx, logSink, metricSink); err "
                "!= nil {\n"
                "\t\treturn err\n"
                "\t}\n",
            }
        );
    }
    else
    {
        if (usage.uses_logs)
        {
            domains.push_back(
                GoRuntimeRegistrationDomain{
                    ", logSink LogSink",
                    ", logSink",
                    "\tif err := RegisterLogDefinitionsTx(ctx, tx, logSink); err != nil {\n"
                    "\t\treturn err\n"
                    "\t}\n",
                }
            );
        }
        if (usage.uses_metrics)
        {
            domains.push_back(
                GoRuntimeRegistrationDomain{
                    ", metricSink MetricSink",
                    ", metricSink",
                    "\tif err := RegisterMetricDefinitionsTx(ctx, tx, metricSink); err != nil "
                    "{\n"
                    "\t\treturn err\n"
                    "\t}\n",
                }
            );
        }
    }
    return domains;
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
    ensure_collections_body << "\treturn backend.EnsureCollections(ctx, nil)\n";
    auto add = [&](const GoRuntimeRegistrationDomain& domain)
    {
        params << domain.parameters;
        args << domain.arguments;
        calls << domain.calls;
    };
    for (const auto& domain : go_runtime_registration_domains(usage))
    {
        add(domain);
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
