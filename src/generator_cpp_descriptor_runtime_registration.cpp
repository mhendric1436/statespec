#include "generator_cpp_descriptor_areas.hpp"

#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>

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

    auto add_domain =
        [&](std::string_view tx_type, std::string_view name, std::string_view register_call)
    {
        tx_parameters << ",\n    " << tx_type << "& " << name;
        runtime_parameters << ",\n    " << tx_type << "& " << name;
        runtime_arguments << ", " << name;
        tx_calls << "    " << register_call << "(tx, " << name << ");\n";
    };

    if (usage.uses_feature_flags)
    {
        add_domain(
            "statespec::backend::IFeatureFlagStore", "feature_flag_store",
            "register_feature_flag_definitionsTx"
        );
    }
    if (usage.uses_queues)
    {
        add_domain(
            "statespec::backend::IQueueStore", "queue_store", "register_queue_definitionsTx"
        );
    }
    if (usage.uses_leases)
    {
        add_domain(
            "statespec::backend::ILeaseStore", "lease_store", "register_lease_definitionsTx"
        );
    }
    if (usage.uses_workflows)
    {
        add_domain(
            "statespec::backend::IWorkflowStore", "workflow_store",
            "register_workflow_definitionsTx"
        );
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        tx_parameters << ",\n    statespec::backend::ILogSink& log_sink";
        runtime_parameters << ",\n    statespec::backend::ILogSink& log_sink";
        tx_parameters << ",\n    statespec::backend::IMetricSink& metric_sink";
        runtime_parameters << ",\n    statespec::backend::IMetricSink& metric_sink";
        runtime_arguments << ", log_sink";
        runtime_arguments << ", metric_sink";
        tx_calls << "    register_observability_catalogTx(tx, log_sink, metric_sink);\n";
    }
    else
    {
        if (usage.uses_logs)
        {
            add_domain("statespec::backend::ILogSink", "log_sink", "register_log_definitionsTx");
        }
        if (usage.uses_metrics)
        {
            add_domain(
                "statespec::backend::IMetricSink", "metric_sink", "register_metric_definitionsTx"
            );
        }
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
