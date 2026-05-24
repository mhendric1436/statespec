#include "generator_rust_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_rust_descriptor_areas.hpp"
#include "generator_rust_descriptor_support.hpp"
#include "generator_rust_descriptors.hpp"
#include "generator_support.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <sstream>
#include <utility>

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

TemplateRenderer::Values rust_lib_values(
    BindingGenerationTier tier,
    const IrSystem& system
)
{
    const auto usage = runtime_domain_usage(system);
    const auto include_api_composition =
        (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api) &&
        !system.api_servers.empty();
    const auto include_worker =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker;
    const auto include_worker_composition = include_worker && !system.workers.empty();
    const auto include_worker_execution =
        include_worker && (include_worker_composition || usage.uses_workflows);
    std::ostringstream runtime_modules;
    std::ostringstream api_modules;
    std::ostringstream worker_modules;
    runtime_modules << "#[path = \"common/shapes.rs\"]\n";
    runtime_modules << "pub mod shapes;\n";
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
        runtime_modules << "#[path = \"common/runtime/entity_gc_descriptors.rs\"]\n";
        runtime_modules << "pub mod runtime_entity_gc_descriptors;\n";
        runtime_modules << "#[path = \"common/runtime/entity_gc_repository.rs\"]\n";
        runtime_modules << "pub mod runtime_entity_gc_repository;\n";
        runtime_modules << "#[path = \"common/runtime/entity_gc_workers.rs\"]\n";
        runtime_modules << "pub mod runtime_entity_gc_workers;\n";
    }
    if (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api)
    {
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
            worker_modules << "#[path = \"worker/workflow_step_handlers.rs\"]\n";
            worker_modules << "pub mod workflow_step_handlers;\n";
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

TemplateRenderer::Values rust_runtime_bootstrap_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream imports;
    std::ostringstream worker_imports;
    std::ostringstream fields;
    std::ostringstream initializers;
    std::ostringstream generics;
    std::ostringstream arguments;
    std::ostringstream worker_run_once;
    auto add =
        [&](bool used, std::string_view module, std::string_view type, std::string_view member)
    {
        if (!used)
        {
            return;
        }
        imports << "use crate::" << module << "::" << type << ";\n";
        fields << "    pub " << member << ": " << type << ",\n";
        initializers << "            " << member << ": " << type << "::new(),\n";
        generics << ", " << type;
        arguments << ", &self." << member;
    };
    add(usage.uses_feature_flags, "runtime_feature_flags", "RuntimeFeatureFlagStore",
        "feature_flags");
    add(usage.uses_queues, "runtime_queues", "RuntimeQueueStore", "queues");
    add(usage.uses_leases, "runtime_leases", "RuntimeLeaseStore", "leases");
    add(usage.uses_workflows, "runtime_workflows", "RuntimeWorkflowStore", "workflows");
    add(usage.uses_logs, "runtime_logs", "RuntimeLogSink", "logs");
    add(usage.uses_metrics, "runtime_metrics", "RuntimeMetricSink", "metrics");
    if (usage.uses_workflows)
    {
        worker_imports << "use crate::workflow_runner::WorkflowRunner;\n";
        worker_imports << "use crate::workflow_step_handlers::WorkflowStepHandler;\n";
        worker_imports << "use crate::worker_contexts::WorkerContext;\n";
        worker_run_once
            << "    pub fn run_once(\n"
            << "        &self,\n"
            << "        context: &WorkerContext,\n"
            << "        handler: &impl WorkflowStepHandler,\n"
            << "        workflow_execution_id: &str,\n"
            << "    ) -> "
               "crate::backend::BackendResult<Option<crate::workflow::WorkflowExecutionRecord>> "
               "{\n"
            << "        let Some(executes) = context.executes.as_ref() else {\n"
            << "            return Ok(None);\n"
            << "        };\n"
            << "        let runner = WorkflowRunner {\n"
            << "            backend: &self.backend,\n"
            << "            workflow_store: &self.workflows,\n"
            << "            handler,\n"
            << "            worker_name: context.worker_name.clone(),\n"
            << "            lease_duration: std::time::Duration::from_secs(30),\n"
            << "            max_attempts: 3,\n"
            << "        };\n"
            << "        runner.run_once(workflow_execution_id, executes, 1)\n"
            << "    }\n";
    }
    else
    {
        worker_imports << "use crate::workflow_step_handlers::WorkflowStepHandler;\n";
        worker_imports << "use crate::worker_contexts::WorkerContext;\n";
        worker_run_once
            << "    pub fn run_once(\n"
            << "        &self,\n"
            << "        _context: &WorkerContext,\n"
            << "        _handler: &impl WorkflowStepHandler,\n"
            << "        _workflow_execution_id: &str,\n"
            << "    ) -> "
               "crate::backend::BackendResult<Option<crate::workflow::WorkflowExecutionRecord>> "
               "{\n"
            << "        Ok(None)\n"
            << "    }\n";
    }
    return TemplateRenderer::Values{
        {"runtime_store_imports", imports.str()},
        {"worker_runtime_imports", worker_imports.str()},
        {"runtime_store_fields", fields.str()},
        {"runtime_store_initializers", initializers.str()},
        {"runtime_bootstrap_generic_arguments", generics.str()},
        {"runtime_bootstrap_arguments", arguments.str()},
        {"worker_runtime_run_once", worker_run_once.str()},
    };
}

TemplateRenderer::Values rust_runtime_codec_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream modules;
    std::ostringstream uses;
    auto add = [&](bool used, std::string_view name, bool reexport = true)
    {
        if (!used)
        {
            return;
        }
        modules << "#[path = \"" << name << ".rs\"]\n";
        modules << "mod " << name << ";\n";
        if (reexport)
        {
            uses << "pub(crate) use self::" << name << "::*;\n";
        }
    };
    add(true, "codec_core");
    add(usage.uses_feature_flags, "codec_feature_flags");
    add(usage.uses_leases, "codec_leases");
    add(usage.uses_logs, "codec_logs");
    add(usage.uses_metrics, "codec_metrics");
    add(usage.uses_observability, "codec_observability", false);
    add(usage.uses_queues, "codec_queues");
    add(usage.uses_workflows, "codec_workflows");
    return TemplateRenderer::Values{
        {"runtime_codec_modules", modules.str()},
        {"runtime_codec_uses", uses.str()},
    };
}

TemplateRenderer::Values rust_api_runtime_bootstrap_values(const IrSystem& system)
{
    auto values = rust_runtime_bootstrap_values(system);
    values.erase("worker_runtime_imports");
    values.erase("worker_runtime_run_once");
    return values;
}

TemplateRenderer::Values rust_entity_gc_descriptor_values(const IrSystem& system)
{
    std::ostringstream descriptors;
    for (const auto& entity : system.entities)
    {
        std::ostringstream terminal_states;
        for (const auto& state : entity.states)
        {
            if (!state.garbage_collection.has_value())
            {
                continue;
            }
            terminal_states << "            EntityGcTerminalStateDescriptor {\n"
                            << "                state: "
                            << rust_entity_state_constant_name(entity.name, state.name)
                            << ".to_string(),\n"
                            << "                after: "
                            << rust_string(state.garbage_collection->after) << ".to_string(),\n"
                            << "                mode: "
                            << rust_string(state.garbage_collection->mode) << ".to_string(),\n"
                            << "            },\n";
        }
        if (terminal_states.str().empty())
        {
            continue;
        }
        descriptors << "        EntityGcDescriptor {\n"
                    << "            entity: " << rust_entity_name_constant_name(entity.name)
                    << ".to_string(),\n"
                    << "            collection: " << rust_entity_name_constant_name(entity.name)
                    << ".to_string(),\n"
                    << "            terminal_states: vec![\n"
                    << terminal_states.str() << "            ],\n"
                    << "        },\n";
    }
    return TemplateRenderer::Values{{"entity_gc_descriptors", descriptors.str()}};
}

void add_rust_common_generated_template_file(
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

TemplateRenderer::Values rust_descriptor_module_values(std::string_view module_name)
{
    return TemplateRenderer::Values{
        {"descriptor_module_name", std::string{module_name}},
        {"descriptor_module_content", ""},
    };
}

std::string replace_all_copy(
    std::string value,
    std::string_view from,
    std::string_view to
)
{
    std::size_t pos = 0;
    while ((pos = value.find(from, pos)) != std::string::npos)
    {
        value.replace(pos, from.size(), to);
        pos += to.size();
    }
    return value;
}

TemplateRenderer::Values rust_entity_descriptor_module_values(const IrEntity& entity)
{
    IrSystem one_entity_system;
    one_entity_system.entities.push_back(entity);
    auto content =
        std::string{"use super::*;\n\n"} + generate_rust_entity_descriptors(one_entity_system);
    const auto prefix = snake_identifier(entity.name);
    content = replace_all_copy(content, "entity_descriptors()", prefix + "_entity_descriptors()");
    content =
        replace_all_copy(content, "collection_descriptors()", prefix + "_collection_descriptors()");
    return TemplateRenderer::Values{
        {"descriptor_module_name", "entity descriptor " + entity.name},
        {"descriptor_module_content", content},
    };
}

TemplateRenderer::Values rust_shape_descriptor_module_values(const IrSystem& system)
{
    return TemplateRenderer::Values{
        {"descriptor_module_name", "shape descriptors"},
        {"descriptor_module_content",
         "use super::*;\n\n" + generate_rust_shape_descriptors(system)},
    };
}

void add_rust_raw_common_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    std::string_view relative_output_path,
    std::string content
)
{
    const auto relative_path = common_artifact_path(relative_output_path);
    result.files.push_back(
        GeneratedFile{
            (options.output_dir / relative_path).string(),
            std::move(content),
            GeneratedArtifactTier::Common,
            relative_path.generic_string(),
        }
    );
}

std::string rust_shape_type_file(const IrShape& shape)
{
    std::ostringstream out;
    out << "#[allow(unused_imports)]\n";
    out << "use crate::json::Json;\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct " << pascal_identifier(shape.name) << " {\n";
    for (const auto& field : shape.fields)
    {
        out << "    pub " << field.name << ": " << rust_shape_type(field.type) << ",\n";
    }
    out << "}\n";
    return out.str();
}

std::string rust_shapes_module(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& shape : system.shapes)
    {
        const auto module_name = "shape_" + snake_identifier(shape.name);
        out << "#[path = \"shapes/" << snake_identifier(shape.name) << ".rs\"]\n";
        out << "mod " << module_name << ";\n";
        out << "pub use " << module_name << "::*;\n";
    }
    return out.str();
}

void add_rust_shape_type_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    add_rust_raw_common_file(result, options, "shapes.rs", rust_shapes_module(system));
    for (const auto& shape : system.shapes)
    {
        add_rust_raw_common_file(
            result, options, "shapes/" + snake_identifier(shape.name) + ".rs",
            rust_shape_type_file(shape)
        );
    }
}

void add_rust_descriptor_module_artifact(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    std::string_view relative_output_path,
    std::string_view module_name,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {}
)
{
    const auto resolved_values =
        values.empty() ? rust_descriptor_module_values(module_name) : values;
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("descriptor_module.rs.tmpl"),
        common_artifact_path(relative_output_path), diagnostics, GeneratedArtifactTier::Common,
        resolved_values
    );
}

void add_rust_descriptor_module_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_rust_descriptor_module_artifact(
        result, options, templates, "descriptors/core.rs", "core descriptors", diagnostics
    );
    add_rust_descriptor_module_artifact(
        result, options, templates, "descriptors/shapes.rs", "shape descriptors", diagnostics,
        rust_shape_descriptor_module_values(system)
    );
    add_rust_descriptor_module_artifact(
        result, options, templates, "descriptors/apis.rs", "API descriptors", diagnostics
    );
    add_rust_descriptor_module_artifact(
        result, options, templates, "descriptors/workers.rs", "worker descriptors", diagnostics
    );
    add_rust_descriptor_module_artifact(
        result, options, templates, "descriptors/runtime.rs", "runtime descriptors", diagnostics
    );
    for (const auto& entity : system.entities)
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            generated_template_path("descriptor_module.rs.tmpl"),
            common_artifact_path("descriptors/entities/" + snake_identifier(entity.name) + ".rs"),
            diagnostics, GeneratedArtifactTier::Common, rust_entity_descriptor_module_values(entity)
        );
    }
}

void add_rust_generated_template_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    std::string_view relative_output_path,
    GeneratedArtifactTier tier,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {}
)
{
    add_generated_template_file(
        result, options.output_dir, templates, template_path_for_output(relative_output_path),
        artifact_path(relative_output_path), diagnostics, tier, values
    );
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
    const auto usage = runtime_domain_usage(system);

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
        result, options.output_dir, templates, "schema_compatibility.rs", "schema_compatibility.rs",
        diagnostics
    );
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
    if (usage.uses_any_runtime_domain)
    {
        add_rust_common_generated_template_file(
            result, options, templates, "runtime/codec.rs", diagnostics,
            rust_runtime_codec_values(system)
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_core.rs", "runtime/codec_core.rs",
            diagnostics
        );
    }
    if (usage.uses_feature_flags)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_feature_flags.rs",
            "runtime/codec_feature_flags.rs", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/feature_flags.rs",
            "runtime/feature_flags.rs", diagnostics
        );
    }
    if (usage.uses_queues)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_queues.rs",
            "runtime/codec_queues.rs", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/queues.rs", "runtime/queues.rs",
            diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_leases.rs",
            "runtime/codec_leases.rs", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/leases.rs", "runtime/leases.rs",
            diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_workflows.rs",
            "runtime/codec_workflows.rs", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/workflows.rs", "runtime/workflows.rs",
            diagnostics
        );
    }
    if (usage.uses_observability)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_observability.rs",
            "runtime/codec_observability.rs", diagnostics
        );
    }
    if (usage.uses_logs)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_logs.rs", "runtime/codec_logs.rs",
            diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/logs.rs", "runtime/logs.rs", diagnostics
        );
    }
    if (usage.uses_metrics)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_metrics.rs",
            "runtime/codec_metrics.rs", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/metrics.rs", "runtime/metrics.rs",
            diagnostics
        );
    }
    if (usage.uses_entity_gc)
    {
        add_rust_common_generated_template_file(
            result, options, templates, "runtime/entity_gc_descriptors.rs", diagnostics,
            rust_entity_gc_descriptor_values(system)
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/entity_gc_repository.rs",
            "runtime/entity_gc_repository.rs", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/entity_gc_workers.rs",
            "runtime/entity_gc_workers.rs", diagnostics
        );
    }

    if (diagnostics.has_errors())
    {
        return;
    }

    add_rust_shape_type_artifacts(result, options, system);
    add_rust_descriptor_module_artifacts(result, options, templates, system, diagnostics);
    if (diagnostics.has_errors())
    {
        return;
    }

    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("descriptors.rs.tmpl"),
        common_artifact_path("descriptors.rs"), diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{{"descriptors", generate_descriptors_rs(system, templates)}}
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Cargo.toml.tmpl"),
        artifact_path(GeneratedArtifactCargoToml), diagnostics, GeneratedArtifactTier::Common, {},
        common_artifact_path(GeneratedArtifactCargoToml)
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("lib.rs.tmpl"),
        artifact_path(GeneratedArtifactRustLib), diagnostics, GeneratedArtifactTier::Common,
        rust_lib_values(options.tier, system), common_artifact_path(GeneratedArtifactRustLib)
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Makefile.tmpl"),
        artifact_path(GeneratedArtifactMakefile), diagnostics, GeneratedArtifactTier::Common,
        rust_makefile_values(options.tier), common_artifact_path(GeneratedArtifactMakefile)
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
    const auto include_api_composition = !system.api_servers.empty();

    add_rust_generated_template_file(
        result, options, templates, "api/api_descriptors.rs", GeneratedArtifactTier::Api,
        diagnostics
    );
    add_rust_generated_template_file(
        result, options, templates, "api/api_codecs.rs", GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{{"api_codecs", generate_api_codecs_rs(system)}}
    );
    add_rust_generated_template_file(
        result, options, templates, "api/api_handlers.rs", GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_operation_handler_methods", generate_api_operation_handler_methods_rs(system)}
        }
    );
    add_rust_generated_template_file(
        result, options, templates, "api/api_handler_registry.rs", GeneratedArtifactTier::Api,
        diagnostics,
        TemplateRenderer::Values{
            {"api_operation_default_handler_methods",
             generate_api_operation_default_handler_methods_rs(system)}
        }
    );
    add_rust_generated_template_file(
        result, options, templates, "api/external_system_operator_metadata_api.rs",
        GeneratedArtifactTier::Api, diagnostics
    );
    if (include_api_composition)
    {
        add_rust_generated_template_file(
            result, options, templates, "api/api_application.rs", GeneratedArtifactTier::Api,
            diagnostics, rust_api_runtime_bootstrap_values(system)
        );
        add_rust_generated_template_file(
            result, options, templates, "api/api_process.rs", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_rust_generated_template_file(
            result, options, templates, "api/api_dispatcher.rs", GeneratedArtifactTier::Api,
            diagnostics,
            TemplateRenderer::Values{
                {"api_operation_dispatch_cases", generate_api_operation_dispatch_cases_rs(system)}
            }
        );
        add_rust_generated_template_file(
            result, options, templates, "api/api_server.rs", GeneratedArtifactTier::Api, diagnostics
        );
        add_rust_generated_template_file(
            result, options, templates, "api/api_transport.rs", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_rust_generated_template_file(
            result, options, templates, "api/api_routes.rs", GeneratedArtifactTier::Api, diagnostics
        );
        add_rust_generated_template_file(
            result, options, templates, "api/main.rs", GeneratedArtifactTier::Api, diagnostics
        );
    }
}

void add_rust_worker_artifacts(
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

    add_rust_generated_template_file(
        result, options, templates, "worker/worker_descriptors.rs", GeneratedArtifactTier::Worker,
        diagnostics
    );
    add_rust_generated_template_file(
        result, options, templates, "worker/worker_contexts.rs", GeneratedArtifactTier::Worker,
        diagnostics
    );
    add_rust_generated_template_file(
        result, options, templates, "worker/worker_registry.rs", GeneratedArtifactTier::Worker,
        diagnostics
    );
    if (include_worker_composition)
    {
        add_rust_generated_template_file(
            result, options, templates, "worker/worker_application.rs",
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_rust_generated_template_file(
            result, options, templates, "worker/worker_process.rs", GeneratedArtifactTier::Worker,
            diagnostics
        );
        add_rust_generated_template_file(
            result, options, templates, "worker/worker_runtime.rs", GeneratedArtifactTier::Worker,
            diagnostics, rust_runtime_bootstrap_values(system)
        );
        add_rust_generated_template_file(
            result, options, templates, "worker/main.rs", GeneratedArtifactTier::Worker, diagnostics
        );
    }
    if (include_worker_execution)
    {
        add_rust_generated_template_file(
            result, options, templates, "worker/workflow_step_handlers.rs",
            GeneratedArtifactTier::Worker, diagnostics,
            TemplateRenderer::Values{
                {"workflow_step_handler_methods",
                 generate_workflow_step_handler_methods_rs(system)},
                {"default_workflow_step_handler_methods",
                 generate_default_workflow_step_handler_methods_rs(system)},
                {"workflow_step_handler_keys", generate_workflow_step_handler_keys_rs(system)}
            }
        );
        add_rust_generated_template_file(
            result, options, templates, "worker/workflow_runner.rs", GeneratedArtifactTier::Worker,
            diagnostics,
            TemplateRenderer::Values{
                {"workflow_step_dispatch_cases", generate_workflow_step_dispatch_cases_rs(system)},
                {"workflow_step_next_cases", generate_workflow_step_next_cases_rs(system)}
            }
        );
    }
    if (usage.uses_queues)
    {
        add_rust_generated_template_file(
            result, options, templates, "worker/worker_queues.rs", GeneratedArtifactTier::Worker,
            diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_rust_generated_template_file(
            result, options, templates, "worker/worker_leases.rs", GeneratedArtifactTier::Worker,
            diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_rust_generated_template_file(
            result, options, templates, "worker/worker_workflows.rs", GeneratedArtifactTier::Worker,
            diagnostics
        );
    }
}

} // namespace statespec
