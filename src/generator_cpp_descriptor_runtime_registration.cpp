#include "generator_cpp_descriptor_areas.hpp"

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

std::string cpp_observability_registration(
    const RuntimeDomainUsage& usage,
    const TemplatePackage& templates
)
{
    std::ostringstream out;
    if (usage.uses_logs)
    {
        out << cpp_runtime_registration_snippet(templates, "logs");
    }
    if (usage.uses_metrics)
    {
        out << cpp_runtime_registration_snippet(templates, "metrics");
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        out << cpp_runtime_registration_snippet(templates, "observability");
    }
    return out.str();
}

} // namespace

std::string generate_cpp_runtime_registration(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream helpers;
    std::ostringstream registrations;
    std::ostringstream tx_parameters;
    std::ostringstream runtime_parameters;
    std::ostringstream runtime_arguments;
    std::ostringstream tx_calls;

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
        registrations << cpp_runtime_registration_snippet(templates, "feature_flags");
        add_domain(
            "statespec::backend::IFeatureFlagStore", "feature_flag_store",
            "register_feature_flag_definitionsTx"
        );
    }
    if (usage.uses_queues)
    {
        registrations << cpp_runtime_registration_snippet(templates, "queues");
        add_domain(
            "statespec::backend::IQueueStore", "queue_store", "register_queue_definitionsTx"
        );
    }
    if (usage.uses_leases)
    {
        registrations << cpp_runtime_registration_snippet(templates, "leases");
        add_domain(
            "statespec::backend::ILeaseStore", "lease_store", "register_lease_definitionsTx"
        );
    }
    if (usage.uses_workflows)
    {
        registrations << cpp_runtime_registration_snippet(templates, "workflows");
        add_domain(
            "statespec::backend::IWorkflowStore", "workflow_store",
            "register_workflow_definitionsTx"
        );
    }
    registrations << cpp_observability_registration(usage, templates);
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
            {"feature_flag_helpers", helpers.str()},
            {"lease_helpers", {}},
            {"domain_registration_helpers", registrations.str()},
            {"tx_parameters", tx_parameters.str()},
            {"runtime_parameters", runtime_parameters.str()},
            {"runtime_arguments", runtime_arguments.str()},
            {"tx_calls", tx_calls.str()},
        }
    );
}

} // namespace statespec
