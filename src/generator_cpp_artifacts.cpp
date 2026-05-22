#include "generator_cpp_artifacts.hpp"

#include "generator_cpp_descriptors.hpp"
#include "generator_support.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{

namespace
{

TemplateRenderer::Values cpp_makefile_values(BindingGenerationTier tier)
{
    const auto include_api =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api;
    const auto include_worker =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker;

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
                     "#include \"api/api_application.hpp\"\\n"
                     "#include \"api/api_codecs.hpp\"\\n"
                     "#include \"api/api_dispatcher.hpp\"\\n"
                     "#include \"api/api_handler_registry.hpp\"\\n"
                     "#include \"api/api_handlers.hpp\"\\n"
                     "#include \"api/api_routes.hpp\"\\n"
                     "#include \"api/api_server.hpp\"\\n"
                     "#include \"api/external_system_operator_metadata_api.hpp\"\\n"
                     "int main() { return 0; }\\n' | "
                     "$(CXX) $(CXXFLAGS) -x c++ - -o $(BUILD_DIR)/check-api\n\n";
        api_rules << "build-api: check-api\n";
        api_rules << "\t$(CXX) $(CXXFLAGS) api/main.cpp -o $(BUILD_DIR)/api-main\n\n";
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
        worker_rules << "\tprintf '#include \"worker/worker_application.hpp\"\\n"
                        "#include \"worker/worker_contexts.hpp\"\\n"
                        "#include \"worker/worker_descriptors.hpp\"\\n"
                        "#include \"worker/worker_leases.hpp\"\\n"
                        "#include \"worker/worker_queues.hpp\"\\n"
                        "#include \"worker/worker_registry.hpp\"\\n"
                        "#include \"worker/worker_runtime.hpp\"\\n"
                        "#include \"worker/worker_workflows.hpp\"\\n"
                        "#include \"worker/workflow_runner.hpp\"\\n"
                        "#include \"worker/workflow_step_handlers.hpp\"\\n"
                        "int main() { return 0; }\\n' | "
                        "$(CXX) $(CXXFLAGS) -x c++ - -o $(BUILD_DIR)/check-worker\n\n";
        worker_rules << "build-worker: check-worker\n";
        worker_rules << "\t$(CXX) $(CXXFLAGS) worker/main.cpp -o $(BUILD_DIR)/worker-main\n\n";
        worker_rules << "package-worker: build-worker $(DIST_DIR)\n";
        worker_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-worker-cpp.tgz common worker "
                        "Makefile\n\n";
    }
    return TemplateRenderer::Values{
        {"target_additions", target_additions.str()},
        {"phony_targets", phony_targets.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
    };
}

TemplateRenderer::Values cpp_runtime_bootstrap_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream arguments;
    if (usage.uses_feature_flags)
    {
        arguments << ", feature_flags_";
    }
    if (usage.uses_queues)
    {
        arguments << ", queues_";
    }
    if (usage.uses_leases)
    {
        arguments << ", leases_";
    }
    if (usage.uses_workflows)
    {
        arguments << ", workflows_";
    }
    if (usage.uses_logs)
    {
        arguments << ", logs_";
    }
    if (usage.uses_metrics)
    {
        arguments << ", metrics_";
    }
    return TemplateRenderer::Values{{"runtime_bootstrap_arguments", arguments.str()}};
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
    add_template_file(
        result, options.output_dir, templates, "runtime/codec.hpp", "runtime/codec.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_core.hpp", "runtime/codec_core.hpp",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_feature_flags.hpp",
        "runtime/codec_feature_flags.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_queues.hpp",
        "runtime/codec_queues.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_leases.hpp",
        "runtime/codec_leases.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_workflows.hpp",
        "runtime/codec_workflows.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_observability.hpp",
        "runtime/codec_observability.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_logs.hpp", "runtime/codec_logs.hpp",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_metrics.hpp",
        "runtime/codec_metrics.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/feature_flag_store.hpp",
        "runtime/feature_flag_store.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/queue_store.hpp", "runtime/queue_store.hpp",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/lease_store.hpp", "runtime/lease_store.hpp",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/workflow_store.hpp",
        "runtime/workflow_store.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/log_sink.hpp", "runtime/log_sink.hpp",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/metric_sink.hpp", "runtime/metric_sink.hpp",
        diagnostics
    );

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
        GeneratedArtifactTier::Common, cpp_makefile_values(options.tier), "common/Makefile"
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
    add_generated_template_file(
        result, options.output_dir, templates, "api/api_descriptors.hpp.tmpl",
        "api/api_descriptors.hpp", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/api_application.hpp.tmpl",
        "api/api_application.hpp", diagnostics, GeneratedArtifactTier::Api,
        cpp_runtime_bootstrap_values(system)
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
        result, options.output_dir, templates, "api/external_system_operator_metadata_api.hpp.tmpl",
        "api/external_system_operator_metadata_api.hpp", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/main.cpp.tmpl", "api/main.cpp", diagnostics,
        GeneratedArtifactTier::Api
    );
}

void add_cpp_worker_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
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
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_queues.hpp.tmpl",
        "worker/worker_queues.hpp", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_leases.hpp.tmpl",
        "worker/worker_leases.hpp", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_workflows.hpp.tmpl",
        "worker/worker_workflows.hpp", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/main.cpp.tmpl", "worker/main.cpp",
        diagnostics, GeneratedArtifactTier::Worker
    );
}

} // namespace statespec
