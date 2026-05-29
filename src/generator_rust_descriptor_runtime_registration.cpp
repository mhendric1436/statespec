#include "generator_rust_descriptor_areas.hpp"

#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>
#include <vector>

namespace statespec
{
namespace
{

std::string rust_runtime_registration_snippet(
    const TemplatePackage& templates,
    std::string_view name
)
{
    return templates.load("generated/runtime_registration_" + std::string(name) + ".rs.tmpl");
}

struct RustRuntimeRegistrationDomain
{
    std::string_view generic_parameters;
    std::string_view generic_bounds;
    std::string_view parameters;
    std::string_view arguments;
    std::string_view calls;
};

std::vector<RustRuntimeRegistrationDomain>
rust_runtime_registration_domains(const RuntimeDomainUsage& usage)
{
    std::vector<RustRuntimeRegistrationDomain> domains;
    if (usage.uses_feature_flags)
    {
        domains.push_back(
            RustRuntimeRegistrationDomain{
                ", F",
                ",\n    F: FeatureFlagStore<B>",
                ",\n    feature_flag_store: &F",
                ", feature_flag_store",
                "    register_feature_flag_definitions_tx(tx, feature_flag_store)?;\n",
            }
        );
    }
    if (usage.uses_queues)
    {
        domains.push_back(
            RustRuntimeRegistrationDomain{
                ", Q",
                ",\n    Q: QueueStore<B>",
                ",\n    queue_store: &Q",
                ", queue_store",
                "    register_queue_definitions_tx(tx, queue_store)?;\n",
            }
        );
    }
    if (usage.uses_leases)
    {
        domains.push_back(
            RustRuntimeRegistrationDomain{
                ", L",
                ",\n    L: LeaseStore<B>",
                ",\n    lease_store: &L",
                ", lease_store",
                "    register_lease_definitions_tx(tx, lease_store)?;\n",
            }
        );
    }
    if (usage.uses_workflows)
    {
        domains.push_back(
            RustRuntimeRegistrationDomain{
                ", W",
                ",\n    W: WorkflowStore<B>",
                ",\n    workflow_store: &W",
                ", workflow_store",
                "    register_workflow_definitions_tx(tx, workflow_store)?;\n",
            }
        );
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        domains.push_back(
            RustRuntimeRegistrationDomain{
                ", LS, M",
                ",\n    LS: LogSink<B>,\n    M: MetricSink<B>",
                ",\n    log_sink: &LS,\n    metric_sink: &M",
                ", log_sink, metric_sink",
                "    register_observability_catalog_tx(tx, log_sink, metric_sink)?;\n",
            }
        );
    }
    else
    {
        if (usage.uses_logs)
        {
            domains.push_back(
                RustRuntimeRegistrationDomain{
                    ", LS",
                    ",\n    LS: LogSink<B>",
                    ",\n    log_sink: &LS",
                    ", log_sink",
                    "    register_log_definitions_tx(tx, log_sink)?;\n",
                }
            );
        }
        if (usage.uses_metrics)
        {
            domains.push_back(
                RustRuntimeRegistrationDomain{
                    ", M",
                    ",\n    M: MetricSink<B>",
                    ",\n    metric_sink: &M",
                    ", metric_sink",
                    "    register_metric_definitions_tx(tx, metric_sink)?;\n",
                }
            );
        }
    }
    return domains;
}

} // namespace

std::string generate_rust_runtime_registration_domain(
    const TemplatePackage& templates,
    std::string_view name
)
{
    return rust_runtime_registration_snippet(templates, name);
}

std::string generate_rust_runtime_registration(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream generic_parameters;
    std::ostringstream generic_bounds;
    std::ostringstream params;
    std::ostringstream args;
    std::ostringstream calls;
    std::ostringstream ensure_collections_body;
    ensure_collections_body << "    backend.ensure_collections(&[\n";
    for (const auto& entity : system.entities)
    {
        ensure_collections_body << "        crate::entity_" << snake_identifier(entity.name)
                                << "::schema::" << snake_identifier(entity.name)
                                << "_collection_descriptor(),\n";
    }
    ensure_collections_body << "    ])\n";
    auto add = [&](const RustRuntimeRegistrationDomain& domain)
    {
        generic_parameters << domain.generic_parameters;
        generic_bounds << domain.generic_bounds;
        params << domain.parameters;
        args << domain.arguments;
        calls << domain.calls;
    };
    for (const auto& domain : rust_runtime_registration_domains(usage))
    {
        add(domain);
    }

    return templates.render(
        "generated/runtime_registration.rs.tmpl",
        TemplateRenderer::Values{
            {"feature_flag_helpers", {}},
            {"lease_helpers", {}},
            {"domain_registration_helpers", {}},
            {"generic_parameters", generic_parameters.str()},
            {"generic_bounds", generic_bounds.str()},
            {"tx_parameters", params.str()},
            {"runtime_parameters", params.str()},
            {"runtime_arguments", args.str()},
            {"ensure_system_collections_body", ensure_collections_body.str()},
            {"tx_calls", calls.str()},
        }
    );
}

} // namespace statespec
