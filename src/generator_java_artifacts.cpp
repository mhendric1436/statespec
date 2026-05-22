#include "generator_java_artifacts.hpp"

#include "generator_java_descriptors.hpp"
#include "generator_support.hpp"

#include <filesystem>

namespace statespec
{

void add_java_common_runtime_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
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
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "Codec.java",
        output_root / "runtime" / "Codec.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "FeatureFlagCodec.java",
        output_root / "runtime" / "FeatureFlagCodec.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "QueueCodec.java",
        output_root / "runtime" / "QueueCodec.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "LeaseCodec.java",
        output_root / "runtime" / "LeaseCodec.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "WorkflowCodec.java",
        output_root / "runtime" / "WorkflowCodec.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "ObservabilityCodec.java",
        output_root / "runtime" / "ObservabilityCodec.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "LogCodec.java",
        output_root / "runtime" / "LogCodec.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "MetricCodec.java",
        output_root / "runtime" / "MetricCodec.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "FeatureFlagStore.java",
        output_root / "runtime" / "FeatureFlagStore.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "QueueStore.java",
        output_root / "runtime" / "QueueStore.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "LeaseStore.java",
        output_root / "runtime" / "LeaseStore.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "WorkflowStore.java",
        output_root / "runtime" / "WorkflowStore.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "LogSink.java",
        output_root / "runtime" / "LogSink.java", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, output_root / "runtime" / "MetricSink.java",
        output_root / "runtime" / "MetricSink.java", diagnostics
    );
    add_generated_template_file(
        result, options.output_dir, templates, "generated/Descriptors.java.tmpl",
        "common/com/statespec/generated/Descriptors.java", diagnostics,
        GeneratedArtifactTier::Common,
        TemplateRenderer::Values{{"descriptors", generate_descriptors_java(system)}}
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
    add_generated_template_file(
        result, options.output_dir, templates,
        "api/com/statespec/generated/ApiDescriptors.java.tmpl",
        "api/com/statespec/generated/ApiDescriptors.java", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "api/com/statespec/generated/ApiApplication.java.tmpl",
        "api/com/statespec/generated/ApiApplication.java", diagnostics, GeneratedArtifactTier::Api
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
        "api/com/statespec/generated/ApiDispatcher.java.tmpl",
        "api/com/statespec/generated/ApiDispatcher.java", diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{
            {"api_operation_dispatch_cases", generate_api_operation_dispatch_cases_java(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/com/statespec/generated/ApiServer.java.tmpl",
        "api/com/statespec/generated/ApiServer.java", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/com/statespec/generated/ApiRoutes.java.tmpl",
        "api/com/statespec/generated/ApiRoutes.java", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java.tmpl",
        "api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java", diagnostics,
        GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/com/statespec/generated/ApiMain.java.tmpl",
        "api/com/statespec/generated/ApiMain.java", diagnostics, GeneratedArtifactTier::Api
    );
}

void add_java_worker_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
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
        GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "worker/com/statespec/generated/WorkflowStepHandlers.java.tmpl",
        "worker/com/statespec/generated/WorkflowStepHandlers.java", diagnostics,
        GeneratedArtifactTier::Worker,
        TemplateRenderer::Values{
            {"workflow_step_handler_keys", generate_workflow_step_handler_keys_java(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "worker/com/statespec/generated/WorkflowRunner.java.tmpl",
        "worker/com/statespec/generated/WorkflowRunner.java", diagnostics,
        GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "worker/com/statespec/generated/WorkerQueues.java.tmpl",
        "worker/com/statespec/generated/WorkerQueues.java", diagnostics,
        GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "worker/com/statespec/generated/WorkerLeases.java.tmpl",
        "worker/com/statespec/generated/WorkerLeases.java", diagnostics,
        GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "worker/com/statespec/generated/WorkerWorkflows.java.tmpl",
        "worker/com/statespec/generated/WorkerWorkflows.java", diagnostics,
        GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "worker/com/statespec/generated/WorkerMain.java.tmpl",
        "worker/com/statespec/generated/WorkerMain.java", diagnostics, GeneratedArtifactTier::Worker
    );
}

} // namespace statespec
