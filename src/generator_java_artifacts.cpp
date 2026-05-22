#include "generator_java_artifacts.hpp"

#include "generator_java_descriptors.hpp"
#include "generator_support.hpp"
#include "statespec/runtime_usage.hpp"

#include <filesystem>
#include <sstream>

namespace statespec
{
namespace
{

TemplateRenderer::Values java_runtime_bootstrap_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream imports;
    std::ostringstream fields;
    std::ostringstream initializers;
    std::ostringstream arguments;
    std::ostringstream worker_imports;
    std::ostringstream worker_run_once;

    auto add = [&](bool used, std::string_view type, std::string_view field)
    {
        if (!used)
        {
            return;
        }
        imports << "import com.statespec.backend.runtime." << type << ";\n";
        fields << "    private final " << type << " " << field << ";\n";
        initializers << "        this." << field << " = new " << type << "();\n";
        arguments << ", " << field;
    };
    add(usage.uses_feature_flags, "FeatureFlagStore", "featureFlags");
    add(usage.uses_queues, "QueueStore", "queues");
    add(usage.uses_leases, "LeaseStore", "leases");
    add(usage.uses_workflows, "WorkflowStore", "workflows");
    add(usage.uses_logs, "LogSink", "logs");
    add(usage.uses_metrics, "MetricSink", "metrics");
    if (usage.uses_workflows)
    {
        worker_imports << "import java.time.Duration;\n";
        worker_imports << "import java.util.Optional;\n";
        worker_run_once
            << "    public Optional<com.statespec.backend.Workflow.WorkflowExecutionRecord> "
               "runOnce(\n"
            << "        Descriptors.WorkerContext context,\n"
            << "        WorkflowStepHandlers.Handler handler,\n"
            << "        String workflowExecutionId\n"
            << "    ) throws Exception {\n"
            << "        if (context.executes().isEmpty()) {\n"
            << "            return Optional.empty();\n"
            << "        }\n"
            << "        WorkflowRunner runner =\n"
            << "            new WorkflowRunner(backend, workflows, handler, context.workerName(), "
               "Duration.ofSeconds(30), 3);\n"
            << "        return runner.runOnce(workflowExecutionId, "
               "context.executes().orElseThrow(), "
               "1);\n"
            << "    }\n";
    }
    return TemplateRenderer::Values{
        {"runtime_store_imports", imports.str()},
        {"runtime_store_fields", fields.str()},
        {"runtime_store_initializers", initializers.str()},
        {"runtime_bootstrap_arguments", arguments.str()},
        {"worker_runtime_imports", worker_imports.str()},
        {"worker_runtime_run_once", worker_run_once.str()},
    };
}

TemplateRenderer::Values java_api_runtime_bootstrap_values(const IrSystem& system)
{
    auto values = java_runtime_bootstrap_values(system);
    values.erase("worker_runtime_imports");
    values.erase("worker_runtime_run_once");
    return values;
}

} // namespace

void add_java_common_runtime_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto usage = runtime_domain_usage(system);
    const std::filesystem::path output_root{"com/statespec/backend"};

    add_template_file(
        result, options.output_dir, templates, output_root / "Json.java", output_root / "Json.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "Backend.java",
        output_root / "Backend.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "ExternalSystem.java",
        output_root / "ExternalSystem.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "FeatureFlag.java",
        output_root / "FeatureFlag.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "Lease.java",
        output_root / "Lease.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "Log.java", output_root / "Log.java",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "Metric.java",
        output_root / "Metric.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "Queue.java",
        output_root / "Queue.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "Workflow.java",
        output_root / "Workflow.java", diagnostics
    );

    if (diagnostics.has_errors())
    {
        return;
    }

    add_template_file(
        result, options.output_dir, templates, output_root / "memory" / "InMemoryBackend.java",
        output_root / "memory" / "InMemoryBackend.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "memory" / "InMemoryTransaction.java",
        output_root / "memory" / "InMemoryTransaction.java", diagnostics
    );
    if (usage.uses_any_runtime_domain)
    {
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "Codec.java",
            output_root / "runtime" / "Codec.java", diagnostics
        );
    }
    if (usage.uses_feature_flags)
    {
        add_template_file(
            result, options.output_dir, templates,
            output_root / "runtime" / "FeatureFlagCodec.java",
            output_root / "runtime" / "FeatureFlagCodec.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates,
            output_root / "runtime" / "FeatureFlagStore.java",
            output_root / "runtime" / "FeatureFlagStore.java", diagnostics
        );
    }
    if (usage.uses_queues)
    {
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "QueueCodec.java",
            output_root / "runtime" / "QueueCodec.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "QueueStore.java",
            output_root / "runtime" / "QueueStore.java", diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "LeaseCodec.java",
            output_root / "runtime" / "LeaseCodec.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "LeaseStore.java",
            output_root / "runtime" / "LeaseStore.java", diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "WorkflowCodec.java",
            output_root / "runtime" / "WorkflowCodec.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "WorkflowStore.java",
            output_root / "runtime" / "WorkflowStore.java", diagnostics
        );
    }
    if (usage.uses_observability)
    {
        add_template_file(
            result, options.output_dir, templates,
            output_root / "runtime" / "ObservabilityCodec.java",
            output_root / "runtime" / "ObservabilityCodec.java", diagnostics
        );
    }
    if (usage.uses_logs)
    {
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "LogCodec.java",
            output_root / "runtime" / "LogCodec.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "LogSink.java",
            output_root / "runtime" / "LogSink.java", diagnostics
        );
    }
    if (usage.uses_metrics)
    {
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "MetricCodec.java",
            output_root / "runtime" / "MetricCodec.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "MetricSink.java",
            output_root / "runtime" / "MetricSink.java", diagnostics
        );
    }
    add_generated_template_file(
        result, options.output_dir, templates, "generated/Descriptors.java.tmpl",
        "common/com/statespec/generated/Descriptors.java", diagnostics,
        GeneratedArtifactTier::Common,
        TemplateRenderer::Values{{"descriptors", generate_descriptors_java(system, templates)}}
    );
    add_generated_template_file(
        result, options.output_dir, templates, "generated/Makefile.tmpl", "Makefile", diagnostics,
        GeneratedArtifactTier::Common, {}, "common/Makefile"
    );
}

void add_java_api_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto include_api_composition = !system.api_servers.empty();

    add_generated_template_file(
        result, options.output_dir, templates,
        "api/com/statespec/generated/ApiDescriptors.java.tmpl",
        "api/com/statespec/generated/ApiDescriptors.java", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/com/statespec/generated/ApiCodecs.java.tmpl",
        "api/com/statespec/generated/ApiCodecs.java", diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{{"api_codecs", generate_api_codecs_java(system)}}
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/com/statespec/generated/ApiHandlers.java.tmpl",
        "api/com/statespec/generated/ApiHandlers.java", diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{
            {"api_operation_handler_methods", generate_api_operation_handler_methods_java(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "api/com/statespec/generated/ApiHandlerRegistry.java.tmpl",
        "api/com/statespec/generated/ApiHandlerRegistry.java", diagnostics,
        GeneratedArtifactTier::Api,
        TemplateRenderer::Values{
            {"api_operation_default_handler_methods",
             generate_api_operation_default_handler_methods_java(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java.tmpl",
        "api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java", diagnostics,
        GeneratedArtifactTier::Api
    );
    if (include_api_composition)
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            "api/com/statespec/generated/ApiApplication.java.tmpl",
            "api/com/statespec/generated/ApiApplication.java", diagnostics,
            GeneratedArtifactTier::Api, java_api_runtime_bootstrap_values(system)
        );
        add_generated_template_file(
            result, options.output_dir, templates,
            "api/com/statespec/generated/ApiDispatcher.java.tmpl",
            "api/com/statespec/generated/ApiDispatcher.java", diagnostics,
            GeneratedArtifactTier::Api,
            TemplateRenderer::Values{
                {"api_operation_dispatch_cases", generate_api_operation_dispatch_cases_java(system)}
            }
        );
        add_generated_template_file(
            result, options.output_dir, templates,
            "api/com/statespec/generated/ApiServer.java.tmpl",
            "api/com/statespec/generated/ApiServer.java", diagnostics, GeneratedArtifactTier::Api
        );
        add_generated_template_file(
            result, options.output_dir, templates,
            "api/com/statespec/generated/ApiRoutes.java.tmpl",
            "api/com/statespec/generated/ApiRoutes.java", diagnostics, GeneratedArtifactTier::Api
        );
        add_generated_template_file(
            result, options.output_dir, templates, "api/com/statespec/generated/ApiMain.java.tmpl",
            "api/com/statespec/generated/ApiMain.java", diagnostics, GeneratedArtifactTier::Api
        );
    }
}

void add_java_worker_artifacts(
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
        result, options.output_dir, templates,
        "worker/com/statespec/generated/WorkerDescriptors.java.tmpl",
        "worker/com/statespec/generated/WorkerDescriptors.java", diagnostics,
        GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "worker/com/statespec/generated/WorkerContexts.java.tmpl",
        "worker/com/statespec/generated/WorkerContexts.java", diagnostics,
        GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "worker/com/statespec/generated/WorkerRegistry.java.tmpl",
        "worker/com/statespec/generated/WorkerRegistry.java", diagnostics,
        GeneratedArtifactTier::Worker
    );
    if (include_worker_composition)
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            "worker/com/statespec/generated/WorkerApplication.java.tmpl",
            "worker/com/statespec/generated/WorkerApplication.java", diagnostics,
            GeneratedArtifactTier::Worker
        );
        add_generated_template_file(
            result, options.output_dir, templates,
            "worker/com/statespec/generated/WorkerRuntime.java.tmpl",
            "worker/com/statespec/generated/WorkerRuntime.java", diagnostics,
            GeneratedArtifactTier::Worker, java_runtime_bootstrap_values(system)
        );
        add_generated_template_file(
            result, options.output_dir, templates,
            "worker/com/statespec/generated/WorkerMain.java.tmpl",
            "worker/com/statespec/generated/WorkerMain.java", diagnostics,
            GeneratedArtifactTier::Worker
        );
    }
    if (include_worker_execution)
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            "worker/com/statespec/generated/WorkflowStepHandlers.java.tmpl",
            "worker/com/statespec/generated/WorkflowStepHandlers.java", diagnostics,
            GeneratedArtifactTier::Worker,
            TemplateRenderer::Values{
                {"workflow_step_handler_methods",
                 generate_workflow_step_handler_methods_java(system)},
                {"workflow_step_handler_keys", generate_workflow_step_handler_keys_java(system)}
            }
        );
        add_generated_template_file(
            result, options.output_dir, templates,
            "worker/com/statespec/generated/WorkflowRunner.java.tmpl",
            "worker/com/statespec/generated/WorkflowRunner.java", diagnostics,
            GeneratedArtifactTier::Worker,
            TemplateRenderer::Values{
                {"workflow_step_dispatch_cases",
                 generate_workflow_step_dispatch_cases_java(system)},
                {"workflow_step_next_cases", generate_workflow_step_next_cases_java(system)}
            }
        );
    }
    if (usage.uses_queues)
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            "worker/com/statespec/generated/WorkerQueues.java.tmpl",
            "worker/com/statespec/generated/WorkerQueues.java", diagnostics,
            GeneratedArtifactTier::Worker
        );
    }
    if (usage.uses_leases)
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            "worker/com/statespec/generated/WorkerLeases.java.tmpl",
            "worker/com/statespec/generated/WorkerLeases.java", diagnostics,
            GeneratedArtifactTier::Worker
        );
    }
    if (usage.uses_workflows)
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            "worker/com/statespec/generated/WorkerWorkflows.java.tmpl",
            "worker/com/statespec/generated/WorkerWorkflows.java", diagnostics,
            GeneratedArtifactTier::Worker
        );
    }
}

} // namespace statespec
