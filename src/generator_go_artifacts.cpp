#include "generator_go_artifacts.hpp"

#include "generator_go_descriptors.hpp"
#include "generator_support.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>

namespace statespec
{

namespace
{

TemplateRenderer::Values go_makefile_values(BindingGenerationTier tier)
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
        api_rules << "check-api:\n";
        api_rules << "\t$(GO) test ./api/...\n\n";
        api_rules << "build-api:\n";
        api_rules << "\t$(GO) test ./api/...\n\n";
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
        worker_rules << "\t$(GO) test ./worker/...\n\n";
        worker_rules << "build-worker:\n";
        worker_rules << "\t$(GO) test ./worker/...\n\n";
        worker_rules << "package-worker: build-worker $(DIST_DIR)\n";
        worker_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-worker-go.tgz common worker "
                        "go.mod Makefile\n\n";
    }
    return TemplateRenderer::Values{
        {"target_additions", target_additions.str()},
        {"phony_targets", phony_targets.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
    };
}

TemplateRenderer::Values go_runtime_bootstrap_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream arguments;
    if (usage.uses_feature_flags)
    {
        arguments << ", app.FeatureFlags";
    }
    if (usage.uses_queues)
    {
        arguments << ", app.Queues";
    }
    if (usage.uses_leases)
    {
        arguments << ", app.Leases";
    }
    if (usage.uses_workflows)
    {
        arguments << ", app.Workflows";
    }
    if (usage.uses_logs)
    {
        arguments << ", app.Logs";
    }
    if (usage.uses_metrics)
    {
        arguments << ", app.Metrics";
    }
    return TemplateRenderer::Values{{"runtime_bootstrap_arguments", arguments.str()}};
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

} // namespace

void add_go_common_runtime_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
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
        result, options.output_dir, templates, "backend/workflow.go", "backend/workflow.go",
        diagnostics
    );

    if (diagnostics.has_errors())
    {
        return;
    }

    add_generated_template_file(
        result, options.output_dir, templates, "backend/memory/backend.go.tmpl",
        "common/backend/memory/backend.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/memory/transaction.go.tmpl",
        "common/backend/memory/transaction.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/codec.go.tmpl",
        "common/backend/runtime/codec.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/codec_feature_flags.go.tmpl",
        "common/backend/runtime/codec_feature_flags.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/codec_queues.go.tmpl",
        "common/backend/runtime/codec_queues.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/codec_leases.go.tmpl",
        "common/backend/runtime/codec_leases.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/codec_workflows.go.tmpl",
        "common/backend/runtime/codec_workflows.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/codec_observability.go.tmpl",
        "common/backend/runtime/codec_observability.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/codec_logs.go.tmpl",
        "common/backend/runtime/codec_logs.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/codec_metrics.go.tmpl",
        "common/backend/runtime/codec_metrics.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/feature_flags.go.tmpl",
        "common/backend/runtime/feature_flags.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/queues.go.tmpl",
        "common/backend/runtime/queues.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/leases.go.tmpl",
        "common/backend/runtime/leases.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/workflows.go.tmpl",
        "common/backend/runtime/workflows.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/logs.go.tmpl",
        "common/backend/runtime/logs.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "backend/runtime/metrics.go.tmpl",
        "common/backend/runtime/metrics.go", diagnostics, GeneratedArtifactTier::Common
    );
    add_generated_template_file(
        result, options.output_dir, templates, "generated/descriptors.go.tmpl",
        "common/backend/descriptors.go", diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{{"descriptors", generate_descriptors_go(system, templates)}}
    );
    add_generated_template_file(
        result, options.output_dir, templates, "generated/go.mod.tmpl", "go.mod", diagnostics,
        GeneratedArtifactTier::Common, {}, "common/go.mod"
    );
    add_generated_template_file(
        result, options.output_dir, templates, "generated/Makefile.tmpl", "Makefile", diagnostics,
        GeneratedArtifactTier::Common, go_makefile_values(options.tier), "common/Makefile"
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
    add_generated_template_file(
        result, options.output_dir, templates, "api/backend/api_descriptors.go.tmpl",
        "api/backend/api_descriptors.go", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/backend/api_application.go.tmpl",
        "api/backend/api_application.go", diagnostics, GeneratedArtifactTier::Api,
        go_runtime_bootstrap_values(system)
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/backend/api_codecs.go.tmpl",
        "api/backend/api_codecs.go", diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{{"api_codecs", generate_api_codecs_go(system)}}
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/backend/api_handlers.go.tmpl",
        "api/backend/api_handlers.go", diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{
            {"api_operation_handler_methods", generate_api_operation_handler_methods_go(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/backend/api_handler_registry.go.tmpl",
        "api/backend/api_handler_registry.go", diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{
            {"api_operation_default_handler_methods",
             generate_api_operation_default_handler_methods_go(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/backend/api_dispatcher.go.tmpl",
        "api/backend/api_dispatcher.go", diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{
            {"api_operation_dispatch_cases", generate_api_operation_dispatch_cases_go(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/backend/api_server.go.tmpl",
        "api/backend/api_server.go", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/backend/api_routes.go.tmpl",
        "api/backend/api_routes.go", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates,
        "api/backend/external_system_operator_metadata_api.go.tmpl",
        "api/backend/external_system_operator_metadata_api.go", diagnostics,
        GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/cmd/api/main.go.tmpl", "api/cmd/api/main.go",
        diagnostics, GeneratedArtifactTier::Api
    );
}

void add_go_worker_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_generated_template_file(
        result, options.output_dir, templates, "worker/backend/worker_descriptors.go.tmpl",
        "worker/backend/worker_descriptors.go", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/backend/worker_contexts.go.tmpl",
        "worker/backend/worker_contexts.go", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/backend/worker_registry.go.tmpl",
        "worker/backend/worker_registry.go", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/backend/worker_application.go.tmpl",
        "worker/backend/worker_application.go", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/backend/worker_runtime.go.tmpl",
        "worker/backend/worker_runtime.go", diagnostics, GeneratedArtifactTier::Worker,
        go_worker_runtime_bootstrap_values(system)
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/backend/workflow_step_handlers.go.tmpl",
        "worker/backend/workflow_step_handlers.go", diagnostics, GeneratedArtifactTier::Worker,
        TemplateRenderer::Values{
            {"workflow_step_handler_methods", generate_workflow_step_handler_methods_go(system)},
            {"workflow_step_handler_keys", generate_workflow_step_handler_keys_go(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/backend/workflow_runner.go.tmpl",
        "worker/backend/workflow_runner.go", diagnostics, GeneratedArtifactTier::Worker,
        TemplateRenderer::Values{
            {"workflow_step_dispatch_cases", generate_workflow_step_dispatch_cases_go(system)},
            {"workflow_step_next_cases", generate_workflow_step_next_cases_go(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/backend/worker_queues.go.tmpl",
        "worker/backend/worker_queues.go", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/backend/worker_leases.go.tmpl",
        "worker/backend/worker_leases.go", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/backend/worker_workflows.go.tmpl",
        "worker/backend/worker_workflows.go", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/cmd/worker/main.go.tmpl",
        "worker/cmd/worker/main.go", diagnostics, GeneratedArtifactTier::Worker
    );
}

} // namespace statespec
