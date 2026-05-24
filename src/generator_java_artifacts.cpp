#include "generator_java_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_java_descriptor_areas.hpp"
#include "generator_java_descriptor_support.hpp"
#include "generator_java_descriptors.hpp"
#include "generator_support.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <filesystem>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace statespec
{
namespace
{

std::string java_makefile_source_list(const std::vector<std::string>& sources)
{
    if (sources.empty())
    {
        return {};
    }

    std::ostringstream out;
    for (const auto& source : sources)
    {
        out << " \\\n  " << source;
    }
    return out.str();
}

std::string java_entity_descriptor_module_class_name(std::string_view entity_name)
{
    return pascal_identifier(std::string{entity_name}) + "DescriptorModule";
}

std::string java_entity_descriptor_module_ref(std::string_view entity_name)
{
    return "com.statespec.generated.descriptors.entities." +
           java_entity_descriptor_module_class_name(entity_name);
}

std::vector<std::string> java_descriptor_module_sources(const IrSystem& system)
{
    std::vector<std::string> sources{
        "common/com/statespec/generated/descriptors/CoreDescriptorModule.java",
        "common/com/statespec/generated/descriptors/ShapeDescriptorModule.java",
        "common/com/statespec/generated/descriptors/ApiDescriptorModule.java",
        "common/com/statespec/generated/descriptors/WorkerDescriptorModule.java",
        "common/com/statespec/generated/descriptors/RuntimeDescriptorModule.java",
    };
    for (const auto& entity : system.entities)
    {
        sources.push_back(
            "common/com/statespec/generated/descriptors/entities/" +
            java_entity_descriptor_module_class_name(entity.name) + ".java"
        );
    }
    return sources;
}

TemplateRenderer::Values java_makefile_values(
    BindingGenerationTier tier,
    const IrSystem& system
)
{
    const auto usage = runtime_domain_usage(system);
    const auto include_api =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api;
    const auto include_worker =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker;
    const auto include_api_composition = include_api && !system.api_servers.empty();
    const auto include_worker_composition = include_worker && !system.workers.empty();
    const auto include_worker_execution =
        include_worker && (include_worker_composition || usage.uses_workflows);

    std::vector<std::string> common_sources{
        "common/com/statespec/backend/Json.java",
        "common/com/statespec/backend/Backend.java",
        "common/com/statespec/backend/ExternalSystem.java",
        "common/com/statespec/backend/FeatureFlag.java",
        "common/com/statespec/backend/Lease.java",
        "common/com/statespec/backend/Log.java",
        "common/com/statespec/backend/Metric.java",
        "common/com/statespec/backend/Queue.java",
        "common/com/statespec/backend/SchemaCompatibility.java",
        "common/com/statespec/backend/Workflow.java",
        "common/com/statespec/backend/memory/InMemoryBackend.java",
        "common/com/statespec/backend/memory/InMemoryTransaction.java",
        "common/com/statespec/generated/Descriptors.java",
    };
    for (const auto& source : java_descriptor_module_sources(system))
    {
        common_sources.push_back(source);
    }
    auto add_common = [&](bool used, std::string source)
    {
        if (used)
        {
            common_sources.push_back(std::move(source));
        }
    };
    add_common(usage.uses_any_runtime_domain, "common/com/statespec/backend/runtime/Codec.java");
    add_common(
        usage.uses_feature_flags, "common/com/statespec/backend/runtime/FeatureFlagCodec.java"
    );
    add_common(
        usage.uses_feature_flags, "common/com/statespec/backend/runtime/FeatureFlagStore.java"
    );
    add_common(usage.uses_queues, "common/com/statespec/backend/runtime/QueueCodec.java");
    add_common(usage.uses_queues, "common/com/statespec/backend/runtime/QueueStore.java");
    add_common(usage.uses_leases, "common/com/statespec/backend/runtime/LeaseCodec.java");
    add_common(usage.uses_leases, "common/com/statespec/backend/runtime/LeaseStore.java");
    add_common(usage.uses_workflows, "common/com/statespec/backend/runtime/WorkflowCodec.java");
    add_common(usage.uses_workflows, "common/com/statespec/backend/runtime/WorkflowStore.java");
    add_common(
        usage.uses_observability, "common/com/statespec/backend/runtime/ObservabilityCodec.java"
    );
    add_common(usage.uses_logs, "common/com/statespec/backend/runtime/LogCodec.java");
    add_common(usage.uses_logs, "common/com/statespec/backend/runtime/LogSink.java");
    add_common(usage.uses_metrics, "common/com/statespec/backend/runtime/MetricCodec.java");
    add_common(usage.uses_metrics, "common/com/statespec/backend/runtime/MetricSink.java");
    add_common(
        usage.uses_entity_gc, "common/com/statespec/backend/runtime/EntityGcDescriptors.java"
    );
    add_common(
        usage.uses_entity_gc, "common/com/statespec/backend/runtime/EntityGcRepository.java"
    );
    add_common(usage.uses_entity_gc, "common/com/statespec/backend/runtime/EntityGcWorkers.java");

    std::vector<std::string> api_sources;
    if (include_api)
    {
        api_sources = {
            "api/com/statespec/generated/ApiDescriptors.java",
            "api/com/statespec/generated/ApiCodecs.java",
            "api/com/statespec/generated/ApiHandlers.java",
            "api/com/statespec/generated/ApiHandlerRegistry.java",
            "api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java",
        };
        if (include_api_composition)
        {
            api_sources.push_back("api/com/statespec/generated/ApiApplication.java");
            api_sources.push_back("api/com/statespec/generated/ApiProcess.java");
            api_sources.push_back("api/com/statespec/generated/ApiDispatcher.java");
            api_sources.push_back("api/com/statespec/generated/ApiServer.java");
            api_sources.push_back("api/com/statespec/generated/ApiTransport.java");
            api_sources.push_back("api/com/statespec/generated/ApiRoutes.java");
            api_sources.push_back("api/com/statespec/generated/ApiMain.java");
        }
    }

    std::vector<std::string> worker_sources;
    if (include_worker)
    {
        worker_sources = {
            "worker/com/statespec/generated/WorkerDescriptors.java",
            "worker/com/statespec/generated/WorkerContexts.java",
            "worker/com/statespec/generated/WorkerRegistry.java",
        };
        if (include_worker_composition)
        {
            worker_sources.push_back("worker/com/statespec/generated/WorkerApplication.java");
            worker_sources.push_back("worker/com/statespec/generated/WorkerProcess.java");
            worker_sources.push_back("worker/com/statespec/generated/WorkerRuntime.java");
            worker_sources.push_back("worker/com/statespec/generated/WorkerMain.java");
        }
        if (include_worker_execution)
        {
            worker_sources.push_back("worker/com/statespec/generated/WorkflowStepHandlers.java");
            worker_sources.push_back("worker/com/statespec/generated/WorkflowRunner.java");
        }
        if (usage.uses_queues)
        {
            worker_sources.push_back("worker/com/statespec/generated/WorkerQueues.java");
        }
        if (usage.uses_leases)
        {
            worker_sources.push_back("worker/com/statespec/generated/WorkerLeases.java");
        }
        if (usage.uses_workflows)
        {
            worker_sources.push_back("worker/com/statespec/generated/WorkerWorkflows.java");
        }
    }

    std::ostringstream build_target_additions;
    std::ostringstream package_target_additions;
    std::ostringstream phony_targets;
    std::ostringstream api_rules;
    std::ostringstream worker_rules;
    if (include_api)
    {
        build_target_additions << " build-api";
        package_target_additions << " package-api";
        phony_targets << " build-api package-api";
        api_rules << "build-api: $(BUILD_DIR)\n";
        api_rules << "\t$(JAVAC) -d $(BUILD_DIR) $(COMMON_SOURCES) "
                     "$(COMMON_EXTENSION_SOURCES) $(API_SOURCES) $(API_EXTENSION_SOURCES)\n\n";
        api_rules << "package-api: build-api $(DIST_DIR)\n";
        api_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-api-java.tgz common api "
                     "Makefile\n\n";
    }
    if (include_worker)
    {
        build_target_additions << " build-worker";
        package_target_additions << " package-worker";
        phony_targets << " build-worker package-worker";
        worker_rules << "build-worker: $(BUILD_DIR)\n";
        worker_rules << "\t$(JAVAC) -d $(BUILD_DIR) $(COMMON_SOURCES) "
                        "$(COMMON_EXTENSION_SOURCES) $(WORKER_SOURCES) "
                        "$(WORKER_EXTENSION_SOURCES)\n\n";
        worker_rules << "package-worker: build-worker $(DIST_DIR)\n";
        worker_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-worker-java.tgz common worker "
                        "Makefile\n\n";
    }

    return TemplateRenderer::Values{
        {"common_sources", java_makefile_source_list(common_sources)},
        {"api_sources", java_makefile_source_list(api_sources)},
        {"worker_sources", java_makefile_source_list(worker_sources)},
        {"build_target_additions", build_target_additions.str()},
        {"package_target_additions", package_target_additions.str()},
        {"phony_targets", phony_targets.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
    };
}

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
    else
    {
        worker_imports << "import java.util.Optional;\n";
        worker_run_once
            << "    public Optional<com.statespec.backend.Workflow.WorkflowExecutionRecord> "
               "runOnce(\n"
            << "        Descriptors.WorkerContext context,\n"
            << "        WorkflowStepHandlers.Handler handler,\n"
            << "        String workflowExecutionId\n"
            << "    ) {\n"
            << "        return Optional.empty();\n"
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

TemplateRenderer::Values java_entity_gc_descriptor_values(const IrSystem& system)
{
    std::vector<std::string> descriptor_entries;
    std::ostringstream descriptors;
    for (const auto& entity : system.entities)
    {
        std::ostringstream terminal_states;
        bool first_terminal_state = true;
        for (const auto& state : entity.states)
        {
            if (!state.garbage_collection.has_value())
            {
                continue;
            }
            if (!first_terminal_state)
            {
                terminal_states << ",\n";
            }
            const auto module_ref = java_entity_descriptor_module_ref(entity.name);
            terminal_states << "                new TerminalState(" << module_ref << "."
                            << java_entity_state_constant_name(entity.name, state.name) << ", "
                            << java_string(state.garbage_collection->after) << ", "
                            << java_string(state.garbage_collection->mode) << ")";
            first_terminal_state = false;
        }
        if (terminal_states.str().empty())
        {
            continue;
        }
        std::ostringstream descriptor;
        descriptor << "            new Descriptor(\n"
                   << "                " << java_entity_descriptor_module_ref(entity.name) << "."
                   << java_entity_name_constant_name(entity.name) << ",\n"
                   << "                " << java_entity_descriptor_module_ref(entity.name) << "."
                   << java_entity_name_constant_name(entity.name) << ",\n"
                   << "                List.of(\n"
                   << terminal_states.str() << "\n"
                   << "                )\n"
                   << "            )";
        descriptor_entries.push_back(descriptor.str());
    }
    for (std::size_t i = 0; i < descriptor_entries.size(); ++i)
    {
        descriptors << descriptor_entries[i];
        if (i + 1 < descriptor_entries.size())
        {
            descriptors << ",";
        }
        descriptors << "\n";
    }
    return TemplateRenderer::Values{{"entity_gc_descriptors", descriptors.str()}};
}

std::filesystem::path java_api_generated_path(std::string_view filename)
{
    return api_artifact_path(
        join_artifact_path(GeneratedJavaOutputPackagePath, filename).generic_string()
    );
}

std::filesystem::path java_worker_generated_path(std::string_view filename)
{
    return worker_artifact_path(
        join_artifact_path(GeneratedJavaOutputPackagePath, filename).generic_string()
    );
}

void add_java_generated_template_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const std::filesystem::path& relative_output_path,
    GeneratedArtifactTier tier,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {}
)
{
    add_generated_template_file(
        result, options.output_dir, templates, template_path_for_output(relative_output_path),
        relative_output_path, diagnostics, tier, values
    );
}

TemplateRenderer::Values java_descriptor_module_values(
    std::string_view package_name,
    std::string_view class_name,
    std::string_view module_name,
    std::string content = {}
)
{
    return TemplateRenderer::Values{
        {"descriptor_module_package", std::string{package_name}},
        {"descriptor_module_class", std::string{class_name}},
        {"descriptor_module_name", std::string{module_name}},
        {"descriptor_module_content", std::move(content)},
    };
}

TemplateRenderer::Values java_entity_descriptor_module_values(const IrEntity& entity)
{
    IrSystem one_entity_system;
    one_entity_system.entities.push_back(entity);
    auto content = generate_java_entity_descriptors(one_entity_system);
    return java_descriptor_module_values(
        "com.statespec.generated.descriptors.entities",
        java_entity_descriptor_module_class_name(entity.name), "entity descriptor " + entity.name,
        std::move(content)
    );
}

TemplateRenderer::Values java_shape_descriptor_module_values(
    std::string_view package_name,
    std::string_view class_name,
    const IrSystem& system
)
{
    return java_descriptor_module_values(
        package_name, class_name, "shape descriptors", generate_java_shape_descriptors(system)
    );
}

void add_java_descriptor_module_artifact(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const std::filesystem::path& relative_output_path,
    std::string_view package_name,
    std::string_view class_name,
    std::string_view module_name,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {}
)
{
    auto resolved_values =
        values.empty() ? java_descriptor_module_values(package_name, class_name, module_name)
                       : values;
    add_generated_template_file(
        result, options.output_dir, templates,
        generated_template_path("DescriptorModule.java.tmpl"),
        common_artifact_path(relative_output_path.generic_string()), diagnostics,
        GeneratedArtifactTier::Common, resolved_values
    );
}

void add_java_descriptor_module_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto descriptor_package_path =
        join_artifact_path(GeneratedJavaOutputPackagePath, "descriptors");
    const auto entity_descriptor_package_path = descriptor_package_path / "entities";
    const std::string descriptor_package = "com.statespec.generated.descriptors";
    const std::string entity_descriptor_package = descriptor_package + ".entities";

    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "CoreDescriptorModule.java",
        descriptor_package, "CoreDescriptorModule", "core descriptors", diagnostics
    );
    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "ShapeDescriptorModule.java",
        descriptor_package, "ShapeDescriptorModule", "shape descriptors", diagnostics,
        java_shape_descriptor_module_values(descriptor_package, "ShapeDescriptorModule", system)
    );
    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "ApiDescriptorModule.java",
        descriptor_package, "ApiDescriptorModule", "API descriptors", diagnostics
    );
    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "WorkerDescriptorModule.java",
        descriptor_package, "WorkerDescriptorModule", "worker descriptors", diagnostics
    );
    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "RuntimeDescriptorModule.java",
        descriptor_package, "RuntimeDescriptorModule", "runtime descriptors", diagnostics
    );
    for (const auto& entity : system.entities)
    {
        const auto class_name = java_entity_descriptor_module_class_name(entity.name);
        add_generated_template_file(
            result, options.output_dir, templates,
            generated_template_path("DescriptorModule.java.tmpl"),
            common_artifact_path(
                (entity_descriptor_package_path / (class_name + ".java")).generic_string()
            ),
            diagnostics, GeneratedArtifactTier::Common, java_entity_descriptor_module_values(entity)
        );
    }
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
    const auto output_root = artifact_path(GeneratedJavaBackendPackagePath);

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
        result, options.output_dir, templates, output_root / "SchemaCompatibility.java",
        output_root / "SchemaCompatibility.java", diagnostics
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
    if (usage.uses_entity_gc)
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            template_path_for_output(output_root / "runtime" / "EntityGcDescriptors.java"),
            common_artifact_path(
                (output_root / "runtime" / "EntityGcDescriptors.java").generic_string()
            ),
            diagnostics, GeneratedArtifactTier::Common, java_entity_gc_descriptor_values(system)
        );
        add_template_file(
            result, options.output_dir, templates,
            output_root / "runtime" / "EntityGcRepository.java",
            output_root / "runtime" / "EntityGcRepository.java", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, output_root / "runtime" / "EntityGcWorkers.java",
            output_root / "runtime" / "EntityGcWorkers.java", diagnostics
        );
    }
    add_java_descriptor_module_artifacts(result, options, templates, system, diagnostics);
    if (diagnostics.has_errors())
    {
        return;
    }
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Descriptors.java.tmpl"),
        common_artifact_path(
            join_artifact_path(GeneratedJavaOutputPackagePath, "Descriptors.java").generic_string()
        ),
        diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{{"descriptors", generate_descriptors_java(system, templates)}}
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Makefile.tmpl"),
        artifact_path(GeneratedArtifactMakefile), diagnostics, GeneratedArtifactTier::Common,
        java_makefile_values(options.tier, system), common_artifact_path(GeneratedArtifactMakefile)
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

    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiDescriptors.java"),
        GeneratedArtifactTier::Api, diagnostics
    );
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiCodecs.java"),
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{{"api_codecs", generate_api_codecs_java(system)}}
    );
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiHandlers.java"),
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_operation_handler_methods", generate_api_operation_handler_methods_java(system)}
        }
    );
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiHandlerRegistry.java"),
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_operation_default_handler_methods",
             generate_api_operation_default_handler_methods_java(system)}
        }
    );
    add_java_generated_template_file(
        result, options, templates,
        java_api_generated_path("ExternalSystemOperatorMetadataApi.java"),
        GeneratedArtifactTier::Api, diagnostics
    );
    if (include_api_composition)
    {
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiApplication.java"),
            GeneratedArtifactTier::Api, diagnostics, java_api_runtime_bootstrap_values(system)
        );
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiProcess.java"),
            GeneratedArtifactTier::Api, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiDispatcher.java"),
            GeneratedArtifactTier::Api, diagnostics,
            TemplateRenderer::Values{
                {"api_operation_dispatch_cases", generate_api_operation_dispatch_cases_java(system)}
            }
        );
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiServer.java"),
            GeneratedArtifactTier::Api, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiTransport.java"),
            GeneratedArtifactTier::Api, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiRoutes.java"),
            GeneratedArtifactTier::Api, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiMain.java"),
            GeneratedArtifactTier::Api, diagnostics
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

    add_java_generated_template_file(
        result, options, templates, java_worker_generated_path("WorkerDescriptors.java"),
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_java_generated_template_file(
        result, options, templates, java_worker_generated_path("WorkerContexts.java"),
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_java_generated_template_file(
        result, options, templates, java_worker_generated_path("WorkerRegistry.java"),
        GeneratedArtifactTier::Worker, diagnostics
    );
    if (include_worker_composition)
    {
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerApplication.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerProcess.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerRuntime.java"),
            GeneratedArtifactTier::Worker, diagnostics, java_runtime_bootstrap_values(system)
        );
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerMain.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
    if (include_worker_execution)
    {
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkflowStepHandlers.java"),
            GeneratedArtifactTier::Worker, diagnostics,
            TemplateRenderer::Values{
                {"workflow_step_handler_methods",
                 generate_workflow_step_handler_methods_java(system)},
                {"default_workflow_step_handler_methods",
                 generate_default_workflow_step_handler_methods_java(system)},
                {"workflow_step_handler_keys", generate_workflow_step_handler_keys_java(system)}
            }
        );
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkflowRunner.java"),
            GeneratedArtifactTier::Worker, diagnostics,
            TemplateRenderer::Values{
                {"workflow_step_dispatch_cases",
                 generate_workflow_step_dispatch_cases_java(system)},
                {"workflow_step_next_cases", generate_workflow_step_next_cases_java(system)}
            }
        );
    }
    if (usage.uses_queues)
    {
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerQueues.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerLeases.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerWorkflows.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
}

} // namespace statespec
