#include "generator_go_artifacts.hpp"

#include "generator_go_descriptors.hpp"
#include "generator_support.hpp"

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
        TemplateRenderer::Values{{"descriptors", generate_descriptors_go(system)}}
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
    DiagnosticBag& diagnostics
)
{
    add_generated_template_file(
        result, options.output_dir, templates, "api/backend/api_descriptors.go.tmpl",
        "api/backend/api_descriptors.go", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/backend/api_handlers.go.tmpl",
        "api/backend/api_handlers.go", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/backend/api_dispatcher.go.tmpl",
        "api/backend/api_dispatcher.go", diagnostics, GeneratedArtifactTier::Api
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
        result, options.output_dir, templates, "worker/backend/workflow_step_handlers.go.tmpl",
        "worker/backend/workflow_step_handlers.go", diagnostics, GeneratedArtifactTier::Worker,
        TemplateRenderer::Values{
            {"workflow_step_handler_keys", generate_workflow_step_handler_keys_go(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/backend/workflow_runner.go.tmpl",
        "worker/backend/workflow_runner.go", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/backend/worker_handlers.go.tmpl",
        "worker/backend/worker_handlers.go", diagnostics, GeneratedArtifactTier::Worker
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
}

} // namespace statespec
