#include "generator_cpp_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_cpp_descriptor_areas.hpp"
#include "generator_cpp_descriptor_support.hpp"
#include "generator_cpp_descriptors.hpp"
#include "generator_entity_index_helpers.hpp"
#include "generator_support.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <algorithm>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace statespec
{

namespace
{

TemplateRenderer::Values cpp_common_runtime_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream includes;
    auto add = [&](std::string_view include) { includes << include_line(include); };
    add("common/schema_compatibility.hpp");
    if (usage.uses_any_runtime_domain)
    {
        add("common/runtime/codec.hpp");
    }
    if (usage.uses_feature_flags)
    {
        add("common/runtime/feature_flag_store.hpp");
    }
    if (usage.uses_queues)
    {
        add("common/runtime/queue_store.hpp");
    }
    if (usage.uses_leases)
    {
        add("common/runtime/lease_store.hpp");
    }
    if (usage.uses_workflows)
    {
        add("common/runtime/workflow_store.hpp");
    }
    if (usage.uses_logs)
    {
        add("common/runtime/log_sink.hpp");
    }
    if (usage.uses_metrics)
    {
        add("common/runtime/metric_sink.hpp");
    }
    if (usage.uses_entity_gc)
    {
        add("common/runtime/entity_gc_descriptors.hpp");
        add("common/runtime/entity_gc_repository.hpp");
        add("common/runtime/entity_gc_workers.hpp");
        add("common/runtime/entity_gc_registration.hpp");
    }
    return TemplateRenderer::Values{{"common_runtime_includes", includes.str()}};
}

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

TemplateRenderer::Values cpp_makefile_values(
    BindingGenerationTier tier,
    const IrSystem& system
)
{
    const auto include_api =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api;
    const auto usage = runtime_domain_usage(system);
    const auto include_worker =
        (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker) &&
        (!system.workers.empty() || usage.uses_workflows);
    const auto include_api_composition = include_api && !system.api_servers.empty();
    const auto include_worker_composition = include_worker && !system.workers.empty();
    const auto include_worker_execution =
        include_worker && (include_worker_composition || usage.uses_workflows);

    std::ostringstream target_additions;
    std::ostringstream phony_targets;
    std::ostringstream help_target_additions;
    std::ostringstream api_rules;
    std::ostringstream worker_rules;
    if (include_api)
    {
        target_additions << "\nCHECK_TARGETS += check-api";
        target_additions << "\nBUILD_TARGETS += build-api";
        target_additions << "\nPACKAGE_TARGETS += package-api";
        phony_targets << " check-api build-api package-api";
        help_target_additions << "\t@printf '%s\\n' '  check-api     build-api     package-api'\n";
        api_rules << "check-api: $(BUILD_DIR)/.dir\n";
        api_rules << "\tprintf '#include \"api/api_descriptors.hpp\"\\n"
                     "#include \"api/api_codecs.hpp\"\\n"
                     "#include \"api/api_handler_registry.hpp\"\\n"
                     "#include \"api/api_handlers.hpp\"\\n"
                     "#include \"api/external_system_operator_metadata_api.hpp\"\\n";
        if (include_api_composition)
        {
            api_rules << "#include \"api/api_application.hpp\"\\n"
                         "#include \"api/api_dispatcher.hpp\"\\n"
                         "#include \"api/api_process.hpp\"\\n"
                         "#include \"api/api_routes.hpp\"\\n"
                         "#include \"api/api_server.hpp\"\\n";
            api_rules << "#include \"api/api_transport.hpp\"\\n";
        }
        api_rules << "int main() { return 0; }\\n' | "
                     "$(CXX) $(CXXFLAGS) -x c++ - -o $(BUILD_DIR)/check-api\n\n";
        api_rules << "build-api: check-api\n";
        if (include_api_composition)
        {
            api_rules << "\t$(CXX) $(CXXFLAGS) api/main.cpp -o $(BUILD_DIR)/api-main\n\n";
        }
        api_rules << "package-api: build-api $(DIST_DIR)\n";
        api_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-api-cpp.tgz common api "
                     "Makefile\n\n";
    }
    if (include_worker)
    {
        target_additions << "\nCHECK_TARGETS += check-worker";
        target_additions << "\nBUILD_TARGETS += build-worker";
        target_additions << "\nPACKAGE_TARGETS += package-worker";
        phony_targets << " check-worker build-worker package-worker";
        help_target_additions
            << "\t@printf '%s\\n' '  check-worker  build-worker  package-worker'\n";
        worker_rules << "check-worker: $(BUILD_DIR)/.dir\n";
        worker_rules << "\tprintf '#include \"worker/worker_contexts.hpp\"\\n"
                        "#include \"worker/worker_descriptors.hpp\"\\n"
                        "#include \"worker/worker_registry.hpp\"\\n";
        if (include_worker_composition)
        {
            worker_rules << "#include \"worker/worker_application.hpp\"\\n"
                            "#include \"worker/worker_process.hpp\"\\n"
                            "#include \"worker/worker_runtime.hpp\"\\n";
        }
        if (usage.uses_leases)
        {
            worker_rules << "#include \"worker/worker_leases.hpp\"\\n";
        }
        if (usage.uses_queues)
        {
            worker_rules << "#include \"worker/worker_queues.hpp\"\\n";
        }
        if (usage.uses_workflows)
        {
            worker_rules << "#include \"worker/worker_workflows.hpp\"\\n";
        }
        if (include_worker_execution)
        {
            worker_rules << "#include \"worker/workflow_runner.hpp\"\\n"
                            "#include \"worker/workflow_step_handlers.hpp\"\\n";
        }
        worker_rules << "int main() { return 0; }\\n' | "
                        "$(CXX) $(CXXFLAGS) -x c++ - -o $(BUILD_DIR)/check-worker\n\n";
        worker_rules << "build-worker: check-worker\n";
        if (include_worker_composition)
        {
            worker_rules << "\t$(CXX) $(CXXFLAGS) worker/main.cpp -o $(BUILD_DIR)/worker-main\n\n";
        }
        worker_rules << "package-worker: build-worker $(DIST_DIR)\n";
        worker_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-worker-cpp.tgz common worker "
                        "Makefile\n\n";
    }
    return TemplateRenderer::Values{
        {"target_additions", target_additions.str()},
        {"phony_targets", phony_targets.str()},
        {"help_target_additions", help_target_additions.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
        {"common_runtime_includes", cpp_common_runtime_values(system)["common_runtime_includes"]},
    };
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
            << "        IWorkflowStepHandler& handler,\n"
            << "        const std::string& workflow_execution_id\n"
            << "    )\n"
            << "    {\n"
            << "        if (!context.executes.has_value())\n"
            << "        {\n"
            << "            return std::nullopt;\n"
            << "        }\n"
            << "        WorkflowRunner runner{\n"
            << "            backend_, workflows_, handler, context.worker_name, "
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
            << "        IWorkflowStepHandler&,\n"
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

std::string cpp_workflow_module_function_name(
    const IrWorkflow& workflow,
    std::string_view suffix
)
{
    return snake_identifier(workflow.name) + std::string(suffix);
}

std::string generate_cpp_workflow_step_module(const IrWorkflow& workflow)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../workflow_step_handlers.hpp\"\n\n";
    out << "#include <optional>\n";
    out << "#include <string>\n\n";
    out << "namespace statespec_generated::worker\n";
    out << "{\n\n";
    out << "inline bool " << cpp_workflow_module_function_name(workflow, "_dispatch_step") << "(\n";
    out << "    IWorkflowStepHandler& handler,\n";
    out << "    const WorkflowStepHandlerContext& context,\n";
    out << "    const statespec::backend::WorkflowExecutionRecord& record\n";
    out << ")\n";
    out << "{\n";
    out << "    if (record.workflow_name != " << cpp_string(workflow.name) << ")\n";
    out << "    {\n";
    out << "        return false;\n";
    out << "    }\n";
    for (const auto& step : workflow.steps)
    {
        out << "    if (record.current_step == " << cpp_string(step.name) << ")\n";
        out << "    {\n";
        out << "        handler.handle_" << snake_identifier(workflow.name + "_" + step.name)
            << "(context);\n";
        out << "        return true;\n";
        out << "    }\n";
    }
    out << "    return false;\n";
    out << "}\n\n";
    out << "inline std::optional<std::string> "
        << cpp_workflow_module_function_name(workflow, "_next_step") << "(\n";
    out << "    const statespec::backend::WorkflowExecutionRecord& record\n";
    out << ")\n";
    out << "{\n";
    out << "    if (record.workflow_name != " << cpp_string(workflow.name) << ")\n";
    out << "    {\n";
    out << "        return std::nullopt;\n";
    out << "    }\n";
    for (const auto& step : workflow.steps)
    {
        for (const auto& statement : step.statements)
        {
            if (statement.kind != "transition_to" || !statement.target.has_value())
            {
                continue;
            }
            out << "    if (record.current_step == " << cpp_string(step.name) << ")\n";
            out << "    {\n";
            out << "        return " << cpp_string(*statement.target) << ";\n";
            out << "    }\n";
        }
    }
    out << "    return std::nullopt;\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::worker\n";
    return out.str();
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
    out << "\n#include <optional>\n";
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
    out << "} // namespace statespec_generated::worker\n";
    return out.str();
}

TemplateRenderer::Values cpp_workflow_runner_values(const IrSystem& system)
{
    std::ostringstream includes;
    std::ostringstream dispatch_cases;
    std::ostringstream next_cases;
    for (const auto& workflow : system.workflows)
    {
        const auto snake = snake_identifier(workflow.name);
        includes << "#include \"workflows/" << snake << ".hpp\"\n";
        dispatch_cases << "            if (!handled)\n";
        dispatch_cases << "            {\n";
        dispatch_cases << "                handled = "
                       << cpp_workflow_module_function_name(workflow, "_dispatch_step")
                       << "(handler_, context, record);\n";
        dispatch_cases << "            }\n";
        next_cases << "            if (!next_step.has_value())\n";
        next_cases << "            {\n";
        next_cases << "                next_step = "
                   << cpp_workflow_module_function_name(workflow, "_next_step") << "(record);\n";
        next_cases << "            }\n";
    }
    return TemplateRenderer::Values{
        {"workflow_step_module_includes", includes.str()},
        {"workflow_step_module_dispatch_cases", dispatch_cases.str()},
        {"workflow_step_module_next_cases", next_cases.str()},
    };
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
    return "api/handlers/" + snake_identifier(std::string{domain_name}) + ".hpp";
}

std::string cpp_api_handler_domain_include_path(std::string_view domain_name)
{
    return "handlers/" + snake_identifier(std::string{domain_name}) + ".hpp";
}

std::string cpp_api_handler_registry_domain_includes(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        out << "#include \"" << cpp_api_handler_domain_include_path(domain.name) << "\"\n";
    }
    return out.str();
}

std::string cpp_api_handler_registry_delegates(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        const auto class_name = cpp_api_handler_domain_class_name(domain.name);
        for (const auto& api : domain.apis)
        {
            out << "    ApiResponse handle_" << snake_identifier(api.name)
                << "(const ApiRequestContext& context) override\n";
            out << "    {\n";
            out << "        " << class_name << " handler{backend_};\n";
            out << "        return handler.handle_" << snake_identifier(api.name) << "(context);\n";
            out << "    }\n\n";
        }
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
        ApiHandlerDomain filtered{domain.name, {}};
        for (const auto& api : domain.apis)
        {
            if (is_entity_crud_api(api))
            {
                filtered.apis.push_back(api);
            }
        }
        if (!filtered.apis.empty())
        {
            result.push_back(std::move(filtered));
        }
    }
    return result;
}

std::string cpp_business_api_handler_delegates(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        if (is_entity_crud_api(api))
        {
            continue;
        }
        out << "    ApiResponse handle_" << snake_identifier(api.name)
            << "(const ApiRequestContext& context) override\n";
        out << "    {\n";
        out << "        if (business_handler_ == nullptr)\n";
        out << "        {\n";
        out << "            return ApiResponse{501, statespec::backend::Json::object({})};\n";
        out << "        }\n";
        out << "        return business_handler_->handle_" << snake_identifier(api.name)
            << "(context);\n";
        out << "    }\n\n";
    }
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
    out << "#include \"../api_codecs.hpp\"\n";
    out << "#include \"../api_handler_registry_support.hpp\"\n";
    for (const auto& entity : system.entities)
    {
        out << "#include \"../../common/entities/" << snake_identifier(entity.name)
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

std::string cpp_api_codec_includes(const std::vector<IrShape>& shapes)
{
    std::ostringstream out;
    out << "#include \"api_codec_support.hpp\"\n";
    for (const auto& shape : shapes)
    {
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
    out << "#include \"../api_codec_support.hpp\"\n\n";
    out << "namespace statespec_generated::api\n";
    out << "{\n\n";
    out << generate_api_codec_operations(filtered);
    out << "} // namespace statespec_generated::api\n";
    return out.str();
}

TemplateRenderer::Values cpp_entity_gc_descriptor_values(const IrSystem& system)
{
    std::ostringstream includes;
    std::ostringstream descriptor_functions;
    std::ostringstream descriptor_calls;
    for (const auto& entity : system.entities)
    {
        std::ostringstream terminal_states;
        for (const auto& state : entity.states)
        {
            if (!state.garbage_collection.has_value())
            {
                continue;
            }
            terminal_states << "                EntityGcTerminalStateDescriptor{"
                            << "::statespec_generated::entities::" << snake_identifier(entity.name)
                            << "::" << cpp_entity_state_constant_name(entity.name, state.name)
                            << ", "
                            << "::statespec_generated::entities::" << snake_identifier(entity.name)
                            << "::k" << pascal_identifier(entity.name) << "State"
                            << pascal_identifier(state.name) << "GcAfter, "
                            << "::statespec_generated::entities::" << snake_identifier(entity.name)
                            << "::k" << pascal_identifier(entity.name) << "State"
                            << pascal_identifier(state.name) << "GcMode},\n";
        }
        if (terminal_states.str().empty())
        {
            continue;
        }
        includes << "#include \"../entities/" << snake_identifier(entity.name) << "/model.hpp\"\n";
        includes << "#include \"../entities/" << snake_identifier(entity.name) << "/schema.hpp\"\n";
        const auto descriptor_function = snake_identifier(entity.name) + "_entity_gc_descriptor";
        descriptor_functions
            << "inline EntityGcDescriptor " << descriptor_function << "()\n"
            << "{\n"
            << "    return EntityGcDescriptor{\n"
            << "        ::statespec_generated::entities::" << snake_identifier(entity.name)
            << "::" << cpp_entity_name_constant_name(entity.name) << ",\n"
            << "        ::statespec_generated::entities::" << snake_identifier(entity.name)
            << "::" << cpp_entity_name_constant_name(entity.name) << ",\n"
            << "        ::statespec_generated::entities::" << snake_identifier(entity.name)
            << "::" << cpp_entity_field_constant_name(entity.name, "status") << ",\n"
            << "        std::vector<EntityGcTerminalStateDescriptor>{\n"
            << terminal_states.str() << "        }\n"
            << "    };\n"
            << "}\n\n";
        descriptor_calls << "        " << descriptor_function << "(),\n";
    }
    if (!includes.str().empty())
    {
        includes << "\n";
    }
    return TemplateRenderer::Values{
        {"entity_gc_entity_includes", includes.str()},
        {"entity_gc_descriptor_functions", descriptor_functions.str()},
        {"entity_gc_descriptor_calls", descriptor_calls.str()},
    };
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
        {"api_main_entity_gc_include",
         "#include \"../common/runtime/entity_gc_registration.hpp\"\n"},
        {"api_main_entity_gc_registration",
         "        statespec::backend::runtime::register_entity_gc_workers(process, backend);\n\n"},
    };
}

TemplateRenderer::Values cpp_worker_main_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    if (!usage.uses_entity_gc)
    {
        return TemplateRenderer::Values{
            {"worker_main_entity_gc_include", ""},
            {"worker_main_entity_gc_registration", ""},
        };
    }
    return TemplateRenderer::Values{
        {"worker_main_entity_gc_include",
         "#include \"../common/runtime/entity_gc_registration.hpp\"\n"},
        {"worker_main_entity_gc_registration",
         "        statespec::backend::runtime::register_entity_gc_workers(runtime, backend);\n\n"},
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
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#define STATESPEC_GENERATED_DESCRIPTOR_TYPES_DEFINED 1\n\n";
    out << "#include \"../backend.hpp\"\n";
    out << "#include \"../external_system.hpp\"\n\n";
    out << "#include <chrono>\n";
    out << "#include <cstdint>\n";
    out << "#include <map>\n";
    out << "#include <optional>\n";
    out << "#include <string>\n";
    out << "#include <string_view>\n";
    out << "#include <utility>\n";
    out << "#include <vector>\n\n";
    out << "namespace statespec_generated\n";
    out << "{\n\n";
    out << "struct LeaseDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> resource;\n";
    out << "    std::chrono::seconds ttl;\n";
    out << "    std::optional<std::chrono::seconds> renew_every;\n";
    out << "    std::optional<std::string> holder;\n";
    out << "    bool fencing_token = false;\n";
    out << "    std::optional<std::chrono::seconds> max_ttl;\n";
    out << "};\n\n";
    out << "struct FeatureFlagDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string type;\n";
    out << "    std::string default_value;\n";
    out << "    std::string scope;\n";
    out << "    std::optional<std::string> owner;\n";
    out << "    std::optional<std::string> description;\n";
    out << "    std::optional<std::string> expires;\n";
    out << "};\n\n";
    out << "struct ValueDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string type;\n";
    out << "    std::optional<std::string> constraint;\n";
    out << "};\n\n";
    out << "struct EnumMemberDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> value;\n";
    out << "};\n\n";
    out << "struct EnumDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<EnumMemberDescriptor> members;\n";
    out << "};\n\n";
    out << "struct EventDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> fields;\n";
    out << "};\n\n";
    out << "struct ExternalSystemPropertyDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string value;\n";
    out << "};\n\n";
    out << "struct ExternalSystemMetadataMappingDescriptor\n";
    out << "{\n";
    out << "    std::string source;\n";
    out << "    std::string target;\n";
    out << "};\n\n";
    out << "struct ExternalSystemMetadataMappingAssignment\n";
    out << "{\n";
    out << "    std::string source;\n";
    out << "    std::string source_root;\n";
    out << "    std::string source_field;\n";
    out << "    std::string target_root;\n";
    out << "    std::string field;\n";
    out << "};\n\n";
    out << "struct ExternalSystemMetadataMappingPlan\n";
    out << "{\n";
    out << "    std::vector<ExternalSystemMetadataMappingAssignment> all_mappings;\n";
    out << "    std::vector<ExternalSystemMetadataMappingAssignment> client_mappings;\n";
    out << "    std::vector<ExternalSystemMetadataMappingAssignment> request_mappings;\n";
    out << "};\n\n";
    out << "struct ExternalSystemMetadataMissingMappingSource\n";
    out << "{\n";
    out << "    std::string source;\n";
    out << "    std::string source_root;\n";
    out << "    std::string source_field;\n";
    out << "    std::string target_root;\n";
    out << "    std::string field;\n";
    out << "};\n\n";
    out << "struct ExternalSystemMetadataMappingInputs\n";
    out << "{\n";
    out << "    std::map<std::string, statespec::backend::Json> input;\n";
    out << "    std::map<std::string, statespec::backend::Json> entity;\n";
    out << "    std::map<std::string, statespec::backend::Json> workflow;\n";
    out << "    std::map<std::string, statespec::backend::Json> metadata;\n\n";
    out << "    const statespec::backend::Json* source_value(\n";
    out << "        std::string_view source_root,\n";
    out << "        std::string_view source_field\n";
    out << "    ) const\n";
    out << "    {\n";
    out << "        const auto* values = source_root == \"input\" ? &input\n";
    out << "            : source_root == \"entity\"                 ? &entity\n";
    out << "            : source_root == \"workflow\"               ? &workflow\n";
    out << "            : source_root == \"metadata\"               ? &metadata\n";
    out << "                                                       : nullptr;\n";
    out << "        if (values == nullptr)\n";
    out << "        {\n";
    out << "            return nullptr;\n";
    out << "        }\n";
    out << "        const auto value = values->find(std::string(source_field));\n";
    out << "        return value == values->end() ? nullptr : &value->second;\n";
    out << "    }\n\n";
    out << "    const statespec::backend::Json* source_value(\n";
    out << "        const ExternalSystemMetadataMappingAssignment& assignment\n";
    out << "    ) const\n";
    out << "    {\n";
    out << "        return source_value(assignment.source_root, assignment.source_field);\n";
    out << "    }\n";
    out << "};\n\n";
    out << "struct ExternalSystemMetadataMappingOutput\n";
    out << "{\n";
    out << "    std::map<std::string, statespec::backend::Json> client_config;\n";
    out << "    std::map<std::string, statespec::backend::Json> request_payload;\n";
    out << "    std::vector<ExternalSystemMetadataMissingMappingSource> missing_sources;\n";
    out << "};\n\n";
    out << "inline std::vector<ExternalSystemMetadataMissingMappingSource> "
           "missing_external_system_metadata_mapping_sources(\n";
    out << "    const ExternalSystemMetadataMappingPlan& plan,\n";
    out << "    const ExternalSystemMetadataMappingInputs& inputs\n";
    out << ")\n";
    out << "{\n";
    out << "    std::vector<ExternalSystemMetadataMissingMappingSource> missing;\n";
    out << "    for (const auto& assignment : plan.all_mappings)\n";
    out << "    {\n";
    out << "        if (inputs.source_value(assignment) == nullptr)\n";
    out << "        {\n";
    out << "            missing.push_back(ExternalSystemMetadataMissingMappingSource{\n";
    out << "                assignment.source,\n";
    out << "                assignment.source_root,\n";
    out << "                assignment.source_field,\n";
    out << "                assignment.target_root,\n";
    out << "                assignment.field,\n";
    out << "            });\n";
    out << "        }\n";
    out << "    }\n";
    out << "    return missing;\n";
    out << "}\n\n";
    out << "class IExternalSystemMetadataMappingApplicator\n";
    out << "{\n";
    out << "public:\n";
    out << "    virtual ~IExternalSystemMetadataMappingApplicator() = default;\n";
    out << "    virtual ExternalSystemMetadataMappingOutput apply(\n";
    out << "        const ExternalSystemMetadataMappingPlan& plan,\n";
    out << "        const ExternalSystemMetadataMappingInputs& inputs\n";
    out << "    ) = 0;\n";
    out << "};\n\n";
    out << templates.load("generated/external_system_runtime.hpp.tmpl") << "\n";
    out << "struct ExternalSystemMetadataDescriptor\n";
    out << "{\n";
    out << "    std::string entity;\n";
    out << "    std::optional<std::string> tenant_field;\n";
    out << "    std::string profile_field;\n";
    out << "    std::vector<std::string> key_fields;\n";
    out << "    std::vector<std::string> required_fields;\n";
    out << "    std::vector<ExternalSystemMetadataMappingDescriptor> mappings;\n";
    out << "};\n\n";
    out << "struct ExternalSystemOperatorMetadataUpsertRequest\n";
    out << "{\n";
    out << "    statespec::backend::ExternalSystemMetadataLookup lookup;\n";
    out << "    statespec::backend::Json document;\n";
    out << "    std::optional<statespec::backend::Version> expected_version;\n";
    out << "};\n\n";
    out << "struct ExternalSystemOperatorMetadataGetRequest\n";
    out << "{\n";
    out << "    statespec::backend::ExternalSystemMetadataLookup lookup;\n";
    out << "};\n\n";
    out << "struct ExternalSystemOperatorMetadataDisableRequest\n";
    out << "{\n";
    out << "    statespec::backend::ExternalSystemMetadataLookup lookup;\n";
    out << "    std::optional<statespec::backend::Version> expected_version;\n";
    out << "    std::string disabled_status = \"Disabled\";\n";
    out << "};\n\n";
    out << "struct ExternalSystemOperatorMetadataDeleteRequest\n";
    out << "{\n";
    out << "    statespec::backend::ExternalSystemMetadataLookup lookup;\n";
    out << "    std::optional<statespec::backend::Version> expected_version;\n";
    out << "    std::string deleted_status = \"Deleted\";\n";
    out << "};\n\n";
    out << "class IExternalSystemOperatorMetadataRepository\n";
    out << "{\n";
    out << "public:\n";
    out << "    virtual ~IExternalSystemOperatorMetadataRepository() = default;\n";
    out << "    virtual std::optional<statespec::backend::VersionedRecord> upsert_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataUpsertRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual std::optional<statespec::backend::VersionedRecord> get_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataGetRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual std::optional<statespec::backend::VersionedRecord> disable_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataDisableRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual std::optional<statespec::backend::VersionedRecord> delete_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        const ExternalSystemOperatorMetadataDeleteRequest& request\n";
    out << "    ) = 0;\n";
    out << "};\n\n";
    out << templates.load("generated/external_system_metadata_runtime.hpp.tmpl") << "\n";
    out << "struct ExternalSystemDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<ExternalSystemPropertyDescriptor> properties;\n";
    out << "    std::optional<ExternalSystemMetadataDescriptor> metadata;\n";
    out << "};\n\n";
    out << "struct ApiDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> method;\n";
    out << "    std::optional<std::string> path;\n";
    out << "    std::optional<std::string> input;\n";
    out << "    std::optional<std::string> output;\n";
    out << "    std::optional<std::string> error;\n";
    out << "    std::optional<std::string> starts_workflow;\n";
    out << "    std::optional<std::string> enqueues;\n";
    out << "};\n\n";
    out << "struct ApiServerDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<std::string> serves;\n";
    out << "    int concurrency = 1;\n";
    out << "};\n\n";
    out << "struct ApiRouteDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string server_name;\n";
    out << "    std::string api_name;\n";
    out << "    std::optional<std::string> method;\n";
    out << "    std::optional<std::string> path;\n";
    out << "    std::optional<std::string> input;\n";
    out << "    std::optional<std::string> output;\n";
    out << "    std::optional<std::string> error;\n";
    out << "};\n\n";
    out << "struct ApiRequestContext\n";
    out << "{\n";
    out << "    std::string server_name;\n";
    out << "    std::string api_name;\n";
    out << "    std::optional<std::string> method;\n";
    out << "    std::optional<std::string> path;\n";
    out << "    statespec::backend::Json body;\n";
    out << "};\n\n";
    out << "struct ApiResponse\n";
    out << "{\n";
    out << "    int status_code = 200;\n";
    out << "    statespec::backend::Json body;\n";
    out << "};\n\n";
    out << "class IExternalSystemOperatorMetadataApiHandler\n";
    out << "{\n";
    out << "public:\n";
    out << "    virtual ~IExternalSystemOperatorMetadataApiHandler() = default;\n";
    out << "    virtual ApiResponse handle_upsert_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        IExternalSystemOperatorMetadataRepository& repository,\n";
    out << "        const ExternalSystemOperatorMetadataUpsertRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual ApiResponse handle_get_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        IExternalSystemOperatorMetadataRepository& repository,\n";
    out << "        const ExternalSystemOperatorMetadataGetRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual ApiResponse handle_disable_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        IExternalSystemOperatorMetadataRepository& repository,\n";
    out << "        const ExternalSystemOperatorMetadataDisableRequest& request\n";
    out << "    ) = 0;\n";
    out << "    virtual ApiResponse handle_delete_metadataTx(\n";
    out << "        statespec::backend::ITransaction& tx,\n";
    out << "        IExternalSystemOperatorMetadataRepository& repository,\n";
    out << "        const ExternalSystemOperatorMetadataDeleteRequest& request\n";
    out << "    ) = 0;\n";
    out << "};\n\n";
    out << "struct WorkerDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    bool singleton = false;\n";
    out << "    std::optional<std::string> lease;\n";
    out << "    std::optional<std::string> polls;\n";
    out << "    std::optional<std::string> executes;\n";
    out << "    int concurrency = 1;\n";
    out << "};\n\n";
    out << "struct WorkerContext\n";
    out << "{\n";
    out << "    std::string worker_name;\n";
    out << "    bool singleton = false;\n";
    out << "    std::optional<std::string> lease;\n";
    out << "    std::optional<std::string> polls;\n";
    out << "    std::optional<std::string> executes;\n";
    out << "    int concurrency = 1;\n";
    out << "};\n\n";
    out << "struct PolicyRuleDescriptor\n";
    out << "{\n";
    out << "    std::string action;\n";
    out << "    std::string condition;\n";
    out << "};\n\n";
    out << "struct QuotaDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string expression;\n";
    out << "};\n\n";
    out << "struct PolicyDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::optional<std::string> tenant_scoped_by;\n";
    out << "    std::vector<PolicyRuleDescriptor> allows;\n";
    out << "    std::vector<PolicyRuleDescriptor> denies;\n";
    out << "    std::vector<QuotaDescriptor> quotas;\n";
    out << "    std::vector<std::string> audits;\n";
    out << "};\n\n";
    out << "struct ShapeDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> fields;\n";
    out << "};\n\n";
    out << "struct LogDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string level;\n";
    out << "    std::string event_name;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> fields;\n";
    out << "};\n\n";
    out << "struct MetricDefinition\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string kind;\n";
    out << "    std::string backend_name;\n";
    out << "    std::string unit;\n";
    out << "    std::vector<statespec::backend::FieldDescriptor> labels;\n";
    out << "};\n\n";
    out << "struct GarbageCollectionPolicy\n";
    out << "{\n";
    out << "    std::string after;\n";
    out << "    std::string mode;\n";
    out << "};\n\n";
    out << "struct EntityStateDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    bool terminal = false;\n";
    out << "    std::optional<GarbageCollectionPolicy> garbage_collection;\n";
    out << "};\n\n";
    out << "struct EntityOwnershipDescriptor\n";
    out << "{\n";
    out << "    std::string authority;\n";
    out << "    std::string system_of_record;\n";
    out << "    std::string lifecycle;\n";
    out << "};\n\n";
    out << "struct EntityRelationDescriptor\n";
    out << "{\n";
    out << "    std::string kind;\n";
    out << "    std::string name;\n";
    out << "    std::string target;\n";
    out << "    bool optional = false;\n";
    out << "    std::optional<std::string> relation_kind;\n";
    out << "    std::optional<std::string> on_parent_delete;\n";
    out << "    std::vector<std::string> parent_must_be_in;\n";
    out << "    std::vector<std::string> unique_within_parent;\n";
    out << "};\n\n";
    out << "struct EntityChildDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string target_entity;\n";
    out << "    std::string relation;\n";
    out << "};\n\n";
    out << "struct EntityInvariantDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::string expression;\n";
    out << "};\n\n";
    out << "struct EntityDescriptor\n";
    out << "{\n";
    out << "    std::string name;\n";
    out << "    std::vector<std::string> key_fields;\n";
    out << "    std::optional<EntityOwnershipDescriptor> ownership;\n";
    out << "    std::vector<EntityRelationDescriptor> relations;\n";
    out << "    std::vector<EntityChildDescriptor> children;\n";
    out << "    std::vector<EntityInvariantDescriptor> invariants;\n";
    out << "    std::vector<EntityStateDescriptor> states;\n";
    out << "    std::optional<std::string> initial_state;\n";
    out << "    std::vector<std::string> terminal_states;\n";
    out << "};\n\n";
    out << "} // namespace statespec_generated\n";
    return out.str();
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
        out << "inline constexpr const char* " << cpp_entity_name_constant_name(entity.name)
            << " = " << cpp_string(entity.name) << ";\n";
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
        }
        for (const auto& state : entity.states)
        {
            out << "inline constexpr const char* "
                << cpp_entity_state_constant_name(entity.name, state.name) << " = "
                << cpp_string(state.name) << ";\n";
        }
        for (const auto& relation : entity.relations)
        {
            const auto relation_constant_prefix = "k" + pascal_identifier(entity.name) +
                                                  "Relation" + pascal_identifier(relation.name);
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
        out << "\n";
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
    return out.str();
}

std::string cpp_event_descriptor_module(const IrSystem& system)
{
    std::ostringstream out;
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
    out << "struct " << pascal_identifier(shape.name) << "\n";
    out << "{\n";
    for (const auto& field : shape.fields)
    {
        out << "    " << cpp_shape_type(field.type) << " " << field.name << "{};\n";
    }
    out << "};\n\n";
    out << "} // namespace statespec_generated\n";
    return out.str();
}

std::string cpp_shapes_umbrella_header(const IrSystem& system)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    for (const auto& shape : system.shapes)
    {
        out << "#include \"shapes/" << snake_identifier(shape.name) << ".hpp\"\n";
    }
    return out.str();
}

void add_cpp_shape_type_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    const auto shared_shapes = with_shapes_matching(system, false);
    if (!shared_shapes.shapes.empty())
    {
        add_cpp_raw_common_file(
            result, options, "shapes.hpp", cpp_shapes_umbrella_header(shared_shapes)
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
    const IrSystem& system
)
{
    const auto api_shapes = with_shapes_matching(system, true);
    if (api_shapes.shapes.empty())
    {
        return;
    }
    add_cpp_raw_api_file(result, options, "api/shapes.hpp", cpp_shapes_umbrella_header(api_shapes));
    for (const auto& shape : api_shapes.shapes)
    {
        add_cpp_raw_api_file(
            result, options, "api/shapes/" + snake_identifier(shape.name) + ".hpp",
            cpp_shape_type_header(shape, "../../common/backend.hpp")
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
    add_cpp_descriptor_module_artifact(
        result, options, templates, "descriptors/core.hpp", "core descriptors", diagnostics
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
        result, options, templates, "descriptors/shapes.hpp", "shape descriptors", diagnostics,
        cpp_shape_descriptor_module_values(system)
    );
    for (const auto& shape : system.shapes)
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            generated_template_path("descriptor_module.hpp.tmpl"),
            common_artifact_path("descriptors/shapes/" + snake_identifier(shape.name) + ".hpp"),
            diagnostics, GeneratedArtifactTier::Common, cpp_shape_descriptor_module_values(shape)
        );
    }
    add_cpp_descriptor_module_artifact(
        result, options, templates, "descriptors/runtime.hpp", "runtime descriptors", diagnostics
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
            result, options, entity_dir + "model.hpp",
            cpp_entity_centered_facade_header(entity, "model")
        );
        add_cpp_raw_common_file(
            result, options, entity_dir + "persistence.hpp",
            cpp_entity_centered_facade_header(entity, "persistence")
        );
        add_cpp_raw_common_file(
            result, options, entity_dir + "schema.hpp",
            cpp_entity_centered_facade_header(entity, "schema")
        );
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

std::string cpp_api_descriptor_module(
    const IrSystem& system,
    const IrApi& api
)
{
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../../common/descriptors/types.hpp\"\n\n";
    out << "namespace statespec_generated::api::descriptors\n";
    out << "{\n\n";
    out << "inline std::vector<::statespec_generated::ApiDescriptor> "
        << cpp_api_descriptor_function_name(api) << "()\n";
    out << "{\n";
    out << "    return {\n";
    out << "        ::statespec_generated::ApiDescriptor{\n";
    out << "            " << cpp_string(api.name) << ",\n";
    out << "            " << optional_string_expr(api.method) << ",\n";
    out << "            " << optional_string_expr(api.path) << ",\n";
    out << "            " << optional_string_expr(api.input) << ",\n";
    out << "            " << optional_string_expr(api.output) << ",\n";
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
        out << "            " << optional_string_expr(api.path) << ",\n";
        out << "            " << optional_string_expr(api.input) << ",\n";
        out << "            " << optional_string_expr(api.output) << ",\n";
        out << "            " << optional_string_expr(api.error) << ",\n";
        out << "        },\n";
    }
    out << "    };\n";
    out << "}\n\n";
    out << "} // namespace statespec_generated::api::descriptors\n";
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
        add_cpp_raw_api_file(
            result, options, "api/descriptors/" + snake_identifier(api.name) + ".hpp",
            cpp_api_descriptor_module(system, api)
        );
    }
}

TemplateRenderer::Values cpp_api_descriptor_values(
    const IrSystem& system,
    std::string_view include_prefix
)
{
    std::ostringstream includes;
    std::ostringstream api_aggregation;
    std::ostringstream server_descriptors;
    for (const auto& api : system.apis)
    {
        includes << "\n#include \"" << include_prefix << snake_identifier(api.name) << ".hpp\"";
        const auto module = cpp_api_descriptor_function_name(api) + "()";
        api_aggregation << "    {\n";
        api_aggregation << "        auto slice = " << module << ";\n";
        api_aggregation << "        descriptors.insert(descriptors.end(), slice.begin(), "
                           "slice.end());\n";
        api_aggregation << "    }\n";
    }
    server_descriptors << "    return {\n";
    for (const auto& api_server : system.api_servers)
    {
        server_descriptors << "        ApiServerDescriptor{\n";
        server_descriptors << "            " << cpp_string(api_server.name) << ",\n";
        server_descriptors << "            {";
        for (std::size_t i = 0; i < api_server.serves.size(); ++i)
        {
            if (i > 0)
            {
                server_descriptors << ", ";
            }
            server_descriptors << cpp_string(api_server.serves[i]);
        }
        server_descriptors << "},\n";
        server_descriptors << "            " << api_server.concurrency.value_or(1) << ",\n";
        server_descriptors << "        },\n";
    }
    server_descriptors << "    };\n";
    return TemplateRenderer::Values{
        {"api_descriptor_includes", includes.str()},
        {"api_descriptor_aggregation", api_aggregation.str()},
        {"api_server_descriptors", server_descriptors.str()},
    };
}

TemplateRenderer::Values cpp_api_route_values(
    const IrSystem& system,
    std::string_view include_prefix
)
{
    std::ostringstream includes;
    std::ostringstream route_aggregation;
    for (const auto& api : system.apis)
    {
        includes << "\n#include \"" << include_prefix << snake_identifier(api.name) << ".hpp\"";
        const auto route_module = cpp_api_route_descriptor_function_name(api) + "()";
        route_aggregation << "    {\n";
        route_aggregation << "        auto slice = " << route_module << ";\n";
        route_aggregation << "        descriptors.insert(descriptors.end(), slice.begin(), "
                             "slice.end());\n";
        route_aggregation << "    }\n";
    }
    return TemplateRenderer::Values{
        {"api_descriptor_includes", includes.str()},
        {"api_route_descriptor_aggregation", route_aggregation.str()},
    };
}

std::string cpp_api_descriptor_catalog_file(const IrSystem& system)
{
    const auto descriptor_values = cpp_api_descriptor_values(system, {});
    const auto route_values = cpp_api_route_values(system, {});
    std::ostringstream out;
    out << "#pragma once\n\n";
    out << "#include \"../../common/descriptors/types.hpp\"\n";
    out << descriptor_values.at("api_descriptor_includes") << "\n\n";
    out << "namespace statespec_generated::api::descriptors\n";
    out << "{\n\n";
    out << "using ApiDescriptor = ::statespec_generated::ApiDescriptor;\n";
    out << "using ApiRouteDescriptor = ::statespec_generated::ApiRouteDescriptor;\n";
    out << "using ApiServerDescriptor = ::statespec_generated::ApiServerDescriptor;\n\n";
    out << "inline std::vector<ApiDescriptor> api_descriptors()\n";
    out << "{\n";
    out << "    std::vector<ApiDescriptor> descriptors;\n";
    out << descriptor_values.at("api_descriptor_aggregation");
    out << "    return descriptors;\n";
    out << "}\n\n";
    out << "inline std::vector<ApiServerDescriptor> api_server_descriptors()\n";
    out << "{\n";
    out << descriptor_values.at("api_server_descriptors");
    out << "}\n\n";
    out << "inline std::vector<ApiRouteDescriptor> api_route_descriptors()\n";
    out << "{\n";
    out << "    std::vector<ApiRouteDescriptor> descriptors;\n";
    out << route_values.at("api_route_descriptor_aggregation");
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

} // namespace

void add_cpp_common_runtime_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto usage = runtime_domain_usage(system);

    add_template_file(result, options.output_dir, templates, "json.hpp", "json.hpp", diagnostics);
    add_template_file(
        result, options.output_dir, templates, "backend.hpp", "backend.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "external_system.hpp", "external_system.hpp",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "feature_flag.hpp", "feature_flag.hpp", diagnostics
    );
    add_template_file(result, options.output_dir, templates, "lease.hpp", "lease.hpp", diagnostics);
    add_template_file(result, options.output_dir, templates, "log.hpp", "log.hpp", diagnostics);
    add_template_file(
        result, options.output_dir, templates, "metric.hpp", "metric.hpp", diagnostics
    );
    add_template_file(result, options.output_dir, templates, "queue.hpp", "queue.hpp", diagnostics);
    add_template_file(
        result, options.output_dir, templates, "schema_compatibility.hpp",
        "schema_compatibility.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "workflow.hpp", "workflow.hpp", diagnostics
    );
    add_cpp_raw_common_file(
        result, options, "descriptors/types.hpp", cpp_descriptor_types_header(templates)
    );
    add_cpp_raw_common_file(
        result, options, "entity_repository.hpp",
        std::string{"#pragma once\n\n#include \"backend.hpp\"\n\n"
                    "namespace statespec_generated\n{\n\n"} +
            templates.load("generated/entity_repository.hpp.tmpl") +
            "\n} // namespace statespec_generated\n"
    );
    add_cpp_raw_common_file(
        result, options, "runtime_registration.hpp",
        std::string{"#pragma once\n\n#include \"descriptors.hpp\"\n\n"
                    "namespace statespec_generated\n{\n\n"} +
            cpp_runtime_registration_includes(system) + "\n" +
            generate_cpp_runtime_registration(system, templates) +
            "\n} // namespace statespec_generated\n"
    );
    add_template_file(
        result, options.output_dir, templates, "memory/transaction.hpp", "memory/transaction.hpp",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "memory/backend.hpp", "memory/backend.hpp",
        diagnostics
    );
    if (usage.uses_any_runtime_domain)
    {
        add_cpp_common_generated_template_file(
            result, options, templates, "runtime/codec.hpp", diagnostics,
            cpp_runtime_codec_values(system)
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_core.hpp",
            "runtime/codec_core.hpp", diagnostics
        );
    }
    if (usage.uses_feature_flags)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_feature_flags.hpp",
            "runtime/codec_feature_flags.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/feature_flag_store.hpp",
            "runtime/feature_flag_store.hpp", diagnostics
        );
    }
    if (usage.uses_queues)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_queues.hpp",
            "runtime/codec_queues.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/queue_store.hpp",
            "runtime/queue_store.hpp", diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_leases.hpp",
            "runtime/codec_leases.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/lease_store.hpp",
            "runtime/lease_store.hpp", diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_workflows.hpp",
            "runtime/codec_workflows.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/workflow_store.hpp",
            "runtime/workflow_store.hpp", diagnostics
        );
    }
    if (usage.uses_observability)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_observability.hpp",
            "runtime/codec_observability.hpp", diagnostics
        );
    }
    if (usage.uses_logs)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_logs.hpp",
            "runtime/codec_logs.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/log_sink.hpp", "runtime/log_sink.hpp",
            diagnostics
        );
    }
    if (usage.uses_metrics)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_metrics.hpp",
            "runtime/codec_metrics.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/metric_sink.hpp",
            "runtime/metric_sink.hpp", diagnostics
        );
    }
    if (usage.uses_entity_gc)
    {
        add_cpp_common_generated_template_file(
            result, options, templates, "runtime/entity_gc_descriptors.hpp", diagnostics,
            cpp_entity_gc_descriptor_values(system)
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/entity_gc_repository.hpp",
            "runtime/entity_gc_repository.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/entity_gc_workers.hpp",
            "runtime/entity_gc_workers.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/entity_gc_registration.hpp",
            "runtime/entity_gc_registration.hpp", diagnostics
        );
    }

    if (diagnostics.has_errors())
    {
        return;
    }

    add_cpp_shape_type_artifacts(result, options, system);
    add_cpp_descriptor_module_artifacts(result, options, templates, system, diagnostics);
    add_cpp_workflow_descriptor_artifacts(result, options, system);
    if (diagnostics.has_errors())
    {
        return;
    }

    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("descriptors.hpp.tmpl"),
        common_artifact_path("descriptors.hpp"), diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{
            {"system_descriptors", generate_system_descriptors_header(system, templates)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Makefile.tmpl"),
        artifact_path(GeneratedArtifactMakefile), diagnostics, GeneratedArtifactTier::Common,
        cpp_makefile_values(options.tier, system), common_artifact_path(GeneratedArtifactMakefile)
    );
}

void add_cpp_api_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto include_api_composition = !system.api_servers.empty();

    add_cpp_api_shape_type_artifacts(result, options, system);
    add_cpp_api_descriptor_artifacts(result, options, system);
    add_cpp_raw_api_file(
        result, options, "api/descriptors/catalog.hpp", cpp_api_descriptor_catalog_file(system)
    );
    add_cpp_generated_template_file(
        result, options, templates, "api/api_descriptors.hpp", GeneratedArtifactTier::Api,
        diagnostics
    );
    add_cpp_generated_template_file(
        result, options, templates, "api/api_codecs.hpp", GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_codec_includes", cpp_api_codec_includes(api_codec_shapes(system))}
        }
    );
    add_cpp_generated_template_file(
        result, options, templates, "api/api_codec_support.hpp", GeneratedArtifactTier::Api,
        diagnostics,
        TemplateRenderer::Values{
            {"api_codec_helpers", generate_api_codec_helpers()},
            {"api_shape_include",
             api_codec_shapes(system).empty() ? "" : "#include \"shapes.hpp\"\n"}
        }
    );
    for (const auto& shape : api_codec_shapes(system))
    {
        add_cpp_raw_api_file(
            result, options, cpp_api_codec_shape_path(shape.name),
            cpp_api_codec_shape_file(system, shape)
        );
    }
    add_cpp_generated_template_file(
        result, options, templates, "api/api_handlers.hpp", GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_operation_handler_methods", generate_api_operation_handler_methods(system)},
            {"business_api_operation_handler_methods",
             generate_business_api_operation_handler_methods(system)}
        }
    );
    add_cpp_generated_template_file(
        result, options, templates, "api/api_handler_registry_support.hpp",
        GeneratedArtifactTier::Api, diagnostics
    );
    const auto handler_domains = crud_api_handler_domains(api_handler_domains(system));
    for (const auto& domain : handler_domains)
    {
        add_cpp_raw_api_file(
            result, options, cpp_api_handler_domain_path(domain.name),
            cpp_api_handler_domain_file(system, domain)
        );
    }
    add_cpp_generated_template_file(
        result, options, templates, "api/api_handler_registry.hpp", GeneratedArtifactTier::Api,
        diagnostics,
        TemplateRenderer::Values{
            {"api_operation_default_handler_methods",
             cpp_api_handler_registry_delegates(handler_domains) +
                 cpp_business_api_handler_delegates(system)},
            {"api_handler_registry_domain_includes",
             cpp_api_handler_registry_domain_includes(handler_domains)}
        }
    );
    add_cpp_generated_template_file(
        result, options, templates, "api/external_system_operator_metadata_api.hpp",
        GeneratedArtifactTier::Api, diagnostics
    );
    if (include_api_composition)
    {
        add_cpp_generated_template_file(
            result, options, templates, "api/api_application.hpp", GeneratedArtifactTier::Api,
            diagnostics, cpp_api_runtime_values(system)
        );
        add_cpp_generated_template_file(
            result, options, templates, "api/api_process.hpp", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_cpp_generated_template_file(
            result, options, templates, "api/api_dispatcher.hpp", GeneratedArtifactTier::Api,
            diagnostics,
            TemplateRenderer::Values{
                {"api_operation_dispatch_cases", generate_api_operation_dispatch_cases(system)}
            }
        );
        add_cpp_generated_template_file(
            result, options, templates, "api/api_server.hpp", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_cpp_generated_template_file(
            result, options, templates, "api/api_transport.hpp", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_cpp_generated_template_file(
            result, options, templates, "api/api_routes.hpp", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_cpp_generated_template_file(
            result, options, templates, "api/main.cpp", GeneratedArtifactTier::Api, diagnostics,
            cpp_api_main_values(system)
        );
    }
}

void add_cpp_worker_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto usage = runtime_domain_usage(system);
    const auto include_worker_composition = !system.workers.empty();
    const auto include_worker_execution = include_worker_composition || usage.uses_workflows;
    if (!include_worker_execution)
    {
        return;
    }

    add_cpp_generated_template_file(
        result, options, templates, "worker/worker_descriptors.hpp", GeneratedArtifactTier::Worker,
        diagnostics
    );
    add_cpp_generated_template_file(
        result, options, templates, "worker/worker_contexts.hpp", GeneratedArtifactTier::Worker,
        diagnostics
    );
    add_cpp_raw_worker_file(
        result, options, "worker/descriptors/catalog.hpp",
        cpp_worker_descriptor_catalog_file(system)
    );
    for (const auto& worker : system.workers)
    {
        add_cpp_raw_worker_file(
            result, options, "worker/descriptors/" + snake_identifier(worker.name) + ".hpp",
            std::string{
                "#pragma once\n\n#include \"../../common/descriptors/types.hpp\"\n\nnamespace "
                "statespec_generated::worker::descriptors\n{\n\n"
            } + generate_cpp_worker_descriptor_module(worker) +
                "} // namespace statespec_generated::worker::descriptors\n"
        );
    }
    add_cpp_raw_worker_file(
        result, options, "worker/worker_registry.hpp", cpp_worker_registry_facade(system)
    );
    for (const auto& worker : system.workers)
    {
        add_cpp_raw_worker_file(
            result, options, "worker/registry/" + snake_identifier(worker.name) + ".hpp",
            cpp_worker_registry_module(worker)
        );
    }
    if (include_worker_composition)
    {
        add_cpp_generated_template_file(
            result, options, templates, "worker/worker_application.hpp",
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_cpp_generated_template_file(
            result, options, templates, "worker/worker_process.hpp", GeneratedArtifactTier::Worker,
            diagnostics
        );
        add_cpp_generated_template_file(
            result, options, templates, "worker/worker_runtime.hpp", GeneratedArtifactTier::Worker,
            diagnostics, cpp_runtime_bootstrap_values(system)
        );
        add_cpp_generated_template_file(
            result, options, templates, "worker/main.cpp", GeneratedArtifactTier::Worker,
            diagnostics, cpp_worker_main_values(system)
        );
    }
    if (include_worker_execution)
    {
        for (const auto& workflow : system.workflows)
        {
            add_cpp_raw_worker_file(
                result, options, "worker/workflows/" + snake_identifier(workflow.name) + ".hpp",
                generate_cpp_workflow_step_module(workflow)
            );
        }
        add_cpp_generated_template_file(
            result, options, templates, "worker/workflow_step_handlers.hpp",
            GeneratedArtifactTier::Worker, diagnostics,
            TemplateRenderer::Values{
                {"workflow_step_handler_methods", generate_workflow_step_handler_methods(system)},
                {"default_workflow_step_handler_methods",
                 generate_default_workflow_step_handler_methods(system)},
                {"workflow_step_handler_keys", generate_workflow_step_handler_keys(system)}
            }
        );
        add_cpp_generated_template_file(
            result, options, templates, "worker/workflow_runner.hpp", GeneratedArtifactTier::Worker,
            diagnostics, cpp_workflow_runner_values(system)
        );
    }
    if (usage.uses_queues)
    {
        add_cpp_generated_template_file(
            result, options, templates, "worker/worker_queues.hpp", GeneratedArtifactTier::Worker,
            diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_cpp_generated_template_file(
            result, options, templates, "worker/worker_leases.hpp", GeneratedArtifactTier::Worker,
            diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_cpp_generated_template_file(
            result, options, templates, "worker/worker_workflows.hpp",
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
}

} // namespace statespec
