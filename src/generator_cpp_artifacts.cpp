#include "generator_cpp_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_cpp_descriptor_areas.hpp"
#include "generator_cpp_descriptor_support.hpp"
#include "generator_cpp_descriptors.hpp"
#include "generator_cpp_package_artifacts.hpp"
#include "generator_entity_index_helpers.hpp"
#include "generator_support.hpp"
#include "generator_workflow_metadata.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

namespace statespec
{

namespace
{

std::vector<std::pair<
    std::string,
    std::string>>
cpp_runtime_registration_modules(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::vector<std::pair<std::string, std::string>> modules;
    auto add = [&](bool used, std::string name, std::string label)
    {
        if (used)
        {
            modules.emplace_back(std::move(name), std::move(label));
        }
    };
    add(usage.uses_feature_flags, "feature_flags", "feature flag runtime registration");
    add(usage.uses_queues, "queues", "queue runtime registration");
    add(usage.uses_leases, "leases", "lease runtime registration");
    add(usage.uses_logs, "logs", "log runtime registration");
    add(usage.uses_metrics, "metrics", "metric runtime registration");
    add(usage.uses_logs && usage.uses_metrics, "observability",
        "observability runtime registration");
    add(usage.uses_workflows, "workflows", "workflow runtime registration");
    return modules;
}

bool cpp_entity_uses_gc(const IrEntity& entity)
{
    return std::any_of(
        entity.states.begin(), entity.states.end(),
        [](const IrState& state) { return state.garbage_collection.has_value(); }
    );
}

std::string cpp_runtime_registration_includes(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream out;
    auto add = [&](bool used, std::string_view path)
    {
        if (used)
        {
            out << "#include \"" << path << "\"\n";
        }
    };
    add(usage.uses_feature_flags, "descriptors/runtime/feature_flags.hpp");
    add(usage.uses_queues, "descriptors/runtime/queues.hpp");
    add(usage.uses_leases, "descriptors/runtime/leases.hpp");
    add(usage.uses_logs, "descriptors/runtime/logs.hpp");
    add(usage.uses_metrics, "descriptors/runtime/metrics.hpp");
    add(usage.uses_logs && usage.uses_metrics, "descriptors/runtime/observability.hpp");
    add(usage.uses_workflows, "descriptors/runtime/workflows.hpp");
    return out.str();
}

TemplateRenderer::Values cpp_runtime_bootstrap_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream includes;
    std::ostringstream worker_domain_includes;
    std::ostringstream initializers;
    std::ostringstream members;
    std::ostringstream arguments;
    std::ostringstream worker_run_once;

    auto add = [&](bool used, std::string_view include, std::string_view type,
                   std::string_view member, std::string_view argument)
    {
        if (!used)
        {
            return;
        }
        includes << "#include \"" << include << "\"\n";
        initializers << ",\n          " << member << "()";
        members << "    " << type << " " << member << ";\n";
        arguments << ", " << argument;
    };
    add(usage.uses_feature_flags, "../common/runtime/feature_flag_store.hpp",
        "statespec::backend::runtime::RuntimeFeatureFlagStore", "feature_flags_", "feature_flags_");
    add(usage.uses_queues, "../common/runtime/queue_store.hpp",
        "statespec::backend::runtime::RuntimeQueueStore", "queues_", "queues_");
    add(usage.uses_leases, "../common/runtime/lease_store.hpp",
        "statespec::backend::runtime::RuntimeLeaseStore", "leases_", "leases_");
    add(usage.uses_workflows, "../common/runtime/workflow_store.hpp",
        "statespec::backend::runtime::RuntimeWorkflowStore", "workflows_", "workflows_");
    add(usage.uses_logs, "../common/runtime/log_sink.hpp",
        "statespec::backend::runtime::RuntimeLogSink", "logs_", "logs_");
    add(usage.uses_metrics, "../common/runtime/metric_sink.hpp",
        "statespec::backend::runtime::RuntimeMetricSink", "metrics_", "metrics_");
    if (usage.uses_queues)
    {
        worker_domain_includes << "#include \"worker_queues.hpp\"\n";
    }
    if (usage.uses_leases)
    {
        worker_domain_includes << "#include \"worker_leases.hpp\"\n";
    }
    if (usage.uses_workflows)
    {
        worker_domain_includes << "#include \"worker_workflows.hpp\"\n";
    }

    if (usage.uses_workflows)
    {
        worker_run_once
            << "    std::optional<statespec::backend::WorkflowExecutionRecord> run_once(\n"
            << "        const WorkerContext& context,\n"
            << "        const WorkflowStepInvokerMap& invokers,\n"
            << "        const std::string& workflow_execution_id\n"
            << "    )\n"
            << "    {\n"
            << "        if (!context.executes.has_value())\n"
            << "        {\n"
            << "            return std::nullopt;\n"
            << "        }\n"
            << "        WorkflowRunner runner{\n"
            << "            backend_, workflows_, invokers, context.worker_name, "
               "std::chrono::seconds{30}, 3\n"
            << "        };\n"
            << "        return runner.run_once(workflow_execution_id, *context.executes, 1);\n"
            << "    }\n";
    }
    else
    {
        worker_run_once
            << "    std::optional<statespec::backend::WorkflowExecutionRecord> run_once(\n"
            << "        const WorkerContext&,\n"
            << "        const WorkflowStepInvokerMap&,\n"
            << "        const std::string&\n"
            << "    )\n"
            << "    {\n"
            << "        return std::nullopt;\n"
            << "    }\n";
    }

    return TemplateRenderer::Values{
        {"runtime_store_includes", includes.str()},
        {"worker_domain_includes", worker_domain_includes.str()},
        {"runtime_store_initializers", initializers.str()},
        {"runtime_store_members", members.str()},
        {"runtime_bootstrap_arguments", arguments.str()},
        {"worker_runtime_run_once", worker_run_once.str()},
    };
}

std::string cpp_workflow_handler_class_name(const IrWorkflow& workflow)
{
    return pascal_identifier(workflow.name) + "V" + std::to_string(workflow.version.value_or(1)) +
           "StepHandler";
}

std::string cpp_workflow_default_handler_class_name(const IrWorkflow& workflow)
{
    return "Default" + cpp_workflow_handler_class_name(workflow);
}

std::string cpp_workflow_handler_namespace(const IrWorkflow& workflow)
{
    return snake_identifier(workflow.name);
}

std::string cpp_workflow_handler_methods(const IrWorkflow& workflow)
{
    std::ostringstream out;
    for (const auto& step : workflow.steps)
    {
        out << "    virtual ::statespec_generated::worker::WorkflowStepResult handle_"
            << snake_identifier(step.name) << "(\n";
        out << "        ::statespec::backend::ITransaction& tx,\n";
        out << "        const ::statespec_generated::worker::WorkflowStepHandlerContext& context\n";
        out << "    ) = 0;\n";
    }
    return out.str();
}

std::string cpp_workflow_default_handler_methods(const IrWorkflow& workflow)
{
    std::ostringstream out;
    for (const auto& step : workflow.steps)
    {
        out << "    ::statespec_generated::worker::WorkflowStepResult handle_"
            << snake_identifier(step.name) << "(\n";
        out << "        ::statespec::backend::ITransaction&,\n";
        out << "        const ::statespec_generated::worker::WorkflowStepHandlerContext&\n";
        out << "    ) override\n";
        out << "    {\n";
        out << "        throw std::runtime_error(\"generated workflow step handler "
            << workflow.name << "." << step.name << " is not implemented\");\n";
        out << "    }\n";
    }
    return out.str();
}

std::string generate_cpp_workflow_handler_module(
    const TemplatePackage& templates,
    const IrWorkflow& workflow
)
{
    const auto class_name = cpp_workflow_handler_class_name(workflow);
    const auto default_class_name = cpp_workflow_default_handler_class_name(workflow);
    const auto ns = cpp_workflow_handler_namespace(workflow);
    return templates.render(
        "worker/workflows/handlers.hpp.tmpl",
        TemplateRenderer::Values{
            {"workflow_namespace", ns},
            {"handler_class", class_name},
            {"default_handler_class", default_class_name},
            {"handler_methods", cpp_workflow_handler_methods(workflow)},
            {"default_handler_methods", cpp_workflow_default_handler_methods(workflow)},
        }
    );
}

std::string cpp_workflow_step_handler_includes(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        out << "#include \"workflows/" << snake_identifier(workflow.name) << "/handlers.hpp\"\n";
    }
    return out.str();
}

std::string cpp_workflow_step_handler_members(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        const auto ns = cpp_workflow_handler_namespace(workflow);
        const auto handler_class = cpp_workflow_handler_class_name(workflow);
        const auto default_class = cpp_workflow_default_handler_class_name(workflow);
        out << "    workflows::" << ns << "::" << default_class << " default_" << ns
            << "_handler_;\n";
        out << "    workflows::" << ns << "::" << handler_class << "* " << ns
            << "_handler_ = &default_" << ns << "_handler_;\n";
    }
    return out.str();
}

std::string cpp_workflow_step_handler_setters(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        const auto ns = cpp_workflow_handler_namespace(workflow);
        const auto handler_class = cpp_workflow_handler_class_name(workflow);
        out << "    void set_" << ns << "_handler(workflows::" << ns << "::" << handler_class
            << "& handler)\n";
        out << "    {\n";
        out << "        " << ns << "_handler_ = &handler;\n";
        out << "    }\n\n";
    }
    return out.str();
}

std::string cpp_workflow_step_handler_lookup(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        const auto ns = cpp_workflow_handler_namespace(workflow);
        out << "        if (workflow_name == " << cpp_string(workflow.name)
            << " && workflow_version == " << workflow.version.value_or(1) << ")\n";
        out << "        {\n";
        out << "            return " << ns << "_handler_;\n";
        out << "        }\n";
    }
    return out.str();
}

std::string cpp_worker_main_workflow_invoker_composition(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& workflow : system.workflows)
    {
        const auto ns = cpp_workflow_handler_namespace(workflow);
        const auto default_class = cpp_workflow_default_handler_class_name(workflow);
        out << "        statespec_generated::worker::workflows::" << ns << "::" << default_class
            << " " << ns << "_handler;\n";
        out << "        statespec_generated::worker::workflows::" << ns
            << "::register_workflow_step_invokers(invokers, " << ns << "_handler);\n";
    }
    return out.str();
}

std::string cpp_workflow_registry_invoker_entries(const IrWorkflow& workflow)
{
    std::ostringstream out;
    for (const auto& step : workflow.steps)
    {
        const auto step_snake = snake_identifier(step.name);
        out << "    invokers.emplace(\n";
        out << "        ::statespec_generated::worker::workflow_step_key("
            << cpp_string(workflow.name) << ", " << workflow.version.value_or(1) << ", "
            << cpp_string(step.name) << "),\n";
        out << "        [&handler](\n";
        out << "            ::statespec::backend::IBackend& backend,\n";
        out << "            ::statespec::backend::ITransaction& tx,\n";
        out << "            const ::statespec_generated::worker::WorkflowStepHandlerContext& "
               "context\n";
        out << "        ) {\n";
        out << "            (void)backend;\n";
        out << "            return handler.handle_" << step_snake << "(tx, context);\n";
        out << "        }\n";
        out << "    );\n";
    }
    for (const auto& phase : workflow_synthetic_child_phases(workflow))
    {
        out << "    invokers.emplace(\n";
        out << "        ::statespec_generated::worker::workflow_step_key("
            << cpp_string(workflow.name) << ", " << workflow.version.value_or(1) << ", "
            << cpp_string(phase.step_name) << "),\n";
        out << "        [](\n";
        out << "            ::statespec::backend::IBackend& backend,\n";
        out << "            ::statespec::backend::ITransaction& tx,\n";
        out << "            const ::statespec_generated::worker::WorkflowStepHandlerContext& "
               "context\n";
        out << "        ) {\n";
        out << "            (void)backend;\n";
        out << "            (void)tx;\n";
        out << "            (void)context;\n";
        if (phase.next_step.has_value())
        {
            out << "            return ::statespec_generated::worker::WorkflowStepResult::complete("
                << cpp_string(*phase.next_step) << ");\n";
        }
        else
        {
            out << "            return "
                   "::statespec_generated::worker::WorkflowStepResult::complete();\n";
        }
        out << "        }\n";
        out << "    );\n";
    }
    return out.str();
}

std::string generate_cpp_workflow_registry_module(
    const TemplatePackage& templates,
    const IrWorkflow& workflow
)
{
    const auto ns = cpp_workflow_handler_namespace(workflow);
    return templates.render(
        "worker/workflows/registry.hpp.tmpl",
        TemplateRenderer::Values{
            {"workflow_namespace", ns},
            {"handler_class", cpp_workflow_handler_class_name(workflow)},
            {"invoker_entries", cpp_workflow_registry_invoker_entries(workflow)},
        }
    );
}

std::string cpp_worker_registry_module(const IrWorker& worker)
{
    const auto snake = snake_identifier(worker.name);
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../worker_contexts.hpp\"\n";
    out << "#include \"../worker_descriptors.hpp\"\n\n";
    out << "#include <optional>\n";
    out << "#include <string_view>\n\n";
    out << "namespace statespec_generated::worker\n";
    out << "{\n\n";
    out << "inline std::optional<WorkerDescriptor> find_" << snake
        << "_worker_descriptor(std::string_view worker_name)\n";
    out << "{\n";
    out << "    if (worker_name == " << cpp_string(worker.name) << ")\n";
    out << "    {\n";
    out << "        return descriptors::" << snake << "_worker_descriptor();\n";
    out << "    }\n";
    out << "    return std::nullopt;\n";
    out << "}\n\n";
    out << "inline std::optional<WorkerContext> find_" << snake
        << "_worker_context(std::string_view worker_name)\n";
    out << "{\n";
    out << "    if (worker_name == " << cpp_string(worker.name) << ")\n";
    out << "    {\n";
    out << "        return descriptors::" << snake << "_worker_context();\n";
    out << "    }\n";
    out << "    return std::nullopt;\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::worker\n";
    return out.str();
}

std::string cpp_worker_registry_facade(const IrSystem& system)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    for (const auto& worker : system.workers)
    {
        out << "#include \"registry/" << snake_identifier(worker.name) << ".hpp\"\n";
    }
    if (!system.workflows.empty())
    {
        out << "#include \"workflow_step_handlers.hpp\"\n";
        for (const auto& workflow : system.workflows)
        {
            out << "#include \"workflows/" << snake_identifier(workflow.name)
                << "/registry.hpp\"\n";
        }
    }
    out << "\n#include <optional>\n";
    out << "#include <string>\n";
    out << "#include <string_view>\n\n";
    out << "namespace statespec_generated::worker\n";
    out << "{\n\n";
    out << "inline std::optional<WorkerDescriptor> find_worker_descriptor(std::string_view "
           "worker_name)\n";
    out << "{\n";
    for (const auto& worker : system.workers)
    {
        out << "    if (auto worker = find_" << snake_identifier(worker.name)
            << "_worker_descriptor(worker_name))\n";
        out << "    {\n";
        out << "        return worker;\n";
        out << "    }\n";
    }
    out << "    return std::nullopt;\n";
    out << "}\n\n";
    out << "inline std::optional<WorkerContext> find_worker_context(std::string_view "
           "worker_name)\n";
    out << "{\n";
    for (const auto& worker : system.workers)
    {
        out << "    if (auto context = find_" << snake_identifier(worker.name)
            << "_worker_context(worker_name))\n";
        out << "    {\n";
        out << "        return context;\n";
        out << "    }\n";
    }
    out << "    return std::nullopt;\n";
    out << "}\n\n";
    if (!system.workflows.empty())
    {
        out << "inline WorkflowStepInvokerMap workflow_step_invokers(WorkflowStepHandlerBundle& "
               "handlers)\n";
        out << "{\n";
        out << "        WorkflowStepInvokerMap values;\n";
        for (const auto& workflow : system.workflows)
        {
            const auto handler_class = cpp_workflow_handler_class_name(workflow);
            out << "        if (auto* handler = static_cast<workflows::"
                << cpp_workflow_handler_namespace(workflow) << "::" << handler_class
                << "*>(handlers.workflow_handler(" << cpp_string(workflow.name) << ", "
                << workflow.version.value_or(1) << ")))\n";
            out << "        {\n";
            out << "            workflows::" << cpp_workflow_handler_namespace(workflow)
                << "::register_workflow_step_invokers(values, *handler);\n";
            out << "        }\n";
        }
        out << "        return values;\n";
        out << "}\n\n";
    }
    out << "} // namespace statespec_generated::worker\n";
    return out.str();
}

TemplateRenderer::Values cpp_workflow_runner_values(const IrSystem& system)
{
    (void)system;
    return TemplateRenderer::Values{};
}

TemplateRenderer::Values cpp_runtime_codec_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream includes;
    auto add = [&](bool used, std::string_view include)
    {
        if (used)
        {
            includes << "#include \"" << include << "\"\n";
        }
    };
    add(usage.uses_feature_flags, "codec_feature_flags.hpp");
    add(usage.uses_queues, "codec_queues.hpp");
    add(usage.uses_leases, "codec_leases.hpp");
    add(usage.uses_workflows, "codec_workflows.hpp");
    add(usage.uses_observability, "codec_observability.hpp");
    return TemplateRenderer::Values{{"runtime_codec_includes", includes.str()}};
}

struct ApiHandlerDomain
{
    std::string name;
    std::vector<IrApi> apis;
};

bool api_output_mentions_entity(
    const IrApi& api,
    const IrEntity& entity
)
{
    return api.output.has_value() && api.output->find(entity.name) != std::string::npos;
}

bool api_input_mentions_entity(
    const IrApi& api,
    const IrEntity& entity
)
{
    return api.input.has_value() && api.input->find(entity.name) != std::string::npos;
}

bool api_name_mentions_entity(
    const IrApi& api,
    const IrEntity& entity
)
{
    return api.name.find(entity.name) != std::string::npos;
}

std::vector<ApiHandlerDomain> api_handler_domains(const IrSystem& system)
{
    std::vector<ApiHandlerDomain> domains;
    for (const auto& entity : system.entities)
    {
        domains.push_back(ApiHandlerDomain{entity.name, {}});
    }
    ApiHandlerDomain operations{"Operations", {}};
    for (const auto& api : system.apis)
    {
        bool assigned = false;
        auto assign_matching_entity = [&](auto matches)
        {
            for (std::size_t i = 0; i < system.entities.size(); ++i)
            {
                if (matches(api, system.entities[i]))
                {
                    domains[i].apis.push_back(api);
                    assigned = true;
                    return;
                }
            }
        };
        assign_matching_entity(api_output_mentions_entity);
        if (!assigned)
        {
            assign_matching_entity(api_input_mentions_entity);
        }
        if (!assigned)
        {
            assign_matching_entity(api_name_mentions_entity);
        }
        if (!assigned)
        {
            operations.apis.push_back(api);
        }
    }
    domains.erase(
        std::remove_if(
            domains.begin(), domains.end(), [](const auto& domain) { return domain.apis.empty(); }
        ),
        domains.end()
    );
    if (!operations.apis.empty())
    {
        domains.push_back(std::move(operations));
    }
    return domains;
}

IrSystem with_domain_apis(
    const IrSystem& system,
    const std::vector<IrApi>& apis
)
{
    auto filtered = system;
    filtered.apis = apis;
    return filtered;
}

std::string cpp_api_handler_domain_class_name(std::string_view domain_name)
{
    return "Default" + pascal_identifier(std::string{domain_name}) + "ApiHandlerRegistry";
}

std::string cpp_api_handler_domain_path(std::string_view domain_name)
{
    return "api/entities/" + snake_identifier(std::string{domain_name}) + "/handlers.hpp";
}

std::string cpp_api_handler_domain_registry_path(std::string_view domain_name)
{
    return "api/entities/" + snake_identifier(std::string{domain_name}) + "/registry.hpp";
}

std::string cpp_api_descriptor_function_name(const IrApi& api);
std::string cpp_api_route_descriptor_function_name(const IrApi& api);

std::string cpp_entity_api_catalog_path(std::string_view entity_name)
{
    return "api/entities/" + snake_identifier(std::string{entity_name}) + "/catalog.hpp";
}

std::string cpp_api_handler_domain_registry_include_path(std::string_view domain_name)
{
    return "entities/" + snake_identifier(std::string{domain_name}) + "/registry.hpp";
}

bool is_entity_crud_api(const IrApi& api);

std::string cpp_api_handler_dispatcher_domain_includes(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        out << "#include \"" << cpp_api_handler_domain_registry_include_path(domain.name) << "\"\n";
    }
    return out.str();
}

std::string cpp_api_handler_lookup_registrations(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        out << "        ::statespec_generated::api::entities::" << snake_identifier(domain.name)
            << "::register_handler_invokers(handlers);\n";
    }
    return out.str();
}

std::string cpp_business_api_handler_lookup_entries(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        if (is_entity_crud_api(api))
        {
            continue;
        }
        out << "        handlers.emplace(" << cpp_string(api.name)
            << ", [](IBusinessApiOperationHandler& handler, const ApiRequestContext& context) {\n";
        out << "            return handler.handle_" << snake_identifier(api.name) << "(context);\n";
        out << "        });\n";
    }
    return out.str();
}

bool is_entity_crud_api(const IrApi& api)
{
    return api.entity.has_value() && api.repository_operation.has_value();
}

std::vector<ApiHandlerDomain> crud_api_handler_domains(const std::vector<ApiHandlerDomain>& domains)
{
    std::vector<ApiHandlerDomain> result;
    for (const auto& domain : domains)
    {
        for (const auto& api : domain.apis)
        {
            if (is_entity_crud_api(api))
            {
                auto target = std::find_if(
                    result.begin(), result.end(),
                    [&](const auto& candidate) { return candidate.name == *api.entity; }
                );
                if (target == result.end())
                {
                    result.push_back(ApiHandlerDomain{*api.entity, {api}});
                }
                else
                {
                    target->apis.push_back(api);
                }
            }
        }
    }
    return result;
}

std::string cpp_api_handler_domain_registry_file(const ApiHandlerDomain& domain)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../../api_handler_registry_support.hpp\"\n";
    out << "#include \"handlers.hpp\"\n";
    out << "\n";
    out << "namespace statespec_generated::api::entities::" << snake_identifier(domain.name)
        << "\n";
    out << "{\n\n";
    out << "inline void register_handler_invokers(::statespec_generated::api::ApiHandlerLookupMap& "
           "handlers)\n";
    out << "{\n";
    for (const auto& api : domain.apis)
    {
        out << "    handlers.emplace(" << cpp_string(api.name)
            << ", [](statespec::backend::IBackend& backend, const ApiRequestContext& context) {\n";
        out << "        ::statespec_generated::api::"
            << cpp_api_handler_domain_class_name(domain.name) << " handler{backend};\n";
        out << "        return handler.handle_" << snake_identifier(api.name) << "(context);\n";
        out << "    });\n";
    }
    out << "}\n\n";
    out << "} // namespace statespec_generated::api::entities::" << snake_identifier(domain.name)
        << "\n";
    return out.str();
}

std::string cpp_api_handler_domain_file(
    const IrSystem& system,
    const ApiHandlerDomain& domain
)
{
    const auto filtered = with_domain_apis(system, domain.apis);
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../../api_codecs.hpp\"\n";
    out << "#include \"../../api_handler_registry_support.hpp\"\n";
    for (const auto& entity : system.entities)
    {
        out << "#include \"../../../common/entities/" << snake_identifier(entity.name)
            << "/persistence.hpp\"\n";
    }
    out << "\n";
    out << "namespace statespec_generated::api\n";
    out << "{\n\n";
    out << "class " << cpp_api_handler_domain_class_name(domain.name) << " final\n";
    out << "{\n";
    out << "  public:\n";
    out << "    explicit " << cpp_api_handler_domain_class_name(domain.name)
        << "(statespec::backend::IBackend& backend)\n";
    out << "        : backend_(backend)\n";
    out << "    {\n";
    out << "    }\n\n";
    out << generate_api_operation_default_handler_domain_methods(filtered);
    out << "  private:\n";
    out << "    [[maybe_unused]] statespec::backend::IBackend& backend_;\n";
    out << "};\n\n";
    out << "} // namespace statespec_generated::api\n";
    return out.str();
}

std::vector<IrShape> api_codec_shapes(const IrSystem& system)
{
    std::vector<IrShape> shapes;
    for (const auto& shape : system.shapes)
    {
        const auto used = std::any_of(
            system.apis.begin(), system.apis.end(),
            [&](const auto& api)
            {
                return (api.input.has_value() && *api.input == shape.name) ||
                       (api.output.has_value() && *api.output == shape.name);
            }
        );
        if (used)
        {
            shapes.push_back(shape);
        }
    }
    return shapes;
}

bool is_api_contract_shape(
    const IrSystem& system,
    std::string_view shape_name
)
{
    return std::any_of(
        system.apis.begin(), system.apis.end(),
        [&](const auto& api)
        {
            return (api.input.has_value() && *api.input == shape_name) ||
                   (api.output.has_value() && *api.output == shape_name);
        }
    );
}

std::optional<std::string> entity_api_shape_owner(
    const IrSystem& system,
    std::string_view shape_name
)
{
    for (const auto& api : system.apis)
    {
        if (!api.entity.has_value() || !api.repository_operation.has_value())
        {
            continue;
        }
        if ((api.input.has_value() && *api.input == shape_name) ||
            (api.output.has_value() && *api.output == shape_name))
        {
            return *api.entity;
        }
    }
    return std::nullopt;
}

std::vector<IrShape> entity_api_shapes(
    const IrSystem& system,
    std::string_view entity_name
)
{
    std::vector<IrShape> shapes;
    for (const auto& shape : system.shapes)
    {
        const auto owner = entity_api_shape_owner(system, shape.name);
        if (owner.has_value() && *owner == entity_name)
        {
            shapes.push_back(shape);
        }
    }
    return shapes;
}

IrSystem with_shapes_matching(
    const IrSystem& system,
    bool api_contract_shapes
)
{
    auto filtered = system;
    filtered.shapes.clear();
    for (const auto& shape : system.shapes)
    {
        if (is_api_contract_shape(system, shape.name) == api_contract_shapes)
        {
            filtered.shapes.push_back(shape);
        }
    }
    return filtered;
}

IrSystem api_contract_shape_system(const IrSystem& system)
{
    return with_shapes_matching(system, true);
}

IrSystem common_shape_system(const IrSystem& system)
{
    return with_shapes_matching(system, false);
}

const IrShape* find_shape(
    const IrSystem& system,
    std::string_view shape_name
)
{
    const auto it = std::find_if(
        system.shapes.begin(), system.shapes.end(),
        [&](const auto& shape) { return shape.name == shape_name; }
    );
    return it == system.shapes.end() ? nullptr : &*it;
}

IrSystem with_codec_shape_apis(
    const IrSystem& system,
    std::string_view shape_name
)
{
    auto filtered = system;
    filtered.apis.clear();
    for (const auto& api : system.apis)
    {
        IrApi scoped = api;
        if (!scoped.input.has_value() || *scoped.input != shape_name)
        {
            scoped.input.reset();
        }
        if (!scoped.output.has_value() || *scoped.output != shape_name)
        {
            scoped.output.reset();
        }
        if (scoped.input.has_value() || scoped.output.has_value())
        {
            filtered.apis.push_back(std::move(scoped));
        }
    }
    return filtered;
}

std::string cpp_api_codec_shape_path(std::string_view shape_name)
{
    return "api/codecs/" + snake_identifier(std::string{shape_name}) + ".hpp";
}

std::string cpp_entity_api_codec_path(std::string_view entity_name)
{
    return "api/entities/" + snake_identifier(std::string{entity_name}) + "/codecs.hpp";
}

IrSystem with_codec_shapes_apis(
    const IrSystem& system,
    const std::vector<IrShape>& shapes
)
{
    auto filtered = system;
    filtered.apis.clear();
    std::set<std::string> shape_names;
    for (const auto& shape : shapes)
    {
        shape_names.insert(shape.name);
    }
    for (const auto& api : system.apis)
    {
        IrApi scoped = api;
        if (!scoped.input.has_value() || shape_names.count(*scoped.input) == 0)
        {
            scoped.input.reset();
        }
        if (!scoped.output.has_value() || shape_names.count(*scoped.output) == 0)
        {
            scoped.output.reset();
        }
        if (scoped.input.has_value() || scoped.output.has_value())
        {
            filtered.apis.push_back(std::move(scoped));
        }
    }
    return filtered;
}

std::string cpp_api_codec_includes(const IrSystem& system)
{
    std::ostringstream out;
    out << "#include \"api_codec_support.hpp\"\n";
    std::set<std::string> included_entities;
    for (const auto& shape : api_codec_shapes(system))
    {
        const auto owner = entity_api_shape_owner(system, shape.name);
        if (owner.has_value())
        {
            if (included_entities.insert(*owner).second)
            {
                out << "#include \"entities/" << snake_identifier(*owner) << "/codecs.hpp\"\n";
            }
            continue;
        }
        out << "#include \"codecs/" << snake_identifier(shape.name) << ".hpp\"\n";
    }
    return out.str();
}

std::string cpp_api_codec_shape_file(
    const IrSystem& system,
    const IrShape& shape
)
{
    const auto filtered = with_codec_shape_apis(system, shape.name);
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../api_codec_support.hpp\"\n";
    out << "#include \"../shapes/" << snake_identifier(shape.name) << ".hpp\"\n\n";
    out << "namespace statespec_generated::api\n";
    out << "{\n\n";
    out << generate_api_codec_operations(filtered);
    out << "} // namespace statespec_generated::api\n";
    return out.str();
}

std::string cpp_entity_api_codec_file(
    const IrSystem& system,
    const IrEntity& entity
)
{
    const auto shapes = entity_api_shapes(system, entity.name);
    const auto filtered = with_codec_shapes_apis(system, shapes);
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../../api_codec_support.hpp\"\n";
    out << "#include \"../../../common/entities/" << snake_identifier(entity.name)
        << "/constants.hpp\"\n";
    out << "#include \"shapes.hpp\"\n\n";
    out << "namespace statespec_generated::api\n";
    out << "{\n\n";
    out << generate_api_codec_operations(filtered);
    out << "} // namespace statespec_generated::api\n";
    return out.str();
}

std::string cpp_entity_gc_descriptor_header(const IrEntity& entity)
{
    std::ostringstream terminal_states;
    for (const auto& state : entity.states)
    {
        if (!state.garbage_collection.has_value())
        {
            continue;
        }
        terminal_states
            << "                ::statespec::backend::runtime::EntityGcTerminalStateDescriptor{"
            << "constants::" << cpp_entity_state_constant_name(entity.name, state.name) << ", "
            << "k" << pascal_identifier(entity.name) << "State" << pascal_identifier(state.name)
            << "GcAfter, "
            << "k" << pascal_identifier(entity.name) << "State" << pascal_identifier(state.name)
            << "GcMode},\n";
    }

    const auto entity_namespace = snake_identifier(entity.name);
    const auto descriptor_function = snake_identifier(entity.name) + "_entity_gc_descriptor";
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"constants.hpp\"\n";
    out << "#include \"schema.hpp\"\n";
    out << "#include \"../../runtime/entity_gc_types.hpp\"\n\n";
    out << "namespace statespec_generated::entities::" << entity_namespace << "\n";
    out << "{\n\n";
    out << "inline ::statespec::backend::runtime::EntityGcDescriptor " << descriptor_function
        << "()\n";
    out << "{\n";
    out << "    return ::statespec::backend::runtime::EntityGcDescriptor{\n";
    out << "        constants::" << cpp_entity_name_constant_name(entity.name) << ",\n";
    out << "        constants::" << cpp_entity_name_constant_name(entity.name) << ",\n";
    out << "        constants::" << cpp_entity_field_constant_name(entity.name, "status") << ",\n";
    out << "        std::vector<::statespec::backend::runtime::EntityGcTerminalStateDescriptor>{\n";
    out << terminal_states.str();
    out << "        }\n";
    out << "    };\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::entities::" << entity_namespace << "\n";
    return out.str();
}

std::string cpp_api_entity_gc_catalog_header(const IrSystem& system)
{
    std::ostringstream includes;
    std::ostringstream descriptor_calls;
    for (const auto& entity : system.entities)
    {
        if (!cpp_entity_uses_gc(entity))
        {
            continue;
        }
        includes << "#include \"../common/entities/" << snake_identifier(entity.name)
                 << "/gc.hpp\"\n";
        descriptor_calls << "        ::statespec_generated::entities::"
                         << snake_identifier(entity.name) << "::" << snake_identifier(entity.name)
                         << "_entity_gc_descriptor(),\n";
    }

    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../common/runtime/entity_gc_types.hpp\"\n";
    out << includes.str() << "\n";
    out << "#include <vector>\n\n";
    out << "namespace statespec_generated::api\n";
    out << "{\n\n";
    out << "inline std::vector<::statespec::backend::runtime::EntityGcDescriptor> "
           "api_entity_gc_descriptors()\n";
    out << "{\n";
    out << "    return std::vector<::statespec::backend::runtime::EntityGcDescriptor>{\n";
    out << descriptor_calls.str();
    out << "    };\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::api\n";
    return out.str();
}

std::string cpp_worker_entity_gc_catalog_header(const IrSystem& system)
{
    std::ostringstream includes;
    std::ostringstream descriptor_calls;
    for (const auto& entity : system.entities)
    {
        if (!cpp_entity_uses_gc(entity))
        {
            continue;
        }
        includes << "#include \"../common/entities/" << snake_identifier(entity.name)
                 << "/gc.hpp\"\n";
        descriptor_calls << "        ::statespec_generated::entities::"
                         << snake_identifier(entity.name) << "::" << snake_identifier(entity.name)
                         << "_entity_gc_descriptor(),\n";
    }

    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../common/runtime/entity_gc_types.hpp\"\n";
    out << includes.str() << "\n";
    out << "#include <vector>\n\n";
    out << "namespace statespec_generated::worker\n";
    out << "{\n\n";
    out << "inline std::vector<::statespec::backend::runtime::EntityGcDescriptor> "
           "worker_entity_gc_descriptors()\n";
    out << "{\n";
    out << "    return std::vector<::statespec::backend::runtime::EntityGcDescriptor>{\n";
    out << descriptor_calls.str();
    out << "    };\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::worker\n";
    return out.str();
}

TemplateRenderer::Values cpp_api_runtime_values(const IrSystem& system)
{
    auto values = cpp_runtime_bootstrap_values(system);
    values.erase("worker_runtime_run_once");
    values.erase("worker_domain_includes");
    return values;
}

TemplateRenderer::Values cpp_api_main_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    if (!usage.uses_entity_gc)
    {
        return TemplateRenderer::Values{
            {"api_main_entity_gc_include", ""},
            {"api_main_entity_gc_registration", ""},
        };
    }
    return TemplateRenderer::Values{
        {"api_main_entity_gc_include", "#include \"../common/runtime/entity_gc_registration.hpp\"\n"
                                       "#include \"entity_gc_catalog.hpp\"\n"},
        {"api_main_entity_gc_registration",
         "        statespec::backend::runtime::register_entity_gc_workers(\n"
         "            process, backend, statespec_generated::api::api_entity_gc_descriptors()\n"
         "        );\n\n"},
    };
}

TemplateRenderer::Values cpp_worker_main_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    const auto workflow_invoker_composition = cpp_worker_main_workflow_invoker_composition(system);
    if (!usage.uses_entity_gc)
    {
        return TemplateRenderer::Values{
            {"worker_main_entity_gc_include", ""},
            {"worker_main_entity_gc_registration", ""},
            {"worker_main_workflow_invoker_composition", workflow_invoker_composition},
        };
    }
    return TemplateRenderer::Values{
        {"worker_main_entity_gc_include",
         "#include \"../common/runtime/entity_gc_registration.hpp\"\n"
         "#include \"entity_gc_catalog.hpp\"\n"},
        {"worker_main_entity_gc_registration",
         "        statespec::backend::runtime::register_entity_gc_workers(\n"
         "            runtime, backend, "
         "statespec_generated::worker::worker_entity_gc_descriptors()\n"
         "        );\n\n"},
        {"worker_main_workflow_invoker_composition", workflow_invoker_composition},
    };
}

void add_cpp_common_generated_template_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    std::string_view relative_output_path,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {}
)
{
    add_generated_template_file(
        result, options.output_dir, templates, template_path_for_output(relative_output_path),
        common_artifact_path(relative_output_path), diagnostics, GeneratedArtifactTier::Common,
        values
    );
}

TemplateRenderer::Values cpp_descriptor_module_values(std::string_view module_name)
{
    return TemplateRenderer::Values{
        {"descriptor_module_name", std::string{module_name}},
        {"descriptor_module_content", ""},
    };
}

std::string cpp_descriptor_types_header(const TemplatePackage& templates)
{
    return generate_cpp_descriptor_types_header(templates);
}

std::string cpp_shape_types_header()
{
    return generate_cpp_shape_types_header();
}

std::string replace_all_copy(
    std::string value,
    std::string_view from,
    std::string_view to
)
{
    std::size_t pos = 0;
    while ((pos = value.find(from, pos)) != std::string::npos)
    {
        value.replace(pos, from.size(), to);
        pos += to.size();
    }
    return value;
}

std::string cpp_entity_centered_facade_header(
    const TemplatePackage& templates,
    const IrEntity& entity,
    std::string_view area
)
{
    const auto entity_path = snake_identifier(entity.name);
    const auto entity_namespace = entity_path;
    std::ostringstream out;
    out << "#pragma once\n\n";
    if (area == "model")
    {
        out << "#include \"constants.hpp\"\n";
        out << "#include \"../../backend.hpp\"\n\n";
        out << "#include <vector>\n\n";
    }
    else if (area == "schema")
    {
        out << "#include \"model.hpp\"\n";
        out << "#include \"../../backend.hpp\"\n\n";
        out << "#include <cstdint>\n";
        out << "#include <vector>\n\n";
    }
    else if (area == "persistence")
    {
        out << "#include \"model.hpp\"\n";
        out << "#include \"schema.hpp\"\n";
        out << "#include \"../../entity_repository.hpp\"\n\n";
        out << "#include <optional>\n";
        out << "#include <string>\n";
        out << "#include <utility>\n";
        out << "#include <vector>\n\n";
    }
    out << "namespace statespec_generated::entities::" << entity_namespace << "\n";
    out << "{\n\n";
    if (area == "model")
    {
        out << "using namespace constants;\n\n";
        out << "inline std::vector<statespec::backend::FieldDescriptor> " << entity_path
            << "_field_descriptors()\n";
        out << "{\n";
        out << "    return {\n";
        for (const auto& field : entity.fields)
        {
            out << "        " << cpp_entity_field_descriptor_expr(entity.name, field) << ",\n";
        }
        out << "    };\n";
        out << "}\n\n";
        out << "inline std::vector<statespec::backend::IndexDescriptor> " << entity_path
            << "_index_descriptors()\n";
        out << "{\n";
        out << "    return {\n";
        for (const auto& index : entity.indexes)
        {
            out << "        statespec::backend::IndexDescriptor{\n";
            out << "            " << cpp_entity_index_constant_name(entity.name, index.name)
                << ",\n";
            out << "            {";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << cpp_entity_field_constant_name(entity.name, index.fields[i]);
            }
            out << "},\n";
            out << "            " << (index.unique ? "true" : "false") << ",\n";
            out << "        },\n";
        }
        out << "    };\n";
        out << "}\n\n";
        out << "// Relationship metadata is rooted with the entity model.\n";
        out << "// common/descriptors.hpp composes entity descriptor catalogs.\n\n";
    }
    else if (area == "schema")
    {
        out << "using namespace constants;\n\n";
        out << "inline constexpr std::uint64_t k" << pascal_identifier(entity.name)
            << "SchemaVersion = 1;\n";
        if (entity.ownership.has_value())
        {
            out << "inline constexpr const char* k" << pascal_identifier(entity.name)
                << "OwnershipAuthority = " << cpp_string(entity.ownership->authority) << ";\n";
            out << "inline constexpr const char* k" << pascal_identifier(entity.name)
                << "OwnershipSystemOfRecord = " << cpp_string(entity.ownership->system_of_record)
                << ";\n";
            out << "inline constexpr const char* k" << pascal_identifier(entity.name)
                << "OwnershipLifecycle = " << cpp_string(entity.ownership->lifecycle) << ";\n";
        }
        for (const auto& state : entity.states)
        {
            out << "inline constexpr bool k" << pascal_identifier(entity.name) << "State"
                << pascal_identifier(state.name)
                << "Terminal = " << (state.terminal ? "true" : "false") << ";\n";
            if (state.garbage_collection.has_value())
            {
                out << "inline constexpr const char* k" << pascal_identifier(entity.name) << "State"
                    << pascal_identifier(state.name)
                    << "GcAfter = " << cpp_string(state.garbage_collection->after) << ";\n";
                out << "inline constexpr const char* k" << pascal_identifier(entity.name) << "State"
                    << pascal_identifier(state.name)
                    << "GcMode = " << cpp_string(state.garbage_collection->mode) << ";\n";
            }
        }
        out << "\n";
        out << "inline statespec::backend::CollectionDescriptor " << entity_path
            << "_collection_descriptor()\n";
        out << "{\n";
        out << "    return statespec::backend::CollectionDescriptor{\n";
        out << "        " << cpp_entity_name_constant_name(entity.name) << ",\n";
        out << "        " << entity_path << "_field_descriptors(),\n";
        out << "        {";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "},\n";
        out << "        " << entity_path << "_index_descriptors(),\n";
        out << "        k" << pascal_identifier(entity.name) << "SchemaVersion,\n";
        out << "    };\n";
        out << "}\n\n";
        out << "// Collection schema and compatibility metadata are rooted with the entity "
               "schema.\n";
        out << "// common/descriptors.hpp composes entity descriptor catalogs.\n\n";
    }
    else if (area == "persistence")
    {
        out << "using namespace constants;\n\n";
        const auto type_name = pascal_identifier(entity.name);
        out << "inline ::statespec_generated::EntityLookup " << entity_path
            << "_lookup(std::vector<::statespec_generated::EntityKeyValue> key_values)\n";
        out << "{\n";
        out << "    return ::statespec_generated::EntityLookup{\n";
        out << "        " << cpp_entity_name_constant_name(entity.name) << ",\n";
        out << "        {";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "},\n";
        out << "        std::move(key_values),\n";
        out << "    };\n";
        out << "}\n\n";
        out << "inline ::statespec_generated::EntityDescriptor " << entity_path
            << "_entity_descriptor()\n";
        out << "{\n";
        out << "    return ::statespec_generated::EntityDescriptor{\n";
        out << "        " << cpp_entity_name_constant_name(entity.name) << ",\n";
        out << "        {";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << cpp_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "},\n";
        out << "        std::nullopt,\n";
        out << "        {},\n";
        out << "        {},\n";
        out << "        {},\n";
        out << "        {},\n";
        out << "        std::nullopt,\n";
        out << "        {},\n";
        out << "    };\n";
        out << "}\n\n";
        out << "class I" << type_name << "Repository\n";
        out << "{\n";
        out << "  public:\n";
        out << "    virtual ~I" << type_name << "Repository() = default;\n";
        out << "    virtual void register_descriptor(statespec::backend::IBackend& backend) = 0;\n";
        out << "    virtual std::optional<statespec::backend::VersionedRecord> createTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        statespec::backend::Json document\n";
        out << "    ) = 0;\n";
        out << "    virtual std::optional<statespec::backend::VersionedRecord> getTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        std::vector<::statespec_generated::EntityKeyValue> key_values\n";
        out << "    ) = 0;\n";
        out << "    virtual std::vector<statespec::backend::VersionedRecord> listByIndexTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        std::string index_name,\n";
        out << "        std::vector<statespec::backend::IndexValue> values\n";
        out << "    ) = 0;\n";
        for (const auto& index : entity.indexes)
        {
            out << "    virtual std::vector<statespec::backend::VersionedRecord> "
                << entity_index_repository_method_name(index.name) << "(\n";
            out << "        statespec::backend::ITransaction& tx,\n";
            out << "        std::vector<statespec::backend::IndexValue> values\n";
            out << "    ) = 0;\n";
        }
        out << "    virtual std::optional<statespec::backend::VersionedRecord> updateTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        std::vector<::statespec_generated::EntityKeyValue> key_values,\n";
        out << "        statespec::backend::Json document,\n";
        out << "        statespec::backend::Version expected_version\n";
        out << "    ) = 0;\n";
        out << "};\n\n";
        out << "class Default" << type_name << "Repository final : public I" << type_name
            << "Repository\n";
        out << "{\n";
        out << "  public:\n";
        out << "    void register_descriptor(statespec::backend::IBackend& backend) override\n";
        out << "    {\n";
        out << "        backend.ensure_collection(" << entity_path
            << "_collection_descriptor());\n";
        out << "    }\n\n";
        out << "    std::optional<statespec::backend::VersionedRecord> createTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        statespec::backend::Json document\n";
        out << "    ) override\n";
        out << "    {\n";
        out << "        return entities_.create_entityTx(\n";
        out << "            tx,\n";
        out << "            ::statespec_generated::EntityCreateRequest{\n";
        out << "                ::statespec_generated::entity_lookup_from_document(" << entity_path
            << "_entity_descriptor(), document),\n";
        out << "                std::move(document),\n";
        out << "            }\n";
        out << "        );\n";
        out << "    }\n\n";
        out << "    std::optional<statespec::backend::VersionedRecord> getTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        std::vector<::statespec_generated::EntityKeyValue> key_values\n";
        out << "    ) override\n";
        out << "    {\n";
        out << "        return entities_.get_entityTx(\n";
        out << "            tx, ::statespec_generated::EntityGetRequest{" << entity_path
            << "_lookup(std::move(key_values))}\n";
        out << "        );\n";
        out << "    }\n\n";
        out << "    std::vector<statespec::backend::VersionedRecord> listByIndexTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        std::string index_name,\n";
        out << "        std::vector<statespec::backend::IndexValue> values\n";
        out << "    ) override\n";
        out << "    {\n";
        out << "        return entities_.list_entities_by_indexTx(\n";
        out << "            tx,\n";
        out << "            ::statespec_generated::EntityListByIndexRequest{\n";
        out << "                " << cpp_entity_name_constant_name(entity.name) << ",\n";
        out << "                std::move(index_name),\n";
        out << "                std::move(values),\n";
        out << "            }\n";
        out << "        );\n";
        out << "    }\n\n";
        for (const auto& index : entity.indexes)
        {
            out << "    std::vector<statespec::backend::VersionedRecord> "
                << entity_index_repository_method_name(index.name) << "(\n";
            out << "        statespec::backend::ITransaction& tx,\n";
            out << "        std::vector<statespec::backend::IndexValue> values\n";
            out << "    ) override\n";
            out << "    {\n";
            out << "        return listByIndexTx(tx, "
                << cpp_entity_index_constant_name(entity.name, index.name)
                << ", std::move(values));\n";
            out << "    }\n\n";
        }
        out << "    std::optional<statespec::backend::VersionedRecord> updateTx(\n";
        out << "        statespec::backend::ITransaction& tx,\n";
        out << "        std::vector<::statespec_generated::EntityKeyValue> key_values,\n";
        out << "        statespec::backend::Json document,\n";
        out << "        statespec::backend::Version expected_version\n";
        out << "    ) override\n";
        out << "    {\n";
        out << "        return entities_.upsert_entityTx(\n";
        out << "            tx,\n";
        out << "            ::statespec_generated::EntityUpsertRequest{\n";
        out << "                " << entity_path << "_lookup(std::move(key_values)),\n";
        out << "                std::move(document),\n";
        out << "                expected_version,\n";
        out << "            }\n";
        out << "        );\n";
        out << "    }\n\n";
        out << "  private:\n";
        out << "    ::statespec_generated::DefaultEntityRepository entities_;\n";
        out << "};\n\n";
    }
    else
    {
        out << "// Entity-centered " << area
            << " facade. Implementation moves here in the next staged split.\n\n";
    }
    out << "} // namespace statespec_generated::entities::" << entity_namespace << "\n";
    const auto content = out.str();
    if (area == "model")
    {
        return templates.render(
            "common/entities/model.hpp.tmpl", TemplateRenderer::Values{{"model_content", content}}
        );
    }
    if (area == "schema")
    {
        return templates.render(
            "common/entities/schema.hpp.tmpl", TemplateRenderer::Values{{"schema_content", content}}
        );
    }
    if (area == "persistence")
    {
        return templates.render(
            "common/entities/persistence.hpp.tmpl",
            TemplateRenderer::Values{{"persistence_content", content}}
        );
    }
    return content;
}

std::string cpp_entity_constants_header(
    const TemplatePackage& templates,
    const IrEntity& entity
)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "namespace statespec_generated::entities::" << snake_identifier(entity.name) << "\n";
    out << "{\n\n";
    out << "namespace constants\n";
    out << "{\n\n";
    out << "inline constexpr const char* " << cpp_entity_name_constant_name(entity.name) << " = "
        << cpp_string(entity.name) << ";\n";
    out << "inline constexpr const char* " << cpp_entity_plural_name_constant_name(entity.name)
        << " = " << cpp_string(cpp_entity_plural_api_field_name(entity.name)) << ";\n";
    out << "inline constexpr const char* k" << pascal_identifier(entity.name)
        << "CollectionName = " << cpp_string(entity.name) << ";\n";
    out << "inline constexpr const char* k" << pascal_identifier(entity.name)
        << "KeyHelperName = " << cpp_string(snake_identifier(entity.name) + "_lookup") << ";\n";
    for (const auto& field : entity.fields)
    {
        out << "inline constexpr const char* "
            << cpp_entity_field_constant_name(entity.name, field.name) << " = "
            << cpp_string(field.name) << ";\n";
        out << "inline constexpr const char* "
            << cpp_entity_field_type_name_constant_name(entity.name, field.name) << " = "
            << cpp_string(field.type) << ";\n";
    }
    for (const auto& index : entity.indexes)
    {
        out << "inline constexpr const char* "
            << cpp_entity_index_constant_name(entity.name, index.name) << " = "
            << cpp_string(index.name) << ";\n";
        out << "inline constexpr const char* k" << pascal_identifier(entity.name) << "Index"
            << pascal_identifier(index.name)
            << "HelperName = " << cpp_string(entity_index_repository_method_name(index.name))
            << ";\n";
    }
    for (const auto& state : entity.states)
    {
        out << "inline constexpr const char* "
            << cpp_entity_state_constant_name(entity.name, state.name) << " = "
            << cpp_string(state.name) << ";\n";
    }
    for (const auto& relation : entity.relations)
    {
        const auto relation_constant_prefix =
            "k" + pascal_identifier(entity.name) + "Relation" + pascal_identifier(relation.name);
        out << "inline constexpr const char* " << relation_constant_prefix
            << "Name = " << cpp_string(relation.name) << ";\n";
        out << "inline constexpr const char* " << relation_constant_prefix
            << "Target = " << cpp_string(relation.target) << ";\n";
        out << "inline constexpr const char* " << relation_constant_prefix
            << "Kind = " << cpp_string(relation.kind) << ";\n";
        if (relation.relation_kind.has_value())
        {
            out << "inline constexpr const char* " << relation_constant_prefix
                << "RelationKind = " << cpp_string(*relation.relation_kind) << ";\n";
        }
        if (relation.on_parent_delete.has_value())
        {
            out << "inline constexpr const char* " << relation_constant_prefix
                << "OnParentDelete = " << cpp_string(*relation.on_parent_delete) << ";\n";
        }
    }
    out << "\n} // namespace constants\n\n";
    out << "} // namespace statespec_generated::entities::" << snake_identifier(entity.name)
        << "\n";
    return templates.render(
        "common/entities/constants.hpp.tmpl",
        TemplateRenderer::Values{{"constants_content", out.str()}}
    );
}

std::string cpp_api_name_constant_name(const std::string& api_name)
{
    return "k" + pascal_identifier(api_name) + "ApiName";
}

std::string cpp_api_route_name_constant_name(
    const std::string& api_server_name,
    const std::string& api_name
)
{
    return "k" + pascal_identifier(api_server_name) + pascal_identifier(api_name) + "RouteName";
}

std::string cpp_api_response_envelope_constant_name(const std::string& entity_name)
{
    return "k" + pascal_identifier(entity_name) + "ListResponseEnvelopeName";
}

std::string cpp_api_server_name_constant_name(const std::string& api_server_name)
{
    return "k" + pascal_identifier(api_server_name) + "ServerName";
}

std::string cpp_api_server_concurrency_constant_name(const std::string& api_server_name)
{
    return "k" + pascal_identifier(api_server_name) + "ServerConcurrency";
}

std::string cpp_api_server_constants_header(const IrApiServer& api_server)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "namespace statespec_generated::api::servers::" << snake_identifier(api_server.name)
        << "::constants\n";
    out << "{\n\n";
    out << "inline constexpr const char* " << cpp_api_server_name_constant_name(api_server.name)
        << " = " << cpp_string(api_server.name) << ";\n";
    out << "inline constexpr int " << cpp_api_server_concurrency_constant_name(api_server.name)
        << " = " << api_server.concurrency.value_or(1) << ";\n\n";
    out << "} // namespace statespec_generated::api::servers::" << snake_identifier(api_server.name)
        << "::constants\n";
    return out.str();
}

std::string cpp_api_server_catalog_header(
    const TemplatePackage& templates,
    const IrSystem& system,
    const IrApiServer& api_server
)
{
    const auto entity_domains = crud_api_handler_domains(api_handler_domains(system));
    std::set<std::string> entity_api_names;
    std::ostringstream entity_includes;
    std::ostringstream entity_append_calls;
    std::ostringstream manual_served_api_entries;
    for (const auto& domain : entity_domains)
    {
        const auto served = std::any_of(
            domain.apis.begin(), domain.apis.end(),
            [&](const IrApi& api)
            {
                return std::find(api_server.serves.begin(), api_server.serves.end(), api.name) !=
                       api_server.serves.end();
            }
        );
        if (served)
        {
            entity_includes << "#include \"entities/" << snake_identifier(domain.name)
                            << ".hpp\"\n";
            entity_append_calls << "    entities::" << snake_identifier(domain.name)
                                << "::append_api_server_names(serves);\n";
        }
        for (const auto& api : domain.apis)
        {
            entity_api_names.insert(api.name);
        }
    }
    for (const auto& served_api : api_server.serves)
    {
        if (entity_api_names.contains(served_api))
        {
            continue;
        }
        manual_served_api_entries << "    serves.push_back(" << cpp_string(served_api) << ");\n";
    }
    return templates.render(
        "api/servers/catalog.hpp.tmpl",
        TemplateRenderer::Values{
            {"api_server_namespace", snake_identifier(api_server.name)},
            {"server_entity_includes", entity_includes.str()},
            {"server_entity_append_calls", entity_append_calls.str()},
            {"manual_served_api_entries", manual_served_api_entries.str()},
            {"api_server_name_constant", cpp_api_server_name_constant_name(api_server.name)},
            {"api_server_concurrency_constant",
             cpp_api_server_concurrency_constant_name(api_server.name)},
        }
    );
}

std::string cpp_api_server_entity_catalog_header(
    const TemplatePackage& templates,
    const IrApiServer& api_server,
    const ApiHandlerDomain& domain
)
{
    std::vector<IrApi> served_domain_apis;
    for (const auto& api : domain.apis)
    {
        if (std::find(api_server.serves.begin(), api_server.serves.end(), api.name) !=
            api_server.serves.end())
        {
            served_domain_apis.push_back(api);
        }
    }

    std::ostringstream condition;
    for (std::size_t i = 0; i < served_domain_apis.size(); ++i)
    {
        if (i > 0)
        {
            condition << " || ";
        }
        condition << "api_name == ::statespec_generated::api::entities::"
                  << snake_identifier(domain.name)
                  << "::constants::" << cpp_api_name_constant_name(served_domain_apis[i].name);
    }
    return templates.render(
        "api/servers/entity_catalog.hpp.tmpl",
        TemplateRenderer::Values{
            {"api_server_namespace", snake_identifier(api_server.name)},
            {"entity_namespace", snake_identifier(domain.name)},
            {"served_api_condition", condition.str()},
        }
    );
}

std::string cpp_entity_api_constants_header(
    const IrSystem& system,
    const IrEntity& entity
)
{
    const auto shapes = entity_api_shapes(system, entity.name);
    std::vector<IrApi> apis;
    for (const auto& domain : crud_api_handler_domains(api_handler_domains(system)))
    {
        if (domain.name == entity.name)
        {
            apis = domain.apis;
            break;
        }
    }

    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../../../common/entities/" << snake_identifier(entity.name)
        << "/constants.hpp\"\n\n";
    out << "namespace statespec_generated::api::entities::" << snake_identifier(entity.name)
        << "::constants\n";
    out << "{\n\n";
    for (const auto& api : apis)
    {
        out << "inline constexpr const char* " << cpp_api_name_constant_name(api.name) << " = "
            << cpp_string(api.name) << ";\n";
    }
    if (!apis.empty())
    {
        out << "\n";
    }
    for (const auto& api_server : system.api_servers)
    {
        for (const auto& api : apis)
        {
            if (std::find(api_server.serves.begin(), api_server.serves.end(), api.name) ==
                api_server.serves.end())
            {
                continue;
            }
            out << "inline constexpr const char* "
                << cpp_api_route_name_constant_name(api_server.name, api.name) << " = "
                << cpp_string(api_server.name + "." + api.name) << ";\n";
        }
    }
    if (!system.api_servers.empty() && !apis.empty())
    {
        out << "\n";
    }
    for (const auto& shape : shapes)
    {
        out << "inline constexpr const char* " << cpp_shape_name_constant_name(shape.name) << " = "
            << cpp_string(shape.name) << ";\n";
    }
    if (!shapes.empty())
    {
        out << "\n";
    }
    out << "inline constexpr const char* " << cpp_api_response_envelope_constant_name(entity.name)
        << " = ::statespec_generated::entities::" << snake_identifier(entity.name)
        << "::constants::" << cpp_entity_plural_name_constant_name(entity.name) << ";\n\n";
    out << "} // namespace statespec_generated::api::entities::" << snake_identifier(entity.name)
        << "::constants\n";
    return out.str();
}

std::string cpp_event_descriptor_module(const IrSystem& system)
{
    std::ostringstream out;
    out << generate_cpp_event_descriptors(system);
    out << "struct EventEnvelope\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::map<std::string, statespec::backend::Json> fields;\n";
    out << "};\n\n";
    for (const auto& event : system.events)
    {
        out << "inline EventEnvelope make_" << snake_identifier(event.name) << "_event(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "    statespec::backend::Json " << field.name;
            out << (i + 1 < event.fields.size() ? ",\n" : "\n");
        }
        out << ")\n";
        out << "{\n";
        out << "    return EventEnvelope{\n";
        out << "        " << cpp_string(event.name) << ",\n";
        out << "        {\n";
        for (const auto& field : event.fields)
        {
            out << "            {" << cpp_string(field.name) << ", std::move(" << field.name
                << ")},\n";
        }
        out << "        },\n";
        out << "    };\n";
        out << "}\n\n";
    }
    return out.str();
}

TemplateRenderer::Values cpp_external_system_descriptor_module_values(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    return TemplateRenderer::Values{
        {"descriptor_module_name", "external system descriptors"},
        {"descriptor_module_content",
         generate_cpp_external_system_descriptors(
             system, templates.load("generated/external_system_call_adapters.hpp.tmpl")
         )},
    };
}

TemplateRenderer::Values cpp_shape_descriptor_module_values(const IrSystem& system)
{
    std::ostringstream includes;
    std::ostringstream aggregation;
    for (const auto& shape : system.shapes)
    {
        const auto prefix = snake_identifier(shape.name);
        includes << "#include \"shapes/" << prefix << ".hpp\"\n";
        aggregation << "    for (auto descriptor : " << prefix << "_shape_descriptors())\n";
        aggregation << "    {\n";
        aggregation << "        descriptors.push_back(std::move(descriptor));\n";
        aggregation << "    }\n";
    }
    std::ostringstream content;
    content << "#include \"../shape_types.hpp\"\n";
    content << includes.str();
    if (!includes.str().empty())
    {
        content << "\n";
    }
    content << "inline std::vector<ShapeDescriptor> shape_descriptors()\n";
    content << "{\n";
    content << "    std::vector<ShapeDescriptor> descriptors;\n";
    content << aggregation.str();
    content << "    return descriptors;\n";
    content << "}\n\n";
    return TemplateRenderer::Values{
        {"descriptor_module_name", "shape descriptors"},
        {"descriptor_module_content", content.str()},
    };
}

TemplateRenderer::Values cpp_shape_descriptor_module_values(const IrShape& shape)
{
    IrSystem one_shape_system;
    one_shape_system.shapes.push_back(shape);
    auto content = generate_cpp_shape_descriptors(one_shape_system);
    const auto prefix = snake_identifier(shape.name);
    content = replace_all_copy(content, "shape_descriptors()", prefix + "_shape_descriptors()");
    content = "#include \"../../shape_types.hpp\"\n\n" + content;
    return TemplateRenderer::Values{
        {"descriptor_module_name", "shape descriptor " + shape.name},
        {"descriptor_module_content", content},
    };
}

void add_cpp_raw_common_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    std::string_view relative_output_path,
    std::string content
)
{
    const auto relative_path = common_artifact_path(relative_output_path);
    result.files.push_back(
        GeneratedFile{
            (options.output_dir / relative_path).string(),
            std::move(content),
            GeneratedArtifactTier::Common,
            relative_path.generic_string(),
        }
    );
}

void add_cpp_raw_api_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    std::string_view relative_output_path,
    std::string content
);

std::string cpp_shape_type_declaration(const IrShape& shape);

std::string cpp_shape_type_header(
    const IrShape& shape,
    std::string_view backend_include
)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"" << backend_include << "\"\n\n";
    out << "#include <cstdint>\n";
    out << "#include <optional>\n";
    out << "#include <string>\n\n";
    out << "namespace statespec_generated\n";
    out << "{\n\n";
    out << cpp_shape_type_declaration(shape);
    out << "} // namespace statespec_generated\n";
    return out.str();
}

std::string cpp_api_shape_descriptor_declarations(
    const IrSystem& system,
    std::string_view function_name
)
{
    auto descriptors = generate_cpp_shape_descriptors(system);
    descriptors = replace_all_copy(descriptors, "shape_descriptors()", std::string{function_name});
    return descriptors;
}

const IrField* cpp_find_entity_field(
    const IrEntity& entity,
    const std::string& field_name
)
{
    const auto it = std::find_if(
        entity.fields.begin(), entity.fields.end(),
        [&](const IrField& field) { return field.name == field_name; }
    );
    return it == entity.fields.end() ? nullptr : &*it;
}

std::string cpp_entity_shape_field_descriptor_expr(
    const IrEntity& entity,
    const IrShape& shape,
    const IrField& field
)
{
    auto expression = cpp_shape_field_descriptor_expr(shape.name, field);
    if (cpp_find_entity_field(entity, field.name) == nullptr)
    {
        if (field.name == cpp_entity_plural_api_field_name(entity.name))
        {
            return "statespec::backend::FieldDescriptor{entities::" +
                   snake_identifier(entity.name) +
                   "::constants::" + cpp_entity_plural_name_constant_name(entity.name) + ", " +
                   cpp_field_type_enum_expr(field.type) + ", " +
                   cpp_shape_field_type_name_constant_name(shape.name, field.name) + ", " +
                   (cpp_field_required(field.type) ? "true" : "false") + "}";
        }
        return expression;
    }
    expression = replace_all_copy(
        expression, cpp_shape_field_constant_name(shape.name, field.name),
        "entities::" + snake_identifier(entity.name) +
            "::constants::" + cpp_entity_field_constant_name(entity.name, field.name)
    );
    expression = replace_all_copy(
        expression, cpp_shape_field_type_name_constant_name(shape.name, field.name),
        "entities::" + snake_identifier(entity.name) +
            "::constants::" + cpp_entity_field_type_name_constant_name(entity.name, field.name)
    );
    return expression;
}

std::string cpp_entity_api_shape_descriptor_declarations(
    const IrEntity& entity,
    const IrShape& shape,
    std::string_view function_name
)
{
    std::ostringstream out;
    out << "inline constexpr const char* " << cpp_shape_name_constant_name(shape.name) << " = "
        << cpp_string(shape.name) << ";\n";
    for (const auto& field : shape.fields)
    {
        if (cpp_find_entity_field(entity, field.name) != nullptr)
        {
            continue;
        }
        if (field.name == cpp_entity_plural_api_field_name(entity.name))
        {
            out << "inline constexpr const char* "
                << cpp_shape_field_type_name_constant_name(shape.name, field.name) << " = "
                << cpp_string(field.type) << ";\n";
            continue;
        }
        out << "inline constexpr const char* "
            << cpp_shape_field_constant_name(shape.name, field.name) << " = "
            << cpp_string(field.name) << ";\n";
        out << "inline constexpr const char* "
            << cpp_shape_field_type_name_constant_name(shape.name, field.name) << " = "
            << cpp_string(field.type) << ";\n";
    }
    out << "\n";
    out << "inline std::vector<ShapeDescriptor> " << function_name << "\n";
    out << "{\n";
    out << "    return {\n";
    out << "        ShapeDescriptor{\n";
    out << "            " << cpp_shape_name_constant_name(shape.name) << ",\n";
    out << "            {\n";
    for (const auto& field : shape.fields)
    {
        out << "                " << cpp_entity_shape_field_descriptor_expr(entity, shape, field)
            << ",\n";
    }
    out << "            },\n";
    out << "        },\n";
    out << "    };\n";
    out << "}\n\n";
    return out.str();
}

std::string cpp_api_shape_type_header(
    const IrShape& shape,
    std::string_view backend_include,
    std::string_view shape_types_include
)
{
    IrSystem one_shape_system;
    one_shape_system.shapes.push_back(shape);

    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"" << backend_include << "\"\n";
    out << "#include \"" << shape_types_include << "\"\n\n";
    out << "#include <cstdint>\n";
    out << "#include <optional>\n";
    out << "#include <string>\n\n";
    out << "namespace statespec_generated\n";
    out << "{\n\n";
    out << cpp_shape_type_declaration(shape);
    out << cpp_api_shape_descriptor_declarations(
        one_shape_system, snake_identifier(shape.name) + "_shape_descriptors()"
    );
    out << "} // namespace statespec_generated\n";
    return out.str();
}

std::string cpp_shape_type_declaration(const IrShape& shape)
{
    std::ostringstream out;
    out << "struct " << pascal_identifier(shape.name) << "\n";
    out << "{\n";
    for (const auto& field : shape.fields)
    {
        out << "    " << cpp_shape_type(field.type) << " " << field.name << "{};\n";
    }
    out << "};\n\n";
    return out.str();
}

std::string cpp_shapes_umbrella_header(
    const IrSystem& system,
    bool api_shapes
)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    for (const auto& shape : system.shapes)
    {
        if (api_shapes)
        {
            const auto owner = entity_api_shape_owner(system, shape.name);
            if (owner.has_value())
            {
                continue;
            }
        }
        out << "#include \"shapes/" << snake_identifier(shape.name) << ".hpp\"\n";
    }
    if (api_shapes)
    {
        out << "#include \"../common/shape_types.hpp\"\n";
        out << "#include <utility>\n";
        for (const auto& entity : system.entities)
        {
            if (!entity_api_shapes(system, entity.name).empty())
            {
                continue;
            }
        }
        out << "\n";
        out << "namespace statespec_generated\n";
        out << "{\n\n";
        std::set<std::string> declared_entities;
        for (const auto& shape : system.shapes)
        {
            const auto owner = entity_api_shape_owner(system, shape.name);
            if (!owner.has_value() || !declared_entities.insert(*owner).second)
            {
                continue;
            }
            out << "namespace api::entities::" << snake_identifier(*owner) << "\n";
            out << "{\n";
            out << "std::vector<::statespec_generated::ShapeDescriptor> shape_descriptors();\n";
            out << "} // namespace api::entities::" << snake_identifier(*owner) << "\n\n";
        }
        out << "inline std::vector<ShapeDescriptor> api_shape_descriptors()\n";
        out << "{\n";
        out << "    std::vector<ShapeDescriptor> descriptors;\n";
        for (const auto& shape : system.shapes)
        {
            if (entity_api_shape_owner(system, shape.name).has_value())
            {
                continue;
            }
            out << "    for (auto descriptor : " << snake_identifier(shape.name)
                << "_shape_descriptors())\n";
            out << "    {\n";
            out << "        descriptors.push_back(std::move(descriptor));\n";
            out << "    }\n";
        }
        std::set<std::string> added_entities;
        for (const auto& shape : system.shapes)
        {
            const auto owner = entity_api_shape_owner(system, shape.name);
            if (!owner.has_value() || !added_entities.insert(*owner).second)
            {
                continue;
            }
            out << "    {\n";
            out << "        auto slice = ::statespec_generated::api::entities::"
                << snake_identifier(*owner) << "::shape_descriptors();\n";
            out << "        descriptors.insert(descriptors.end(), slice.begin(), slice.end());\n";
            out << "    }\n";
        }
        out << "    return descriptors;\n";
        out << "}\n\n";
        out << "} // namespace statespec_generated\n";
    }
    return out.str();
}

std::string cpp_entity_api_shapes_header(
    const TemplatePackage& templates,
    const IrEntity& entity,
    const std::vector<IrShape>& shapes
)
{
    std::ostringstream shape_type_declarations;
    std::ostringstream shape_descriptor_declarations;
    for (const auto& shape : shapes)
    {
        shape_type_declarations << cpp_shape_type_declaration(shape);
        shape_descriptor_declarations << cpp_entity_api_shape_descriptor_declarations(
            entity, shape, snake_identifier(shape.name) + "_shape_descriptors()"
        );
    }
    return templates.render(
        "api/entities/shapes.hpp.tmpl",
        TemplateRenderer::Values{
            {"entity_snake", snake_identifier(entity.name)},
            {"shape_type_declarations", shape_type_declarations.str()},
            {"shape_descriptor_declarations", shape_descriptor_declarations.str()},
        }
    );
}

std::string cpp_entity_api_catalog_header(
    const IrSystem& system,
    const IrEntity& entity
)
{
    const auto shapes = entity_api_shapes(system, entity.name);
    std::vector<IrApi> apis;
    for (const auto& domain : crud_api_handler_domains(api_handler_domains(system)))
    {
        if (domain.name == entity.name)
        {
            apis = domain.apis;
            break;
        }
    }

    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../../../common/descriptors/types.hpp\"\n";
    out << "#include \"../../../common/shape_types.hpp\"\n";
    out << "#include \"constants.hpp\"\n";
    out << "#include \"shapes.hpp\"\n";
    out << "#include \"codecs.hpp\"\n";
    out << "#include \"handlers.hpp\"\n";
    out << "#include \"registry.hpp\"\n";
    for (const auto& api : apis)
    {
        out << "#include \"descriptors/" << snake_identifier(api.name) << ".hpp\"\n";
    }
    out << "\n#include <string>\n";
    out << "#include <utility>\n";
    out << "#include <vector>\n\n";
    out << "namespace statespec_generated::api::entities::" << snake_identifier(entity.name)
        << "\n";
    out << "{\n\n";
    out << "using HandlerRegistry = ::statespec_generated::api::"
        << cpp_api_handler_domain_class_name(entity.name) << ";\n\n";
    out << "inline std::vector<::statespec_generated::ShapeDescriptor> shape_descriptors()\n";
    out << "{\n";
    out << "    std::vector<::statespec_generated::ShapeDescriptor> descriptors;\n";
    for (const auto& shape : shapes)
    {
        out << "    for (auto descriptor : ::statespec_generated::" << snake_identifier(shape.name)
            << "_shape_descriptors())\n";
        out << "    {\n";
        out << "        descriptors.push_back(std::move(descriptor));\n";
        out << "    }\n";
    }
    out << "    return descriptors;\n";
    out << "}\n\n";
    out << "inline std::vector<::statespec_generated::ApiDescriptor> api_descriptors()\n";
    out << "{\n";
    out << "    std::vector<::statespec_generated::ApiDescriptor> descriptors;\n";
    for (const auto& api : apis)
    {
        out << "    {\n";
        out << "        auto slice = descriptors::" << cpp_api_descriptor_function_name(api)
            << "();\n";
        out << "        descriptors.insert(descriptors.end(), slice.begin(), slice.end());\n";
        out << "    }\n";
    }
    out << "    return descriptors;\n";
    out << "}\n\n";
    out << "inline std::vector<std::string> api_names()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& api : apis)
    {
        out << "        constants::" << cpp_api_name_constant_name(api.name) << ",\n";
    }
    out << "    };\n";
    out << "}\n\n";
    out << "inline std::vector<::statespec_generated::ApiRouteDescriptor> "
           "api_route_descriptors()\n";
    out << "{\n";
    out << "    std::vector<::statespec_generated::ApiRouteDescriptor> descriptors;\n";
    for (const auto& api : apis)
    {
        out << "    {\n";
        out << "        auto slice = descriptors::" << cpp_api_route_descriptor_function_name(api)
            << "();\n";
        out << "        descriptors.insert(descriptors.end(), slice.begin(), slice.end());\n";
        out << "    }\n";
    }
    out << "    return descriptors;\n";
    out << "}\n\n";
    out << "inline std::vector<std::string> handler_entrypoints()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& api : apis)
    {
        out << "        \"handle_" << snake_identifier(api.name) << "\",\n";
    }
    out << "    };\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::api::entities::" << snake_identifier(entity.name)
        << "\n";
    return out.str();
}

void add_cpp_shape_type_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    const auto shared_shapes = common_shape_system(system);
    if (!shared_shapes.shapes.empty())
    {
        add_cpp_raw_common_file(
            result, options, "shapes.hpp", cpp_shapes_umbrella_header(shared_shapes, false)
        );
    }
    for (const auto& shape : shared_shapes.shapes)
    {
        add_cpp_raw_common_file(
            result, options, "shapes/" + snake_identifier(shape.name) + ".hpp",
            cpp_shape_type_header(shape, "../backend.hpp")
        );
    }
}

void add_cpp_api_shape_type_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system
)
{
    const auto api_shapes = api_contract_shape_system(system);
    add_cpp_raw_api_file(
        result, options, "api/shapes.hpp", cpp_shapes_umbrella_header(api_shapes, true)
    );
    for (const auto& shape : api_shapes.shapes)
    {
        if (entity_api_shape_owner(system, shape.name).has_value())
        {
            continue;
        }
        add_cpp_raw_api_file(
            result, options, "api/shapes/" + snake_identifier(shape.name) + ".hpp",
            cpp_api_shape_type_header(
                shape, "../../common/backend.hpp", "../../common/shape_types.hpp"
            )
        );
    }
    for (const auto& entity : system.entities)
    {
        const auto shapes = entity_api_shapes(system, entity.name);
        if (shapes.empty())
        {
            continue;
        }
        add_cpp_raw_api_file(
            result, options, "api/entities/" + snake_identifier(entity.name) + "/constants.hpp",
            cpp_entity_api_constants_header(system, entity)
        );
        add_cpp_raw_api_file(
            result, options, "api/entities/" + snake_identifier(entity.name) + "/shapes.hpp",
            cpp_entity_api_shapes_header(templates, entity, shapes)
        );
    }
}

void add_cpp_descriptor_module_artifact(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    std::string_view relative_output_path,
    std::string_view module_name,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {}
)
{
    const auto resolved_values =
        values.empty() ? cpp_descriptor_module_values(module_name) : values;
    add_generated_template_file(
        result, options.output_dir, templates,
        generated_template_path("descriptor_module.hpp.tmpl"),
        common_artifact_path(relative_output_path), diagnostics, GeneratedArtifactTier::Common,
        resolved_values
    );
}

void add_cpp_descriptor_module_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto common_shapes = common_shape_system(system);
    add_cpp_descriptor_module_artifact(
        result, options, templates, "descriptors/core.hpp", "core descriptors", diagnostics
    );
    add_cpp_descriptor_module_artifact(
        result, options, templates, "descriptors/values_enums.hpp", "value and enum descriptors",
        diagnostics,
        TemplateRenderer::Values{
            {"descriptor_module_name", "value and enum descriptors"},
            {"descriptor_module_content", generate_cpp_value_enum_descriptors(system)}
        }
    );
    add_cpp_descriptor_module_artifact(
        result, options, templates, "descriptors/events.hpp", "event helpers", diagnostics,
        TemplateRenderer::Values{
            {"descriptor_module_name", "event helpers"},
            {"descriptor_module_content", cpp_event_descriptor_module(system)}
        }
    );
    add_cpp_descriptor_module_artifact(
        result, options, templates, "descriptors/external_systems.hpp",
        "external system descriptors", diagnostics,
        cpp_external_system_descriptor_module_values(system, templates)
    );
    add_cpp_descriptor_module_artifact(
        result, options, templates, "descriptors/policies.hpp", "policy descriptors", diagnostics,
        TemplateRenderer::Values{
            {"descriptor_module_name", "policy descriptors"},
            {"descriptor_module_content", generate_cpp_policy_descriptors(system)}
        }
    );
    if (!common_shapes.shapes.empty())
    {
        add_cpp_descriptor_module_artifact(
            result, options, templates, "descriptors/shapes.hpp", "shape descriptors", diagnostics,
            cpp_shape_descriptor_module_values(common_shapes)
        );
        for (const auto& shape : common_shapes.shapes)
        {
            add_generated_template_file(
                result, options.output_dir, templates,
                generated_template_path("descriptor_module.hpp.tmpl"),
                common_artifact_path("descriptors/shapes/" + snake_identifier(shape.name) + ".hpp"),
                diagnostics, GeneratedArtifactTier::Common,
                cpp_shape_descriptor_module_values(shape)
            );
        }
    }
    add_cpp_descriptor_module_artifact(
        result, options, templates, "descriptors/runtime.hpp", "runtime descriptors", diagnostics,
        TemplateRenderer::Values{
            {"descriptor_module_name", "runtime descriptors"},
            {"descriptor_module_content", generate_cpp_feature_flag_descriptors(system) +
                                              generate_cpp_observability_descriptors(system) +
                                              generate_cpp_runtime_descriptors(system)}
        }
    );
    for (const auto& [name, label] : cpp_runtime_registration_modules(system))
    {
        add_cpp_descriptor_module_artifact(
            result, options, templates, "descriptors/runtime/" + name + ".hpp", label, diagnostics,
            TemplateRenderer::Values{
                {"descriptor_module_name", label},
                {"descriptor_module_content",
                 generate_cpp_runtime_registration_domain(templates, name)}
            }
        );
    }
    for (const auto& entity : system.entities)
    {
        const auto entity_dir = "entities/" + snake_identifier(entity.name) + "/";
        add_cpp_raw_common_file(
            result, options, entity_dir + "constants.hpp",
            cpp_entity_constants_header(templates, entity)
        );
        add_cpp_raw_common_file(
            result, options, entity_dir + "model.hpp",
            cpp_entity_centered_facade_header(templates, entity, "model")
        );
        add_cpp_raw_common_file(
            result, options, entity_dir + "persistence.hpp",
            cpp_entity_centered_facade_header(templates, entity, "persistence")
        );
        add_cpp_raw_common_file(
            result, options, entity_dir + "schema.hpp",
            cpp_entity_centered_facade_header(templates, entity, "schema")
        );
        if (cpp_entity_uses_gc(entity))
        {
            add_cpp_raw_common_file(
                result, options, entity_dir + "gc.hpp", cpp_entity_gc_descriptor_header(entity)
            );
        }
    }
}

void add_cpp_workflow_descriptor_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    for (const auto& workflow : system.workflows)
    {
        add_cpp_raw_common_file(
            result, options, "workflows/" + snake_identifier(workflow.name) + ".hpp",
            generate_cpp_workflow_descriptor(workflow)
        );
    }
}

void add_cpp_generated_template_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    std::string_view relative_output_path,
    GeneratedArtifactTier tier,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {}
)
{
    add_generated_template_file(
        result, options.output_dir, templates, template_path_for_output(relative_output_path),
        artifact_path(relative_output_path), diagnostics, tier, values
    );
}

void add_cpp_raw_api_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    std::string_view relative_output_path,
    std::string content
)
{
    const auto relative_path = artifact_path(relative_output_path);
    result.files.push_back(
        GeneratedFile{
            (options.output_dir / relative_path).string(),
            std::move(content),
            GeneratedArtifactTier::Api,
            relative_path.generic_string(),
        }
    );
}

void add_cpp_raw_worker_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    std::string_view relative_output_path,
    std::string content
)
{
    const auto relative_path = artifact_path(relative_output_path);
    result.files.push_back(
        GeneratedFile{
            (options.output_dir / relative_path).string(),
            std::move(content),
            GeneratedArtifactTier::Worker,
            relative_path.generic_string(),
        }
    );
}

std::string cpp_api_descriptor_function_name(const IrApi& api)
{
    return snake_identifier(api.name) + "_api_descriptors";
}

std::string cpp_api_route_descriptor_function_name(const IrApi& api)
{
    return snake_identifier(api.name) + "_api_route_descriptors";
}

std::string cpp_api_shape_include_for(
    const IrSystem& system,
    std::string_view shape_name,
    bool entity_local
)
{
    const auto owner = entity_api_shape_owner(system, shape_name);
    if (owner.has_value())
    {
        return entity_local ? "../shapes.hpp"
                            : "../entities/" + snake_identifier(*owner) + "/shapes.hpp";
    }
    return "../shapes/" + snake_identifier(std::string{shape_name}) + ".hpp";
}

std::string cpp_api_optional_shape_name_expr(
    const IrSystem& system,
    const std::optional<std::string>& value
)
{
    if (!value.has_value())
    {
        return "std::nullopt";
    }
    if (find_shape(system, *value) == nullptr)
    {
        return optional_string_expr(value);
    }
    return "std::optional<std::string>{" + cpp_shape_name_constant_name(*value) + "}";
}

const IrEntity* cpp_api_entity(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api.entity.has_value())
    {
        return nullptr;
    }
    const auto it = std::find_if(
        system.entities.begin(), system.entities.end(),
        [&](const IrEntity& entity) { return entity.name == *api.entity; }
    );
    return it == system.entities.end() ? nullptr : &*it;
}

bool cpp_entity_has_field(
    const IrEntity& entity,
    std::string_view field_name
)
{
    return std::any_of(
        entity.fields.begin(), entity.fields.end(),
        [&](const IrField& field) { return field.name == field_name; }
    );
}

bool cpp_api_path_uses_entity_constants(
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = cpp_api_entity(system, api);
    if (entity == nullptr || !api.path.has_value())
    {
        return false;
    }
    for (std::size_t pos = 0; (pos = api.path->find('{', pos)) != std::string::npos;)
    {
        const auto end = api.path->find('}', pos + 1);
        if (end == std::string::npos)
        {
            return false;
        }
        if (cpp_entity_has_field(
                *entity, std::string_view{*api.path}.substr(pos + 1, end - pos - 1)
            ))
        {
            return true;
        }
        pos = end + 1;
    }
    return false;
}

std::string cpp_api_optional_path_expr(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api.path.has_value())
    {
        return "std::nullopt";
    }
    const auto* entity = cpp_api_entity(system, api);
    if (entity == nullptr || !cpp_api_path_uses_entity_constants(system, api))
    {
        return optional_string_expr(api.path);
    }

    std::vector<std::string> parts;
    std::size_t cursor = 0;
    for (std::size_t pos = 0; (pos = api.path->find('{', cursor)) != std::string::npos;)
    {
        const auto end = api.path->find('}', pos + 1);
        if (end == std::string::npos)
        {
            return optional_string_expr(api.path);
        }
        const auto field_name = api.path->substr(pos + 1, end - pos - 1);
        parts.push_back(cpp_string(api.path->substr(cursor, pos - cursor + 1)));
        if (cpp_entity_has_field(*entity, field_name))
        {
            parts.push_back(
                "::statespec_generated::entities::" + snake_identifier(entity->name) +
                "::constants::" + cpp_entity_field_constant_name(entity->name, field_name)
            );
        }
        else
        {
            parts.push_back(cpp_string(field_name));
        }
        parts.push_back(cpp_string("}"));
        cursor = end + 1;
    }
    if (cursor < api.path->size())
    {
        parts.push_back(cpp_string(api.path->substr(cursor)));
    }
    if (parts.empty())
    {
        return optional_string_expr(api.path);
    }

    std::ostringstream expr;
    expr << "std::optional<std::string>{std::string{" << parts.front() << "}";
    for (std::size_t i = 1; i < parts.size(); ++i)
    {
        expr << " + " << parts[i];
    }
    expr << "}";
    return expr.str();
}

std::string cpp_api_descriptor_module(
    const IrSystem& system,
    const IrApi& api,
    bool entity_local
)
{
    std::ostringstream out;
    const auto common_prefix = entity_local ? "../../../../common" : "../../common";
    out << "#pragma once\n\n";
    out << "#include \"" << common_prefix << "/descriptors/types.hpp\"\n\n";
    if (const auto* entity = cpp_api_entity(system, api);
        entity != nullptr && cpp_api_path_uses_entity_constants(system, api))
    {
        out << "#include \"" << common_prefix << "/entities/" << snake_identifier(entity->name)
            << "/constants.hpp\"\n";
    }
    std::set<std::string> shape_includes;
    if (api.input.has_value() && find_shape(system, *api.input) != nullptr)
    {
        shape_includes.insert(cpp_api_shape_include_for(system, *api.input, entity_local));
    }
    if (api.output.has_value() && find_shape(system, *api.output) != nullptr)
    {
        shape_includes.insert(cpp_api_shape_include_for(system, *api.output, entity_local));
    }
    for (const auto& include : shape_includes)
    {
        out << "#include \"" << include << "\"\n";
    }
    if (!shape_includes.empty())
    {
        out << "\n";
    }
    if (entity_local)
    {
        const auto* entity = cpp_api_entity(system, api);
        out << "namespace statespec_generated::api::entities::" << snake_identifier(entity->name)
            << "::descriptors\n";
    }
    else
    {
        out << "namespace statespec_generated::api::descriptors\n";
    }
    out << "{\n\n";
    out << "inline std::vector<::statespec_generated::ApiDescriptor> "
        << cpp_api_descriptor_function_name(api) << "()\n";
    out << "{\n";
    out << "    return {\n";
    out << "        ::statespec_generated::ApiDescriptor{\n";
    out << "            " << cpp_string(api.name) << ",\n";
    out << "            " << optional_string_expr(api.method) << ",\n";
    out << "            " << cpp_api_optional_path_expr(system, api) << ",\n";
    out << "            " << cpp_api_optional_shape_name_expr(system, api.input) << ",\n";
    out << "            " << cpp_api_optional_shape_name_expr(system, api.output) << ",\n";
    out << "            " << optional_string_expr(api.error) << ",\n";
    out << "            " << optional_string_expr(api.starts_workflow) << ",\n";
    out << "            " << optional_string_expr(api.enqueues) << ",\n";
    out << "        },\n";
    out << "    };\n";
    out << "}\n\n";
    out << "inline std::vector<::statespec_generated::ApiRouteDescriptor> "
        << cpp_api_route_descriptor_function_name(api) << "()\n";
    out << "{\n";
    out << "    return {\n";
    for (const auto& api_server : system.api_servers)
    {
        if (std::find(api_server.serves.begin(), api_server.serves.end(), api.name) ==
            api_server.serves.end())
        {
            continue;
        }
        out << "        ::statespec_generated::ApiRouteDescriptor{\n";
        out << "            " << cpp_string(api_server.name + "." + api.name) << ",\n";
        out << "            " << cpp_string(api_server.name) << ",\n";
        out << "            " << cpp_string(api.name) << ",\n";
        out << "            " << optional_string_expr(api.method) << ",\n";
        out << "            " << cpp_api_optional_path_expr(system, api) << ",\n";
        out << "            " << cpp_api_optional_shape_name_expr(system, api.input) << ",\n";
        out << "            " << cpp_api_optional_shape_name_expr(system, api.output) << ",\n";
        out << "            " << optional_string_expr(api.error) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";
    if (entity_local)
    {
        const auto* entity = cpp_api_entity(system, api);
        out << "} // namespace statespec_generated::api::entities::"
            << snake_identifier(entity->name) << "::descriptors\n";
    }
    else
    {
        out << "} // namespace statespec_generated::api::descriptors\n";
    }
    return out.str();
}

void add_cpp_api_descriptor_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    for (const auto& api : system.apis)
    {
        if (const auto* entity = cpp_api_entity(system, api); entity != nullptr)
        {
            add_cpp_raw_api_file(
                result, options,
                "api/entities/" + snake_identifier(entity->name) + "/descriptors/" +
                    snake_identifier(api.name) + ".hpp",
                cpp_api_descriptor_module(system, api, true)
            );
            continue;
        }
        add_cpp_raw_api_file(
            result, options, "api/descriptors/" + snake_identifier(api.name) + ".hpp",
            cpp_api_descriptor_module(system, api, false)
        );
    }
}

std::string cpp_api_descriptor_catalog_file(const IrSystem& system)
{
    std::set<std::string> entity_api_names;
    const auto entity_domains = crud_api_handler_domains(api_handler_domains(system));
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../../common/descriptors/types.hpp\"\n";
    out << "#include \"../../common/shape_types.hpp\"\n";
    out << "#include \"../shapes.hpp\"\n";
    for (const auto& domain : entity_domains)
    {
        out << "#include \"../entities/" << snake_identifier(domain.name) << "/catalog.hpp\"\n";
        for (const auto& api : domain.apis)
        {
            entity_api_names.insert(api.name);
        }
    }
    for (const auto& api : system.apis)
    {
        if (entity_api_names.contains(api.name))
        {
            continue;
        }
        out << "#include \"" << snake_identifier(api.name) << ".hpp\"\n";
    }
    out << "\n#include <vector>\n\n";
    out << "namespace statespec_generated::api::descriptors\n";
    out << "{\n\n";
    out << "using ApiDescriptor = ::statespec_generated::ApiDescriptor;\n";
    out << "using ApiRouteDescriptor = ::statespec_generated::ApiRouteDescriptor;\n";
    out << "using ApiServerDescriptor = ::statespec_generated::ApiServerDescriptor;\n\n";
    out << "inline std::vector<::statespec_generated::ShapeDescriptor> api_shape_descriptors()\n";
    out << "{\n";
    out << "    return ::statespec_generated::api_shape_descriptors();\n";
    out << "}\n\n";
    out << "inline std::vector<ApiDescriptor> api_descriptors()\n";
    out << "{\n";
    out << "    std::vector<ApiDescriptor> descriptors;\n";
    for (const auto& domain : entity_domains)
    {
        out << "    {\n";
        out << "        auto slice = ::statespec_generated::api::entities::"
            << snake_identifier(domain.name) << "::api_descriptors();\n";
        out << "        descriptors.insert(descriptors.end(), slice.begin(), slice.end());\n";
        out << "    }\n";
    }
    for (const auto& api : system.apis)
    {
        if (entity_api_names.contains(api.name))
        {
            continue;
        }
        out << "    {\n";
        out << "        auto slice = " << cpp_api_descriptor_function_name(api) << "();\n";
        out << "        descriptors.insert(descriptors.end(), slice.begin(), slice.end());\n";
        out << "    }\n";
    }
    out << "    return descriptors;\n";
    out << "}\n\n";
    out << "inline std::vector<ApiRouteDescriptor> api_route_descriptors()\n";
    out << "{\n";
    out << "    std::vector<ApiRouteDescriptor> descriptors;\n";
    for (const auto& domain : entity_domains)
    {
        out << "    {\n";
        out << "        auto slice = ::statespec_generated::api::entities::"
            << snake_identifier(domain.name) << "::api_route_descriptors();\n";
        out << "        descriptors.insert(descriptors.end(), slice.begin(), slice.end());\n";
        out << "    }\n";
    }
    for (const auto& api : system.apis)
    {
        if (entity_api_names.contains(api.name))
        {
            continue;
        }
        out << "    {\n";
        out << "        auto slice = " << cpp_api_route_descriptor_function_name(api) << "();\n";
        out << "        descriptors.insert(descriptors.end(), slice.begin(), slice.end());\n";
        out << "    }\n";
    }
    out << "    return descriptors;\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::api::descriptors\n";
    return out.str();
}

std::string cpp_worker_descriptor_catalog_file(const IrSystem& system)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../../common/descriptors/types.hpp\"\n";
    for (const auto& worker : system.workers)
    {
        out << "#include \"" << snake_identifier(worker.name) << ".hpp\"\n";
    }
    out << "\nnamespace statespec_generated::worker::descriptors\n";
    out << "{\n\n";
    out << "using WorkerContext = ::statespec_generated::WorkerContext;\n";
    out << "using WorkerDescriptor = ::statespec_generated::WorkerDescriptor;\n\n";
    out << "inline std::vector<WorkerDescriptor> worker_descriptors()\n";
    out << "{\n";
    out << "    std::vector<WorkerDescriptor> descriptors;\n";
    for (const auto& worker : system.workers)
    {
        out << "    descriptors.push_back(" << snake_identifier(worker.name)
            << "_worker_descriptor());\n";
    }
    out << "    return descriptors;\n";
    out << "}\n\n";
    out << "inline std::vector<WorkerContext> worker_contexts()\n";
    out << "{\n";
    out << "    std::vector<WorkerContext> contexts;\n";
    for (const auto& worker : system.workers)
    {
        out << "    contexts.push_back(" << snake_identifier(worker.name)
            << "_worker_context());\n";
    }
    out << "    return contexts;\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::worker::descriptors\n";
    return out.str();
}

std::string cpp_api_descriptors_header(const IrSystem& system)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../common/descriptors/types.hpp\"\n";
    out << "#include \"descriptors/catalog.hpp\"\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "#include \"servers/" << snake_identifier(api_server.name) << "/catalog.hpp\"\n";
    }
    out << "\n#include <vector>\n\n";
    out << "namespace statespec_generated::api\n";
    out << "{\n\n";
    out << "using ApiDescriptor = ::statespec_generated::ApiDescriptor;\n";
    out << "using ApiServerDescriptor = ::statespec_generated::ApiServerDescriptor;\n\n";
    out << "inline std::vector<ApiDescriptor> api_descriptors()\n";
    out << "{\n";
    out << "    return descriptors::api_descriptors();\n";
    out << "}\n\n";
    out << "inline std::vector<ApiServerDescriptor> api_server_descriptors()\n";
    out << "{\n";
    out << "    std::vector<ApiServerDescriptor> descriptors;\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "    {\n";
        out << "        auto slice = servers::" << snake_identifier(api_server.name)
            << "::api_server_descriptors();\n";
        out << "        descriptors.insert(descriptors.end(), slice.begin(), slice.end());\n";
        out << "    }\n";
    }
    out << "    return descriptors;\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::api\n";
    return out.str();
}

} // namespace

#include "generator_cpp_artifact_tier_builders.inc"

} // namespace statespec
