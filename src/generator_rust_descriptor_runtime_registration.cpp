#include "generator_rust_descriptor_areas.hpp"

#include "statespec/runtime_usage.hpp"

#include <sstream>

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
    auto add = [&](std::string_view generic, std::string_view bound, std::string_view name,
                   std::string_view fn)
    {
        generic_parameters << ", " << generic;
        generic_bounds << ",\n    " << generic << ": " << bound << "<B>";
        params << ",\n    " << name << ": &" << generic;
        args << ", " << name;
        calls << "    " << fn << "(tx, " << name << ")?;\n";
    };
    if (usage.uses_feature_flags)
    {
        add("F", "FeatureFlagStore", "feature_flag_store", "register_feature_flag_definitions_tx");
    }
    if (usage.uses_queues)
    {
        add("Q", "QueueStore", "queue_store", "register_queue_definitions_tx");
    }
    if (usage.uses_leases)
    {
        add("L", "LeaseStore", "lease_store", "register_lease_definitions_tx");
    }
    if (usage.uses_workflows)
    {
        add("W", "WorkflowStore", "workflow_store", "register_workflow_definitions_tx");
    }
    if (usage.uses_logs && usage.uses_metrics)
    {
        generic_parameters << ", LS, M";
        generic_bounds << ",\n    LS: LogSink<B>,\n    M: MetricSink<B>";
        params << ",\n    log_sink: &LS,\n    metric_sink: &M";
        args << ", log_sink, metric_sink";
        calls << "    register_observability_catalog_tx(tx, log_sink, metric_sink)?;\n";
    }
    else
    {
        if (usage.uses_logs)
        {
            add("LS", "LogSink", "log_sink", "register_log_definitions_tx");
        }
        if (usage.uses_metrics)
        {
            add("M", "MetricSink", "metric_sink", "register_metric_definitions_tx");
        }
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
            {"tx_calls", calls.str()},
        }
    );
}

} // namespace statespec
