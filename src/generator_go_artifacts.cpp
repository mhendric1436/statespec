#include "generator_go_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_go_descriptors.hpp"
#include "generator_support.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{

namespace
{

TemplateRenderer::Values go_makefile_values(
    BindingGenerationTier tier,
    const IrSystem& system
)
{
    const auto include_api =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api;
    const auto include_worker =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker;
    const auto include_api_composition = include_api && !system.api_servers.empty();
    const auto include_worker_composition = include_worker && !system.workers.empty();

    std::ostringstream target_additions;
    std::ostringstream phony_targets;
    std::ostringstream api_package_additions;
    std::ostringstream worker_package_additions;
    std::ostringstream api_rules;
    std::ostringstream worker_rules;
    if (include_api_composition)
    {
        api_package_additions << " ./api/cmd/api";
    }
    if (include_worker_composition)
    {
        worker_package_additions << " ./worker/cmd/worker";
    }
    if (include_api)
    {
        target_additions << "\nCHECK_TARGETS += check-api";
        target_additions << "\nBUILD_TARGETS += build-api";
        target_additions << "\nPACKAGE_TARGETS += package-api";
        phony_targets << " check-api build-api package-api";
        api_rules << "check-api:\n";
        api_rules << "\t$(GO) test $(API_PACKAGES)\n\n";
        api_rules << "build-api:\n";
        api_rules << "\t$(GO) test $(API_PACKAGES)\n\n";
        api_rules << "package-api: build-api $(DIST_DIR)\n";
        api_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-api-go.tgz common api go.mod "
                     "Makefile\n\n";
    }
    if (include_worker)
    {
        target_additions << "\nCHECK_TARGETS += check-worker";
        target_additions << "\nBUILD_TARGETS += build-worker";
        target_additions << "\nPACKAGE_TARGETS += package-worker";
        phony_targets << " check-worker build-worker package-worker";
        worker_rules << "check-worker:\n";
        worker_rules << "\t$(GO) test $(WORKER_PACKAGES)\n\n";
        worker_rules << "build-worker:\n";
        worker_rules << "\t$(GO) test $(WORKER_PACKAGES)\n\n";
        worker_rules << "package-worker: build-worker $(DIST_DIR)\n";
        worker_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-worker-go.tgz common worker "
                        "go.mod Makefile\n\n";
    }
    return TemplateRenderer::Values{
        {"target_additions", target_additions.str()},
        {"phony_targets", phony_targets.str()},
        {"api_package_additions", api_package_additions.str()},
        {"worker_package_additions", worker_package_additions.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
    };
}

TemplateRenderer::Values go_runtime_bootstrap_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream runtime_import;
    std::ostringstream fields;
    std::ostringstream initializers;
    std::ostringstream arguments;
    std::ostringstream worker_run_once;
    std::ostringstream worker_time_import;

    auto add = [&](bool used, std::string_view field, std::string_view type, std::string_view ctor)
    {
        if (!used)
        {
            return;
        }
        fields << "\t" << field << " *runtime." << type << "\n";
        initializers << "\t\t" << field << ": runtime." << ctor << "(),\n";
        arguments << ", app." << field;
    };
    if (usage.uses_any_runtime_domain)
    {
        runtime_import << "\truntime \"statespec-generated/common/backend/runtime\"\n";
    }
    add(usage.uses_feature_flags, "FeatureFlags", "FeatureFlagStore", "NewFeatureFlagStore");
    add(usage.uses_queues, "Queues", "QueueStore", "NewQueueStore");
    add(usage.uses_leases, "Leases", "LeaseStore", "NewLeaseStore");
    add(usage.uses_workflows, "Workflows", "WorkflowStore", "NewWorkflowStore");
    add(usage.uses_logs, "Logs", "LogSink", "NewLogSink");
    add(usage.uses_metrics, "Metrics", "MetricSink", "NewMetricSink");
    if (usage.uses_workflows)
    {
        worker_time_import << "\t\"time\"\n";
        worker_run_once
            << "func (runtime *WorkerTierRuntime) RunOnce(ctx context.Context, workerContext "
               "common.WorkerContext, handler WorkflowStepHandler, workflowExecutionID string) "
               "(*common.WorkflowExecutionRecord, error) {\n"
            << "\tif workerContext.Executes == nil {\n"
            << "\t\treturn nil, nil\n"
            << "\t}\n"
            << "\trunner := WorkflowRunner{\n"
            << "\t\tBackend:       runtime.Backend,\n"
            << "\t\tWorkflowStore: runtime.Workflows,\n"
            << "\t\tHandler:       handler,\n"
            << "\t\tWorkerName:    workerContext.WorkerName,\n"
            << "\t\tLeaseDuration: 30 * time.Second,\n"
            << "\t\tMaxAttempts:   3,\n"
            << "\t}\n"
            << "\treturn runner.RunOnce(ctx, workflowExecutionID, *workerContext.Executes, 1)\n"
            << "}\n";
    }
    return TemplateRenderer::Values{
        {"runtime_store_import", runtime_import.str()},
        {"runtime_store_fields", fields.str()},
        {"runtime_store_initializers", initializers.str()},
        {"runtime_bootstrap_arguments", arguments.str()},
        {"worker_runtime_time_import", worker_time_import.str()},
        {"worker_runtime_run_once", worker_run_once.str()},
    };
}

TemplateRenderer::Values go_worker_runtime_bootstrap_values(const IrSystem& system)
{
    auto values = go_runtime_bootstrap_values(system);
    auto& arguments = values["runtime_bootstrap_arguments"];
    std::size_t pos = 0;
    while ((pos = arguments.find("app.", pos)) != std::string::npos)
    {
        arguments.replace(pos, 4, "runtime.");
        pos += 8;
    }
    return values;
}

TemplateRenderer::Values go_api_runtime_bootstrap_values(const IrSystem& system)
{
    auto values = go_runtime_bootstrap_values(system);
    values.erase("worker_runtime_time_import");
    values.erase("worker_runtime_run_once");
    return values;
}

void add_go_common_generated_template_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    std::string_view relative_output_path,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {},
    const std::filesystem::path& relative_artifact_path = {}
)
{
    add_generated_template_file(
        result, options.output_dir, templates, template_path_for_output(relative_output_path),
        common_artifact_path(relative_output_path), diagnostics, GeneratedArtifactTier::Common,
        values,
        relative_artifact_path.empty() ? common_artifact_path(relative_output_path)
                                       : relative_artifact_path
    );
}

void add_go_generated_template_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    std::string_view relative_output_path,
    GeneratedArtifactTier tier,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {},
    const std::filesystem::path& relative_artifact_path = {}
)
{
    add_generated_template_file(
        result, options.output_dir, templates, template_path_for_output(relative_output_path),
        artifact_path(relative_output_path), diagnostics, tier, values, relative_artifact_path
    );
}

} // namespace

void add_go_common_runtime_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto usage = runtime_domain_usage(system);

    add_template_file(
        result, options.output_dir, templates, "backend/json.go", "backend/json.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "backend/backend.go", "backend/backend.go",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "backend/external_system.go",
        "backend/external_system.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "backend/feature_flag.go", "backend/feature_flag.go",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "backend/lease.go", "backend/lease.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "backend/log.go", "backend/log.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "backend/metric.go", "backend/metric.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "backend/queue.go", "backend/queue.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "backend/schema_compatibility.go",
        "backend/schema_compatibility.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "backend/workflow.go", "backend/workflow.go",
        diagnostics
    );

    if (diagnostics.has_errors())
    {
        return;
    }

    add_go_common_generated_template_file(
        result, options, templates, "backend/memory/backend.go", diagnostics
    );
    add_go_common_generated_template_file(
        result, options, templates, "backend/memory/transaction.go", diagnostics
    );
    if (usage.uses_any_runtime_domain)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec.go", diagnostics
        );
    }
    if (usage.uses_feature_flags)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_feature_flags.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/feature_flags.go", diagnostics
        );
    }
    if (usage.uses_queues)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_queues.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/queues.go", diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_leases.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/leases.go", diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_workflows.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/workflows.go", diagnostics
        );
    }
    if (usage.uses_observability)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_observability.go", diagnostics
        );
    }
    if (usage.uses_logs)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_logs.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/logs.go", diagnostics
        );
    }
    if (usage.uses_metrics)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_metrics.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/metrics.go", diagnostics
        );
    }
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("descriptors.go.tmpl"),
        common_artifact_path("backend/descriptors.go"), diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{{"descriptors", generate_descriptors_go(system, templates)}}
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("go.mod.tmpl"),
        artifact_path(GeneratedArtifactGoMod), diagnostics, GeneratedArtifactTier::Common, {},
        common_artifact_path(GeneratedArtifactGoMod)
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Makefile.tmpl"),
        artifact_path(GeneratedArtifactMakefile), diagnostics, GeneratedArtifactTier::Common,
        go_makefile_values(options.tier, system), common_artifact_path(GeneratedArtifactMakefile)
    );
}

void add_go_api_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto include_api_composition = !system.api_servers.empty();

    add_go_generated_template_file(
        result, options, templates, "api/backend/api_descriptors.go", GeneratedArtifactTier::Api,
        diagnostics
    );
    add_go_generated_template_file(
        result, options, templates, "api/backend/api_codecs.go", GeneratedArtifactTier::Api,
        diagnostics, TemplateRenderer::Values{{"api_codecs", generate_api_codecs_go(system)}}
    );
    add_go_generated_template_file(
        result, options, templates, "api/backend/api_handlers.go", GeneratedArtifactTier::Api,
        diagnostics,
        TemplateRenderer::Values{
            {"api_operation_handler_methods", generate_api_operation_handler_methods_go(system)}
        }
    );
    add_go_generated_template_file(
        result, options, templates, "api/backend/api_handler_registry.go",
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_operation_default_handler_methods",
             generate_api_operation_default_handler_methods_go(system)}
        }
    );
    add_go_generated_template_file(
        result, options, templates, "api/backend/external_system_operator_metadata_api.go",
        GeneratedArtifactTier::Api, diagnostics
    );
    if (include_api_composition)
    {
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_application.go",
            GeneratedArtifactTier::Api, diagnostics, go_api_runtime_bootstrap_values(system)
        );
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_process.go", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_transport.go", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_dispatcher.go", GeneratedArtifactTier::Api,
            diagnostics,
            TemplateRenderer::Values{
                {"api_operation_dispatch_cases", generate_api_operation_dispatch_cases_go(system)}
            }
        );
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_server.go", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_routes.go", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "api/cmd/api/main.go", GeneratedArtifactTier::Api,
            diagnostics
        );
    }
}

void add_go_worker_artifacts(
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

    add_go_generated_template_file(
        result, options, templates, "worker/backend/worker_descriptors.go",
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_go_generated_template_file(
        result, options, templates, "worker/backend/worker_contexts.go",
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_go_generated_template_file(
        result, options, templates, "worker/backend/worker_registry.go",
        GeneratedArtifactTier::Worker, diagnostics
    );
    if (include_worker_composition)
    {
        add_go_generated_template_file(
            result, options, templates, "worker/backend/worker_application.go",
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "worker/backend/worker_runtime.go",
            GeneratedArtifactTier::Worker, diagnostics, go_worker_runtime_bootstrap_values(system)
        );
        add_go_generated_template_file(
            result, options, templates, "worker/cmd/worker/main.go", GeneratedArtifactTier::Worker,
            diagnostics
        );
    }
    if (include_worker_execution)
    {
        add_go_generated_template_file(
            result, options, templates, "worker/backend/workflow_step_handlers.go",
            GeneratedArtifactTier::Worker, diagnostics,
            TemplateRenderer::Values{
                {"workflow_step_handler_methods",
                 generate_workflow_step_handler_methods_go(system)},
                {"workflow_step_handler_keys", generate_workflow_step_handler_keys_go(system)}
            }
        );
        add_go_generated_template_file(
            result, options, templates, "worker/backend/workflow_runner.go",
            GeneratedArtifactTier::Worker, diagnostics,
            TemplateRenderer::Values{
                {"workflow_step_dispatch_cases", generate_workflow_step_dispatch_cases_go(system)},
                {"workflow_step_next_cases", generate_workflow_step_next_cases_go(system)}
            }
        );
    }
    if (usage.uses_queues)
    {
        add_go_generated_template_file(
            result, options, templates, "worker/backend/worker_queues.go",
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_go_generated_template_file(
            result, options, templates, "worker/backend/worker_leases.go",
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_go_generated_template_file(
            result, options, templates, "worker/backend/worker_workflows.go",
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
}

} // namespace statespec
