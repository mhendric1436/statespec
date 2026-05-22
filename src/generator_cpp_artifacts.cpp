#include "generator_cpp_artifacts.hpp"

#include "generator_cpp_descriptors.hpp"
#include "generator_support.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{

namespace
{

TemplateRenderer::Values cpp_common_runtime_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream includes;
    auto add = [&](std::string_view include) { includes << "#include \"" << include << "\"\\n"; };
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
    return TemplateRenderer::Values{{"common_runtime_includes", includes.str()}};
}

TemplateRenderer::Values cpp_makefile_values(
    BindingGenerationTier tier,
    const IrSystem& system
)
{
    const auto include_api =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api;
    const auto include_worker =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker;
    const auto include_api_composition = include_api && !system.api_servers.empty();
    const auto usage = runtime_domain_usage(system);
    const auto include_worker_composition = include_worker && !system.workers.empty();
    const auto include_worker_execution =
        include_worker && (include_worker_composition || usage.uses_workflows);

    std::ostringstream target_additions;
    std::ostringstream phony_targets;
    std::ostringstream api_rules;
    std::ostringstream worker_rules;
    if (include_api)
    {
        target_additions << "\nCHECK_TARGETS += check-api";
        target_additions << "\nBUILD_TARGETS += build-api";
        target_additions << "\nPACKAGE_TARGETS += package-api";
        phony_targets << " check-api build-api package-api";
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
                         "#include \"api/api_routes.hpp\"\\n"
                         "#include \"api/api_server.hpp\"\\n";
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
        worker_rules << "check-worker: $(BUILD_DIR)/.dir\n";
        worker_rules << "\tprintf '#include \"worker/worker_contexts.hpp\"\\n"
                        "#include \"worker/worker_descriptors.hpp\"\\n"
                        "#include \"worker/worker_registry.hpp\"\\n";
        if (include_worker_composition)
        {
            worker_rules << "#include \"worker/worker_application.hpp\"\\n"
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
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
        {"common_runtime_includes", cpp_common_runtime_values(system)["common_runtime_includes"]},
    };
}

TemplateRenderer::Values cpp_runtime_bootstrap_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream includes;
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

    return TemplateRenderer::Values{
        {"runtime_store_includes", includes.str()},
        {"runtime_store_initializers", initializers.str()},
        {"runtime_store_members", members.str()},
        {"runtime_bootstrap_arguments", arguments.str()},
        {"worker_runtime_run_once", worker_run_once.str()},
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

TemplateRenderer::Values cpp_api_runtime_values(const IrSystem& system)
{
    auto values = cpp_runtime_bootstrap_values(system);
    values.erase("worker_runtime_run_once");
    return values;
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
        add_generated_template_file(
            result, options.output_dir, templates, "runtime/codec.hpp.tmpl",
            "common/runtime/codec.hpp", diagnostics, GeneratedArtifactTier::Common,
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

    if (diagnostics.has_errors())
    {
        return;
    }

    add_generated_template_file(
        result, options.output_dir, templates, "generated/descriptors.hpp.tmpl",
        "common/descriptors.hpp", diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{
            {"system_descriptors", generate_system_descriptors_header(system, templates)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "generated/Makefile.tmpl", "Makefile", diagnostics,
        GeneratedArtifactTier::Common, cpp_makefile_values(options.tier, system), "common/Makefile"
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

    add_generated_template_file(
        result, options.output_dir, templates, "api/api_descriptors.hpp.tmpl",
        "api/api_descriptors.hpp", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/api_codecs.hpp.tmpl", "api/api_codecs.hpp",
        diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{{"api_codecs", generate_api_codecs(system)}}
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/api_handlers.hpp.tmpl", "api/api_handlers.hpp",
        diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{
            {"api_operation_handler_methods", generate_api_operation_handler_methods(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/api_handler_registry.hpp.tmpl",
        "api/api_handler_registry.hpp", diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{
            {"api_operation_default_handler_methods",
             generate_api_operation_default_handler_methods(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/external_system_operator_metadata_api.hpp.tmpl",
        "api/external_system_operator_metadata_api.hpp", diagnostics, GeneratedArtifactTier::Api
    );
    if (include_api_composition)
    {
        add_generated_template_file(
            result, options.output_dir, templates, "api/api_application.hpp.tmpl",
            "api/api_application.hpp", diagnostics, GeneratedArtifactTier::Api,
            cpp_api_runtime_values(system)
        );
        add_generated_template_file(
            result, options.output_dir, templates, "api/api_dispatcher.hpp.tmpl",
            "api/api_dispatcher.hpp", diagnostics, GeneratedArtifactTier::Api,
            TemplateRenderer::Values{
                {"api_operation_dispatch_cases", generate_api_operation_dispatch_cases(system)}
            }
        );
        add_generated_template_file(
            result, options.output_dir, templates, "api/api_server.hpp.tmpl", "api/api_server.hpp",
            diagnostics, GeneratedArtifactTier::Api
        );
        add_generated_template_file(
            result, options.output_dir, templates, "api/api_routes.hpp.tmpl", "api/api_routes.hpp",
            diagnostics, GeneratedArtifactTier::Api
        );
        add_generated_template_file(
            result, options.output_dir, templates, "api/main.cpp.tmpl", "api/main.cpp", diagnostics,
            GeneratedArtifactTier::Api
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

    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_descriptors.hpp.tmpl",
        "worker/worker_descriptors.hpp", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_contexts.hpp.tmpl",
        "worker/worker_contexts.hpp", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_registry.hpp.tmpl",
        "worker/worker_registry.hpp", diagnostics, GeneratedArtifactTier::Worker
    );
    if (include_worker_composition)
    {
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_application.hpp.tmpl",
            "worker/worker_application.hpp", diagnostics, GeneratedArtifactTier::Worker
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_runtime.hpp.tmpl",
            "worker/worker_runtime.hpp", diagnostics, GeneratedArtifactTier::Worker,
            cpp_runtime_bootstrap_values(system)
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/main.cpp.tmpl", "worker/main.cpp",
            diagnostics, GeneratedArtifactTier::Worker
        );
    }
    if (include_worker_execution)
    {
        add_generated_template_file(
            result, options.output_dir, templates, "worker/workflow_step_handlers.hpp.tmpl",
            "worker/workflow_step_handlers.hpp", diagnostics, GeneratedArtifactTier::Worker,
            TemplateRenderer::Values{
                {"workflow_step_handler_methods", generate_workflow_step_handler_methods(system)},
                {"workflow_step_handler_keys", generate_workflow_step_handler_keys(system)}
            }
        );
        add_generated_template_file(
            result, options.output_dir, templates, "worker/workflow_runner.hpp.tmpl",
            "worker/workflow_runner.hpp", diagnostics, GeneratedArtifactTier::Worker,
            TemplateRenderer::Values{
                {"workflow_step_dispatch_cases", generate_workflow_step_dispatch_cases(system)},
                {"workflow_step_next_cases", generate_workflow_step_next_cases(system)}
            }
        );
    }
    if (usage.uses_queues)
    {
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_queues.hpp.tmpl",
            "worker/worker_queues.hpp", diagnostics, GeneratedArtifactTier::Worker
        );
    }
    if (usage.uses_leases)
    {
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_leases.hpp.tmpl",
            "worker/worker_leases.hpp", diagnostics, GeneratedArtifactTier::Worker
        );
    }
    if (usage.uses_workflows)
    {
        add_generated_template_file(
            result, options.output_dir, templates, "worker/worker_workflows.hpp.tmpl",
            "worker/worker_workflows.hpp", diagnostics, GeneratedArtifactTier::Worker
        );
    }
}

} // namespace statespec
