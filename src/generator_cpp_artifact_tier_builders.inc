void add_cpp_common_runtime_artifacts_impl(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto usage = runtime_domain_usage(system);

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
        result, options.output_dir, templates, "schema_compatibility.hpp",
        "schema_compatibility.hpp", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "workflow.hpp", "workflow.hpp", diagnostics
    );
    add_cpp_raw_common_file(result, options, "shape_types.hpp", cpp_shape_types_header());
    add_cpp_raw_common_file(
        result, options, "descriptors/types.hpp", cpp_descriptor_types_header(templates)
    );
    add_cpp_raw_common_file(
        result, options, "entity_repository.hpp",
        std::string{"#pragma once\n\n#include \"backend.hpp\"\n\n"
                    "namespace statespec_generated\n{\n\n"} +
            templates.load("generated/entity_repository.hpp.tmpl") +
            "\n} // namespace statespec_generated\n"
    );
    add_cpp_raw_common_file(
        result, options, "runtime_registration.hpp",
        std::string{"#pragma once\n\n#include \"descriptors.hpp\"\n\n"
                    "namespace statespec_generated\n{\n\n"} +
            cpp_runtime_registration_includes(system) + "\n" +
            generate_cpp_runtime_registration(system, templates) +
            "\n} // namespace statespec_generated\n"
    );
    add_template_file(
        result, options.output_dir, templates, "memory/transaction.hpp", "memory/transaction.hpp",
        diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "memory/backend.hpp", "memory/backend.hpp",
        diagnostics
    );
    if (usage.uses_any_runtime_domain)
    {
        add_cpp_common_generated_template_file(
            result, options, templates, "runtime/codec.hpp", diagnostics,
            cpp_runtime_codec_values(system)
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_core.hpp",
            "runtime/codec_core.hpp", diagnostics
        );
    }
    if (usage.uses_feature_flags)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_feature_flags.hpp",
            "runtime/codec_feature_flags.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/feature_flag_store.hpp",
            "runtime/feature_flag_store.hpp", diagnostics
        );
    }
    if (usage.uses_queues)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_queues.hpp",
            "runtime/codec_queues.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/queue_store.hpp",
            "runtime/queue_store.hpp", diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_leases.hpp",
            "runtime/codec_leases.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/lease_store.hpp",
            "runtime/lease_store.hpp", diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_workflows.hpp",
            "runtime/codec_workflows.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/workflow_store.hpp",
            "runtime/workflow_store.hpp", diagnostics
        );
    }
    if (usage.uses_observability)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_observability.hpp",
            "runtime/codec_observability.hpp", diagnostics
        );
    }
    if (usage.uses_logs)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_logs.hpp",
            "runtime/codec_logs.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/log_sink.hpp", "runtime/log_sink.hpp",
            diagnostics
        );
    }
    if (usage.uses_metrics)
    {
        add_template_file(
            result, options.output_dir, templates, "runtime/codec_metrics.hpp",
            "runtime/codec_metrics.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/metric_sink.hpp",
            "runtime/metric_sink.hpp", diagnostics
        );
    }
    if (usage.uses_entity_gc)
    {
        add_cpp_common_generated_template_file(
            result, options, templates, "runtime/entity_gc_types.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/entity_gc_repository.hpp",
            "runtime/entity_gc_repository.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/entity_gc_workers.hpp",
            "runtime/entity_gc_workers.hpp", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/entity_gc_registration.hpp",
            "runtime/entity_gc_registration.hpp", diagnostics
        );
    }

    if (diagnostics.has_errors())
    {
        return;
    }

    add_cpp_shape_type_artifacts(result, options, system);
    add_cpp_descriptor_module_artifacts(result, options, templates, system, diagnostics);
    add_cpp_workflow_descriptor_artifacts(result, options, system);
    if (diagnostics.has_errors())
    {
        return;
    }

    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("descriptors.hpp.tmpl"),
        common_artifact_path("descriptors.hpp"), diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{
            {"system_descriptors", generate_system_descriptors_header(system, templates)}
        }
    );
    add_cpp_package_artifacts(result, options, templates, system, diagnostics);
}

void add_cpp_api_artifacts_impl(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    if (system.apis.empty() && system.api_servers.empty())
    {
        return;
    }

    const auto include_api_composition = !system.api_servers.empty();

    add_cpp_api_shape_type_artifacts(result, options, templates, system);
    add_cpp_api_descriptor_artifacts(result, options, system);
    add_cpp_raw_api_file(
        result, options, "api/descriptors/catalog.hpp", cpp_api_descriptor_catalog_file(system)
    );
    for (const auto& api_server : system.api_servers)
    {
        add_cpp_raw_api_file(
            result, options, "api/servers/" + snake_identifier(api_server.name) + "/constants.hpp",
            cpp_api_server_constants_header(api_server)
        );
        for (const auto& domain : crud_api_handler_domains(api_handler_domains(system)))
        {
            const auto served = std::any_of(
                domain.apis.begin(), domain.apis.end(),
                [&](const IrApi& api)
                {
                    return std::find(
                               api_server.serves.begin(), api_server.serves.end(), api.name
                           ) != api_server.serves.end();
                }
            );
            if (!served)
            {
                continue;
            }
            add_cpp_raw_api_file(
                result, options,
                "api/servers/" + snake_identifier(api_server.name) + "/entities/" +
                    snake_identifier(domain.name) + ".hpp",
                cpp_api_server_entity_catalog_header(templates, api_server, domain)
            );
        }
        add_cpp_raw_api_file(
            result, options, "api/servers/" + snake_identifier(api_server.name) + "/catalog.hpp",
            cpp_api_server_catalog_header(templates, system, api_server)
        );
    }
    add_cpp_raw_api_file(
        result, options, "api/api_descriptors.hpp", cpp_api_descriptors_header(system)
    );
    add_cpp_generated_template_file(
        result, options, templates, "api/api_codecs.hpp", GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{{"api_codec_includes", cpp_api_codec_includes(system)}}
    );
    add_cpp_generated_template_file(
        result, options, templates, "api/api_codec_support.hpp", GeneratedArtifactTier::Api,
        diagnostics,
        TemplateRenderer::Values{
            {"api_codec_helpers", generate_api_codec_helpers()},
            {"api_shape_include",
             api_codec_shapes(system).empty() ? "" : "#include \"shapes.hpp\"\n"}
        }
    );
    for (const auto& shape : api_codec_shapes(system))
    {
        if (entity_api_shape_owner(system, shape.name).has_value())
        {
            continue;
        }
        add_cpp_raw_api_file(
            result, options, cpp_api_codec_shape_path(shape.name),
            cpp_api_codec_shape_file(system, shape)
        );
    }
    for (const auto& entity : system.entities)
    {
        if (entity_api_shapes(system, entity.name).empty())
        {
            continue;
        }
        add_cpp_raw_api_file(
            result, options, cpp_entity_api_codec_path(entity.name),
            cpp_entity_api_codec_file(system, entity)
        );
    }
    add_cpp_generated_template_file(
        result, options, templates, "api/api_handlers.hpp", GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"business_api_operation_handler_methods",
             generate_business_api_operation_handler_methods(system)}
        }
    );
    add_cpp_generated_template_file(
        result, options, templates, "api/api_handler_registry_support.hpp",
        GeneratedArtifactTier::Api, diagnostics
    );
    const auto handler_domains = crud_api_handler_domains(api_handler_domains(system));
    for (const auto& domain : handler_domains)
    {
        for (const auto& entity : system.entities)
        {
            if (entity.name != domain.name)
            {
                continue;
            }
            add_cpp_raw_api_file(
                result, options, cpp_entity_api_catalog_path(domain.name),
                cpp_entity_api_catalog_header(system, entity)
            );
            break;
        }
        add_cpp_raw_api_file(
            result, options, cpp_api_handler_domain_path(domain.name),
            cpp_api_handler_domain_file(system, domain)
        );
        add_cpp_raw_api_file(
            result, options, cpp_api_handler_domain_registry_path(domain.name),
            cpp_api_handler_domain_registry_file(domain)
        );
    }
    add_cpp_generated_template_file(
        result, options, templates, "api/api_handler_registry.hpp", GeneratedArtifactTier::Api,
        diagnostics,
        TemplateRenderer::Values{
            {"api_handler_registry_domain_includes",
             cpp_api_handler_dispatcher_domain_includes(handler_domains)},
            {"api_handler_lookup_registrations",
             cpp_api_handler_lookup_registrations(handler_domains)},
            {"api_business_handler_lookup_entries", cpp_business_api_handler_lookup_entries(system)}
        }
    );
    add_cpp_generated_template_file(
        result, options, templates, "api/external_system_operator_metadata_api.hpp",
        GeneratedArtifactTier::Api, diagnostics
    );
    if (include_api_composition)
    {
        add_cpp_generated_template_file(
            result, options, templates, "api/api_application.hpp", GeneratedArtifactTier::Api,
            diagnostics, cpp_api_runtime_values(system)
        );
        add_cpp_generated_template_file(
            result, options, templates, "api/api_process.hpp", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_cpp_generated_template_file(
            result, options, templates, "api/api_dispatcher.hpp", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_cpp_generated_template_file(
            result, options, templates, "api/api_server.hpp", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_cpp_generated_template_file(
            result, options, templates, "api/api_transport.hpp", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_cpp_generated_template_file(
            result, options, templates, "api/api_routes.hpp", GeneratedArtifactTier::Api,
            diagnostics
        );
        if (runtime_domain_usage(system).uses_entity_gc)
        {
            add_cpp_raw_api_file(
                result, options, "api/entity_gc_catalog.hpp",
                cpp_api_entity_gc_catalog_header(system)
            );
        }
        add_cpp_generated_template_file(
            result, options, templates, "api/main.cpp", GeneratedArtifactTier::Api, diagnostics,
            cpp_api_main_values(system)
        );
    }
}

void add_cpp_worker_artifacts_impl(
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
    if (!include_worker_execution)
    {
        return;
    }

    add_cpp_generated_template_file(
        result, options, templates, "worker/worker_descriptors.hpp", GeneratedArtifactTier::Worker,
        diagnostics
    );
    add_cpp_generated_template_file(
        result, options, templates, "worker/worker_contexts.hpp", GeneratedArtifactTier::Worker,
        diagnostics
    );
    add_cpp_raw_worker_file(
        result, options, "worker/descriptors/catalog.hpp",
        cpp_worker_descriptor_catalog_file(system)
    );
    for (const auto& worker : system.workers)
    {
        add_cpp_raw_worker_file(
            result, options, "worker/descriptors/" + snake_identifier(worker.name) + ".hpp",
            std::string{
                "#pragma once\n\n#include \"../../common/descriptors/types.hpp\"\n\nnamespace "
                "statespec_generated::worker::descriptors\n{\n\n"
            } + generate_cpp_worker_descriptor_module(worker) +
                "} // namespace statespec_generated::worker::descriptors\n"
        );
    }
    add_cpp_raw_worker_file(
        result, options, "worker/worker_registry.hpp", cpp_worker_registry_facade(system)
    );
    for (const auto& worker : system.workers)
    {
        add_cpp_raw_worker_file(
            result, options, "worker/registry/" + snake_identifier(worker.name) + ".hpp",
            cpp_worker_registry_module(worker)
        );
    }
    if (include_worker_composition)
    {
        add_cpp_generated_template_file(
            result, options, templates, "worker/worker_application.hpp",
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_cpp_generated_template_file(
            result, options, templates, "worker/worker_process.hpp", GeneratedArtifactTier::Worker,
            diagnostics
        );
        add_cpp_generated_template_file(
            result, options, templates, "worker/worker_runtime.hpp", GeneratedArtifactTier::Worker,
            diagnostics, cpp_runtime_bootstrap_values(system)
        );
        if (usage.uses_entity_gc)
        {
            add_cpp_raw_worker_file(
                result, options, "worker/entity_gc_catalog.hpp",
                cpp_worker_entity_gc_catalog_header(system)
            );
        }
        add_cpp_generated_template_file(
            result, options, templates, "worker/main.cpp", GeneratedArtifactTier::Worker,
            diagnostics, cpp_worker_main_values(system)
        );
    }
    if (include_worker_execution)
    {
        add_cpp_generated_template_file(
            result, options, templates, "worker/workflow_step_types.hpp",
            GeneratedArtifactTier::Worker, diagnostics
        );
        for (const auto& workflow : system.workflows)
        {
            add_cpp_raw_worker_file(
                result, options,
                "worker/workflows/" + snake_identifier(workflow.name) + "/handlers.hpp",
                generate_cpp_workflow_handler_module(templates, workflow)
            );
            add_cpp_raw_worker_file(
                result, options,
                "worker/workflows/" + snake_identifier(workflow.name) + "/registry.hpp",
                generate_cpp_workflow_registry_module(templates, workflow)
            );
        }
        add_cpp_generated_template_file(
            result, options, templates, "worker/workflow_step_handlers.hpp",
            GeneratedArtifactTier::Worker, diagnostics,
            TemplateRenderer::Values{
                {"workflow_step_handler_includes", cpp_workflow_step_handler_includes(system)},
                {"default_workflow_step_handler_members",
                 cpp_workflow_step_handler_members(system)},
                {"default_workflow_step_handler_setters",
                 cpp_workflow_step_handler_setters(system)},
                {"default_workflow_step_handler_lookup", cpp_workflow_step_handler_lookup(system)}
            }
        );
        add_cpp_generated_template_file(
            result, options, templates, "worker/workflow_runner.hpp", GeneratedArtifactTier::Worker,
            diagnostics, cpp_workflow_runner_values(system)
        );
    }
    if (usage.uses_queues)
    {
        add_cpp_generated_template_file(
            result, options, templates, "worker/worker_queues.hpp", GeneratedArtifactTier::Worker,
            diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_cpp_generated_template_file(
            result, options, templates, "worker/worker_leases.hpp", GeneratedArtifactTier::Worker,
            diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_cpp_generated_template_file(
            result, options, templates, "worker/worker_workflows.hpp",
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
}
