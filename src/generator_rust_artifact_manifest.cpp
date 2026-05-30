#include "generator_rust_artifact_manifest.hpp"

#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <algorithm>
#include <sstream>

namespace statespec
{
namespace
{

bool rust_manifest_api_uses_shape(
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

bool rust_manifest_has_common_shapes(const IrSystem& system)
{
    return std::any_of(
        system.shapes.begin(), system.shapes.end(),
        [&](const auto& shape) { return !rust_manifest_api_uses_shape(system, shape.name); }
    );
}

} // namespace

TemplateRenderer::Values rust_makefile_values(
    BindingGenerationTier tier,
    const IrSystem& system
)
{
    const auto usage = runtime_domain_usage(system);
    const auto include_api =
        (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api) &&
        (!system.apis.empty() || !system.api_servers.empty());
    const auto include_worker =
        (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker) &&
        (!system.workers.empty() || usage.uses_workflows);

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
        help_target_additions
            << "\t@printf '%s\\n' '  check-worker  build-worker  package-worker'\n";
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
        {"help_target_additions", help_target_additions.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
    };
}

TemplateRenderer::Values rust_lib_values(
    BindingGenerationTier tier,
    const IrSystem& system
)
{
    const auto usage = runtime_domain_usage(system);
    const auto include_api_composition =
        (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api) &&
        !system.api_servers.empty();
    const auto include_api =
        (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api) &&
        (!system.apis.empty() || !system.api_servers.empty());
    const auto include_worker =
        (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker) &&
        (!system.workers.empty() || usage.uses_workflows);
    const auto include_worker_composition = include_worker && !system.workers.empty();
    const auto include_worker_execution =
        include_worker && (include_worker_composition || usage.uses_workflows);
    std::ostringstream runtime_modules;
    std::ostringstream api_modules;
    std::ostringstream worker_modules;
    if (rust_manifest_has_common_shapes(system))
    {
        runtime_modules << "#[path = \"common/shapes.rs\"]\n";
        runtime_modules << "pub mod common_shapes;\n";
    }
    runtime_modules << "#[path = \"common/entity_repository.rs\"]\n";
    runtime_modules << "pub mod entity_repository;\n";
    runtime_modules << "#[path = \"common/runtime_registration.rs\"]\n";
    runtime_modules << "pub mod runtime_registration;\n";
    if (usage.uses_any_runtime_domain)
    {
        runtime_modules << "#[path = \"common/runtime/codec.rs\"]\n";
        runtime_modules << "pub(crate) mod runtime_codec;\n";
    }
    if (usage.uses_feature_flags)
    {
        runtime_modules << "#[path = \"common/runtime/feature_flags.rs\"]\n";
        runtime_modules << "pub mod runtime_feature_flags;\n";
    }
    if (usage.uses_leases)
    {
        runtime_modules << "#[path = \"common/runtime/leases.rs\"]\n";
        runtime_modules << "pub mod runtime_leases;\n";
    }
    if (usage.uses_logs)
    {
        runtime_modules << "#[path = \"common/runtime/logs.rs\"]\n";
        runtime_modules << "pub mod runtime_logs;\n";
    }
    if (usage.uses_metrics)
    {
        runtime_modules << "#[path = \"common/runtime/metrics.rs\"]\n";
        runtime_modules << "pub mod runtime_metrics;\n";
    }
    if (usage.uses_queues)
    {
        runtime_modules << "#[path = \"common/runtime/queues.rs\"]\n";
        runtime_modules << "pub mod runtime_queues;\n";
    }
    if (usage.uses_workflows)
    {
        runtime_modules << "#[path = \"common/runtime/workflows.rs\"]\n";
        runtime_modules << "pub mod runtime_workflows;\n";
    }
    if (usage.uses_entity_gc)
    {
        runtime_modules << "#[path = \"common/runtime/entity_gc_types.rs\"]\n";
        runtime_modules << "pub mod runtime_entity_gc_types;\n";
        runtime_modules << "#[path = \"common/runtime/entity_gc_repository.rs\"]\n";
        runtime_modules << "pub mod runtime_entity_gc_repository;\n";
        runtime_modules << "#[path = \"common/runtime/entity_gc_workers.rs\"]\n";
        runtime_modules << "pub mod runtime_entity_gc_workers;\n";
        runtime_modules << "#[path = \"common/runtime/entity_gc_registration.rs\"]\n";
        runtime_modules << "pub mod runtime_entity_gc_registration;\n";
    }
    for (const auto& entity : system.entities)
    {
        runtime_modules << "#[path = \"common/entities/" << snake_identifier(entity.name)
                        << "/mod.rs\"]\n";
        runtime_modules << "pub mod entity_" << snake_identifier(entity.name) << ";\n";
    }
    if (include_api)
    {
        api_modules << "#[path = \"api/shapes.rs\"]\n";
        api_modules << "pub mod api_shapes;\n";
        api_modules << "#[path = \"api/api_descriptors.rs\"]\n";
        api_modules << "pub mod api_descriptors;\n";
        api_modules << "#[path = \"api/api_codecs.rs\"]\n";
        api_modules << "pub mod api_codecs;\n";
        api_modules << "#[path = \"api/api_handler_registry.rs\"]\n";
        api_modules << "pub mod api_handler_registry;\n";
        api_modules << "#[path = \"api/api_handlers.rs\"]\n";
        api_modules << "pub mod api_handlers;\n";
        api_modules << "#[path = \"api/external_system_operator_metadata_api.rs\"]\n";
        api_modules << "pub mod external_system_operator_metadata_api;\n";
        if (include_api_composition)
        {
            if (usage.uses_entity_gc)
            {
                api_modules << "#[path = \"api/entity_gc_catalog.rs\"]\n";
                api_modules << "pub mod api_entity_gc_catalog;\n";
            }
            api_modules << "#[path = \"api/api_application.rs\"]\n";
            api_modules << "pub mod api_application;\n";
            api_modules << "#[path = \"api/api_process.rs\"]\n";
            api_modules << "pub mod api_process;\n";
            api_modules << "#[path = \"api/api_dispatcher.rs\"]\n";
            api_modules << "pub mod api_dispatcher;\n";
            api_modules << "#[path = \"api/api_routes.rs\"]\n";
            api_modules << "pub mod api_routes;\n";
            api_modules << "#[path = \"api/api_server.rs\"]\n";
            api_modules << "pub mod api_server;\n";
            api_modules << "#[path = \"api/api_transport.rs\"]\n";
            api_modules << "pub mod api_transport;\n";
            api_modules << "#[path = \"api/main.rs\"]\n";
            api_modules << "pub mod api_main;\n";
        }
    }
    if (include_worker)
    {
        worker_modules << "#[path = \"worker/worker_contexts.rs\"]\n";
        worker_modules << "pub mod worker_contexts;\n";
        worker_modules << "#[path = \"worker/worker_descriptors.rs\"]\n";
        worker_modules << "pub mod worker_descriptors;\n";
        worker_modules << "#[path = \"worker/worker_registry.rs\"]\n";
        worker_modules << "pub mod worker_registry;\n";
        if (usage.uses_leases)
        {
            worker_modules << "#[path = \"worker/worker_leases.rs\"]\n";
            worker_modules << "pub mod worker_leases;\n";
        }
        if (usage.uses_queues)
        {
            worker_modules << "#[path = \"worker/worker_queues.rs\"]\n";
            worker_modules << "pub mod worker_queues;\n";
        }
        if (usage.uses_workflows)
        {
            worker_modules << "#[path = \"worker/worker_workflows.rs\"]\n";
            worker_modules << "pub mod worker_workflows;\n";
        }
        if (include_worker_composition)
        {
            if (usage.uses_entity_gc)
            {
                worker_modules << "#[path = \"worker/entity_gc_catalog.rs\"]\n";
                worker_modules << "pub mod worker_entity_gc_catalog;\n";
            }
            worker_modules << "#[path = \"worker/worker_application.rs\"]\n";
            worker_modules << "pub mod worker_application;\n";
            worker_modules << "#[path = \"worker/worker_process.rs\"]\n";
            worker_modules << "pub mod worker_process;\n";
            worker_modules << "#[path = \"worker/worker_runtime.rs\"]\n";
            worker_modules << "pub mod worker_runtime;\n";
            worker_modules << "#[path = \"worker/main.rs\"]\n";
            worker_modules << "pub mod worker_main;\n";
        }
        if (include_worker_execution)
        {
            for (const auto& workflow : system.workflows)
            {
                worker_modules << "#[path = \"worker/workflows/" << snake_identifier(workflow.name)
                               << "/handlers.rs\"]\n";
                worker_modules << "pub mod workflow_" << snake_identifier(workflow.name)
                               << "_handlers;\n";
                worker_modules << "#[path = \"worker/workflows/" << snake_identifier(workflow.name)
                               << "/registry.rs\"]\n";
                worker_modules << "pub mod workflow_" << snake_identifier(workflow.name)
                               << "_registry;\n";
            }
            worker_modules << "#[path = \"worker/workflow_step_handlers.rs\"]\n";
            worker_modules << "pub mod workflow_step_handlers;\n";
            worker_modules << "#[path = \"worker/workflow_step_registry.rs\"]\n";
            worker_modules << "pub mod workflow_step_registry;\n";
            worker_modules << "#[path = \"worker/workflow_runner.rs\"]\n";
            worker_modules << "pub mod workflow_runner;\n";
        }
    }
    return TemplateRenderer::Values{
        {"runtime_modules", runtime_modules.str()},
        {"api_modules", api_modules.str()},
        {"worker_modules", worker_modules.str()},
    };
}

} // namespace statespec
