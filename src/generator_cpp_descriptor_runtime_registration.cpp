#include "generator_cpp_descriptor_areas.hpp"

#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>
#include <vector>

namespace statespec
{
namespace
{

std::string cpp_runtime_registration_snippet(
    const TemplatePackage& templates,
    std::string_view name
)
{
    return templates.load("generated/runtime_registration_" + std::string(name) + ".hpp.tmpl");
}

struct CppRuntimeRegistrationDomain
{
    std::string_view tx_parameters;
    std::string_view runtime_parameters;
    std::string_view runtime_arguments;
    std::string_view tx_calls;
};

std::vector<CppRuntimeRegistrationDomain>
cpp_runtime_registration_domains(const RuntimeDomainUsage& usage)
{
    std::vector<CppRuntimeRegistrationDomain> domains;
    if (usage.uses_feature_flags)
    {
        domains.push_back(
            CppRuntimeRegistrationDomain{
                ",\n    statespec::backend::IFeatureFlagStore& feature_flag_store",
                ",\n    statespec::backend::IFeatureFlagStore& feature_flag_store",
                ", feature_flag_store",
                "    register_feature_flag_definitionsTx(tx, feature_flag_store);\n",
            }
        );
    }
    if (usage.uses_queues)
    {
        domains.push_back(
            CppRuntimeRegistrationDomain{
                ",\n    statespec::backend::IQueueStore& queue_store",
                ",\n    statespec::backend::IQueueStore& queue_store",
                ", queue_store",
                "    register_queue_definitionsTx(tx, queue_store);\n",
            }
        );
    }
    if (usage.uses_leases)
    {
        domains.push_back(
            CppRuntimeRegistrationDomain{
                ",\n    statespec::backend::ILeaseStore& lease_store",
                ",\n    statespec::backend::ILeaseStore& lease_store",
                ", lease_store",
                "    register_lease_definitionsTx(tx, lease_store);\n",
            }
        );
    }
    if (usage.uses_workflows)
    {
        domains.push_back(
            CppRuntimeRegistrationDomain{
                ",\n    statespec::backend::IWorkflowStore& workflow_store",
                ",\n    statespec::backend::IWorkflowStore& workflow_store",
                ", workflow_store",
                "    register_workflow_definitionsTx(tx, workflow_store);\n",
            }
        );
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        domains.push_back(
            CppRuntimeRegistrationDomain{
                ",\n    statespec::backend::ILogSink& log_sink"
                ",\n    statespec::backend::IMetricSink& metric_sink",
                ",\n    statespec::backend::ILogSink& log_sink"
                ",\n    statespec::backend::IMetricSink& metric_sink",
                ", log_sink, metric_sink",
                "    register_observability_catalogTx(tx, log_sink, metric_sink);\n",
            }
        );
    }
    else
    {
        if (usage.uses_logs)
        {
            domains.push_back(
                CppRuntimeRegistrationDomain{
                    ",\n    statespec::backend::ILogSink& log_sink",
                    ",\n    statespec::backend::ILogSink& log_sink",
                    ", log_sink",
                    "    register_log_definitionsTx(tx, log_sink);\n",
                }
            );
        }
        if (usage.uses_metrics)
        {
            domains.push_back(
                CppRuntimeRegistrationDomain{
                    ",\n    statespec::backend::IMetricSink& metric_sink",
                    ",\n    statespec::backend::IMetricSink& metric_sink",
                    ", metric_sink",
                    "    register_metric_definitionsTx(tx, metric_sink);\n",
                }
            );
        }
    }
    return domains;
}

} // namespace

std::string generate_cpp_runtime_registration_domain(
    const TemplatePackage& templates,
    std::string_view name
)
{
    return cpp_runtime_registration_snippet(templates, name);
}

std::string generate_cpp_runtime_registration(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream tx_parameters;
    std::ostringstream runtime_parameters;
    std::ostringstream runtime_arguments;
    std::ostringstream tx_calls;
    std::ostringstream ensure_collections_body;

    ensure_collections_body << "    backend.ensure_collections({\n";
    for (const auto& entity : system.entities)
    {
        ensure_collections_body << "        ::statespec_generated::entities::"
                                << snake_identifier(entity.name)
                                << "::" << snake_identifier(entity.name)
                                << "_collection_descriptor(),\n";
    }
    ensure_collections_body << "    });\n";

    auto add_domain = [&](const CppRuntimeRegistrationDomain& domain)
    {
        tx_parameters << domain.tx_parameters;
        runtime_parameters << domain.runtime_parameters;
        runtime_arguments << domain.runtime_arguments;
        tx_calls << domain.tx_calls;
    };

    for (const auto& domain : cpp_runtime_registration_domains(usage))
    {
        add_domain(domain);
    }

    return templates.render(
        "generated/runtime_registration.hpp.tmpl",
        TemplateRenderer::Values{
            {"feature_flag_helpers", {}},
            {"lease_helpers", {}},
            {"domain_registration_helpers", {}},
            {"tx_parameters", tx_parameters.str()},
            {"runtime_parameters", runtime_parameters.str()},
            {"runtime_arguments", runtime_arguments.str()},
            {"ensure_system_collections_body", ensure_collections_body.str()},
            {"tx_calls", tx_calls.str()},
        }
    );
}

} // namespace statespec
