#include "generator_rust_artifacts.hpp"

#include "generator_rust_descriptors.hpp"
#include "generator_support.hpp"

#include <sstream>

namespace statespec
{

namespace
{

TemplateRenderer::Values rust_makefile_values(BindingGenerationTier tier)
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
        api_rules << "\t$(CARGO) test\n\n";
        api_rules << "build-api:\n";
        api_rules << "\t$(CARGO) build\n\n";
        api_rules << "package-api: build-api $(DIST_DIR)\n";
        api_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-api-rust.tgz common api "
                     "Cargo.toml lib.rs Makefile\n\n";
    }
    if (include_worker)
    {
        target_additions << "\nCHECK_TARGETS += check-worker";
        target_additions << "\nBUILD_TARGETS += build-worker";
        target_additions << "\nPACKAGE_TARGETS += package-worker";
        phony_targets << " check-worker build-worker package-worker";
        worker_rules << "check-worker:\n";
        worker_rules << "\t$(CARGO) test\n\n";
        worker_rules << "build-worker:\n";
        worker_rules << "\t$(CARGO) build\n\n";
        worker_rules << "package-worker: build-worker $(DIST_DIR)\n";
        worker_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-worker-rust.tgz common worker "
                        "Cargo.toml lib.rs Makefile\n\n";
    }
    return TemplateRenderer::Values{
        {"target_additions", target_additions.str()},
        {"phony_targets", phony_targets.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
    };
}

TemplateRenderer::Values rust_lib_values(BindingGenerationTier tier)
{
    std::ostringstream api_modules;
    std::ostringstream worker_modules;
    if (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api)
    {
        api_modules << "#[path = \"api/api_descriptors.rs\"]\n";
        api_modules << "pub mod api_descriptors;\n";
        api_modules << "#[path = \"api/api_application.rs\"]\n";
        api_modules << "pub mod api_application;\n";
        api_modules << "#[path = \"api/api_dispatcher.rs\"]\n";
        api_modules << "pub mod api_dispatcher;\n";
        api_modules << "#[path = \"api/api_handler_registry.rs\"]\n";
        api_modules << "pub mod api_handler_registry;\n";
        api_modules << "#[path = \"api/api_handlers.rs\"]\n";
        api_modules << "pub mod api_handlers;\n";
        api_modules << "#[path = \"api/api_routes.rs\"]\n";
        api_modules << "pub mod api_routes;\n";
        api_modules << "#[path = \"api/api_server.rs\"]\n";
        api_modules << "pub mod api_server;\n";
        api_modules << "#[path = \"api/external_system_operator_metadata_api.rs\"]\n";
        api_modules << "pub mod external_system_operator_metadata_api;\n";
        api_modules << "#[path = \"api/main.rs\"]\n";
        api_modules << "pub mod api_main;\n";
    }
    if (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker)
    {
        worker_modules << "#[path = \"worker/worker_contexts.rs\"]\n";
        worker_modules << "pub mod worker_contexts;\n";
        worker_modules << "#[path = \"worker/worker_descriptors.rs\"]\n";
        worker_modules << "pub mod worker_descriptors;\n";
        worker_modules << "#[path = \"worker/worker_handlers.rs\"]\n";
        worker_modules << "pub mod worker_handlers;\n";
        worker_modules << "#[path = \"worker/worker_leases.rs\"]\n";
        worker_modules << "pub mod worker_leases;\n";
        worker_modules << "#[path = \"worker/worker_queues.rs\"]\n";
        worker_modules << "pub mod worker_queues;\n";
        worker_modules << "#[path = \"worker/worker_workflows.rs\"]\n";
        worker_modules << "pub mod worker_workflows;\n";
        worker_modules << "#[path = \"worker/worker_registry.rs\"]\n";
        worker_modules << "pub mod worker_registry;\n";
        worker_modules << "#[path = \"worker/worker_application.rs\"]\n";
        worker_modules << "pub mod worker_application;\n";
        worker_modules << "#[path = \"worker/worker_runtime.rs\"]\n";
        worker_modules << "pub mod worker_runtime;\n";
        worker_modules << "#[path = \"worker/workflow_step_handlers.rs\"]\n";
        worker_modules << "pub mod workflow_step_handlers;\n";
        worker_modules << "#[path = \"worker/workflow_runner.rs\"]\n";
        worker_modules << "pub mod workflow_runner;\n";
        worker_modules << "#[path = \"worker/main.rs\"]\n";
        worker_modules << "pub mod worker_main;\n";
    }
    return TemplateRenderer::Values{
        {"api_modules", api_modules.str()},
        {"worker_modules", worker_modules.str()},
    };
}

} // namespace

void add_rust_common_runtime_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_template_file(result, options.output_dir, templates, "json.rs", "json.rs", diagnostics);
    add_template_file(
        result, options.output_dir, templates, "backend.rs", "backend.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "external_system.rs", "external_system.rs",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "feature_flag.rs", "feature_flag.rs", diagnostics
    );
    add_template_file(result, options.output_dir, templates, "lease.rs", "lease.rs", diagnostics);
    add_template_file(result, options.output_dir, templates, "log.rs", "log.rs", diagnostics);
    add_template_file(result, options.output_dir, templates, "metric.rs", "metric.rs", diagnostics);
    add_template_file(result, options.output_dir, templates, "queue.rs", "queue.rs", diagnostics);
    add_template_file(
        result, options.output_dir, templates, "workflow.rs", "workflow.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "memory/backend.rs", "memory/backend.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "memory/transaction.rs", "memory/transaction.rs",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec.rs", "runtime/codec.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_core.rs", "runtime/codec_core.rs",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_feature_flags.rs",
        "runtime/codec_feature_flags.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_queues.rs", "runtime/codec_queues.rs",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_leases.rs", "runtime/codec_leases.rs",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_workflows.rs",
        "runtime/codec_workflows.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_observability.rs",
        "runtime/codec_observability.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_logs.rs", "runtime/codec_logs.rs",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/codec_metrics.rs",
        "runtime/codec_metrics.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/feature_flags.rs",
        "runtime/feature_flags.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/queues.rs", "runtime/queues.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/leases.rs", "runtime/leases.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/workflows.rs", "runtime/workflows.rs",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/logs.rs", "runtime/logs.rs", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "runtime/metrics.rs", "runtime/metrics.rs",
        diagnostics
    );

    if (diagnostics.has_errors())
    {
        return;
    }

    add_generated_template_file(
        result, options.output_dir, templates, "generated/descriptors.rs.tmpl",
        "common/descriptors.rs", diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{{"descriptors", generate_descriptors_rs(system)}}
    );
    add_generated_template_file(
        result, options.output_dir, templates, "generated/Cargo.toml.tmpl", "Cargo.toml",
        diagnostics, GeneratedArtifactTier::Common, {}, "common/Cargo.toml"
    );
    add_generated_template_file(
        result, options.output_dir, templates, "generated/lib.rs.tmpl", "lib.rs", diagnostics,
        GeneratedArtifactTier::Common, rust_lib_values(options.tier), "common/lib.rs"
    );
    add_generated_template_file(
        result, options.output_dir, templates, "generated/Makefile.tmpl", "Makefile", diagnostics,
        GeneratedArtifactTier::Common, rust_makefile_values(options.tier), "common/Makefile"
    );
}

void add_rust_api_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_generated_template_file(
        result, options.output_dir, templates, "api/api_descriptors.rs.tmpl",
        "api/api_descriptors.rs", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/api_application.rs.tmpl",
        "api/api_application.rs", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/api_handlers.rs.tmpl", "api/api_handlers.rs",
        diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{
            {"api_operation_handler_methods", generate_api_operation_handler_methods_rs(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/api_handler_registry.rs.tmpl",
        "api/api_handler_registry.rs", diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{
            {"api_operation_default_handler_methods",
             generate_api_operation_default_handler_methods_rs(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/api_dispatcher.rs.tmpl",
        "api/api_dispatcher.rs", diagnostics, GeneratedArtifactTier::Api,
        TemplateRenderer::Values{
            {"api_operation_dispatch_cases", generate_api_operation_dispatch_cases_rs(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/api_server.rs.tmpl", "api/api_server.rs",
        diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/api_routes.rs.tmpl", "api/api_routes.rs",
        diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/external_system_operator_metadata_api.rs.tmpl",
        "api/external_system_operator_metadata_api.rs", diagnostics, GeneratedArtifactTier::Api
    );
    add_generated_template_file(
        result, options.output_dir, templates, "api/main.rs.tmpl", "api/main.rs", diagnostics,
        GeneratedArtifactTier::Api
    );
}

void add_rust_worker_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_descriptors.rs.tmpl",
        "worker/worker_descriptors.rs", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_contexts.rs.tmpl",
        "worker/worker_contexts.rs", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_registry.rs.tmpl",
        "worker/worker_registry.rs", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_application.rs.tmpl",
        "worker/worker_application.rs", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_runtime.rs.tmpl",
        "worker/worker_runtime.rs", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/workflow_step_handlers.rs.tmpl",
        "worker/workflow_step_handlers.rs", diagnostics, GeneratedArtifactTier::Worker,
        TemplateRenderer::Values{
            {"workflow_step_handler_keys", generate_workflow_step_handler_keys_rs(system)}
        }
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/workflow_runner.rs.tmpl",
        "worker/workflow_runner.rs", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_handlers.rs.tmpl",
        "worker/worker_handlers.rs", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_queues.rs.tmpl",
        "worker/worker_queues.rs", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_leases.rs.tmpl",
        "worker/worker_leases.rs", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/worker_workflows.rs.tmpl",
        "worker/worker_workflows.rs", diagnostics, GeneratedArtifactTier::Worker
    );
    add_generated_template_file(
        result, options.output_dir, templates, "worker/main.rs.tmpl", "worker/main.rs", diagnostics,
        GeneratedArtifactTier::Worker
    );
}

} // namespace statespec
