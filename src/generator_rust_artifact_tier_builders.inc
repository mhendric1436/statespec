void add_rust_common_runtime_artifacts_impl(
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
    add_rust_raw_common_file(result, options, "shape_types.rs", rust_shape_types_file());
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
            result, options, templates, "runtime/entity_gc_types.rs", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/entity_gc_repository.rs",
            "runtime/entity_gc_repository.rs", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/entity_gc_workers.rs",
            "runtime/entity_gc_workers.rs", diagnostics
        );
        add_template_file(
            result, options.output_dir, templates, "runtime/entity_gc_registration.rs",
            "runtime/entity_gc_registration.rs", diagnostics
        );
    }

    if (diagnostics.has_errors())
    {
        return;
    }

    add_rust_shape_type_artifacts(result, options, system);
    add_rust_descriptor_module_artifacts(result, options, templates, system, diagnostics);
    add_rust_workflow_descriptor_artifacts(result, options, system);
    if (diagnostics.has_errors())
    {
        return;
    }

    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("descriptors.rs.tmpl"),
        common_artifact_path("descriptors.rs"), diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{{"descriptors", generate_descriptors_rs(system, templates)}}
    );
    add_rust_package_artifacts(result, options, templates, system, diagnostics);
}

void add_rust_api_artifacts_impl(
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

    add_rust_api_shape_type_artifacts(result, options, templates, system);
    add_rust_api_descriptor_artifacts(result, options, system);
    add_rust_raw_api_file(
        result, options, "api/descriptors/catalog.rs", rust_api_descriptor_catalog_file(system)
    );
    for (const auto& api_server : system.api_servers)
    {
        add_rust_raw_api_file(
            result, options, "api/servers/" + snake_identifier(api_server.name) + "/constants.rs",
            rust_api_server_constants_file(api_server)
        );
        for (const auto& domain : crud_api_handler_domains_rs(api_handler_domains(system)))
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
            add_rust_raw_api_file(
                result, options,
                "api/servers/" + snake_identifier(api_server.name) + "/entities/" +
                    snake_identifier(domain.name) + ".rs",
                rust_api_server_entity_catalog_file(templates, api_server, domain)
            );
        }
        add_rust_raw_api_file(
            result, options, "api/servers/" + snake_identifier(api_server.name) + "/catalog.rs",
            rust_api_server_catalog_file(templates, system, api_server)
        );
    }
    add_rust_raw_api_file(
        result, options, "api/api_descriptors.rs", rust_api_descriptors_file(system)
    );
    add_rust_generated_template_file(
        result, options, templates, "api/api_codecs.rs", GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_codec_modules", rust_api_codec_modules(system, api_codec_shapes(system))},
            {"api_codec_helpers", generate_api_codec_helpers_rs()},
            {"api_shape_import", rust_api_shape_import(system)}
        }
    );
    for (const auto& shape : api_codec_shapes(system))
    {
        if (rust_entity_api_shape_owner(system, shape.name).has_value())
        {
            continue;
        }
        add_rust_raw_api_file(
            result, options, rust_api_codec_shape_path(shape.name),
            rust_api_codec_shape_file(system, shape)
        );
    }
    for (const auto& entity : system.entities)
    {
        if (rust_entity_api_shapes(system, entity.name).empty())
        {
            continue;
        }
        add_rust_raw_api_file(
            result, options, rust_entity_api_codec_path(entity.name),
            rust_entity_api_codec_file(system, entity)
        );
    }
    add_rust_generated_template_file(
        result, options, templates, "api/api_handlers.rs", GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"business_api_operation_handler_methods",
             generate_business_api_operation_handler_methods_rs(system)}
        }
    );
    const auto handler_domains = crud_api_handler_domains_rs(api_handler_domains(system));
    for (const auto& domain : handler_domains)
    {
        for (const auto& entity : system.entities)
        {
            if (entity.name != domain.name)
            {
                continue;
            }
            add_rust_raw_api_file(
                result, options, rust_entity_api_catalog_path(domain.name),
                rust_entity_api_catalog_file(system, entity)
            );
            break;
        }
        add_rust_raw_api_file(
            result, options, rust_api_handler_domain_path(domain.name),
            rust_api_handler_domain_file(system, domain)
        );
        add_rust_raw_api_file(
            result, options, rust_api_handler_domain_registry_path(domain.name),
            rust_api_handler_domain_registry_file(domain)
        );
    }
    add_rust_generated_template_file(
        result, options, templates, "api/api_handler_registry.rs", GeneratedArtifactTier::Api,
        diagnostics,
        TemplateRenderer::Values{
            {"api_handler_lookup_registrations",
             rust_api_handler_lookup_registrations(handler_domains)},
            {"api_business_handler_lookup_entries",
             rust_business_api_handler_lookup_entries(system)},
            {"api_handler_registry_domain_modules",
             rust_api_handler_registry_domain_modules(handler_domains)},
            {"api_shape_import", rust_api_shape_import(system)}
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
            diagnostics
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
        if (runtime_domain_usage(system).uses_entity_gc)
        {
            add_rust_raw_api_file(
                result, options, "api/entity_gc_catalog.rs", rust_api_entity_gc_catalog_file(system)
            );
        }
        add_rust_generated_template_file(
            result, options, templates, "api/main.rs", GeneratedArtifactTier::Api, diagnostics,
            rust_api_main_values(system)
        );
    }
}

void add_rust_worker_artifacts_impl(
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

    add_rust_generated_template_file(
        result, options, templates, "worker/worker_descriptors.rs", GeneratedArtifactTier::Worker,
        diagnostics
    );
    add_rust_generated_template_file(
        result, options, templates, "worker/worker_contexts.rs", GeneratedArtifactTier::Worker,
        diagnostics
    );
    add_rust_raw_worker_file(
        result, options, "worker/descriptors/catalog.rs",
        rust_worker_descriptor_catalog_file(system)
    );
    for (const auto& worker : system.workers)
    {
        add_rust_raw_worker_file(
            result, options, "worker/descriptors/" + snake_identifier(worker.name) + ".rs",
            generate_rust_worker_descriptor_module(worker)
        );
    }
    add_rust_raw_worker_file(
        result, options, "worker/worker_registry.rs", rust_worker_registry_facade(system)
    );
    for (const auto& worker : system.workers)
    {
        add_rust_raw_worker_file(
            result, options, "worker/registry/" + snake_identifier(worker.name) + ".rs",
            rust_worker_registry_module(worker)
        );
    }
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
        if (usage.uses_entity_gc)
        {
            add_rust_raw_worker_file(
                result, options, "worker/entity_gc_catalog.rs",
                rust_worker_entity_gc_catalog_file(system)
            );
        }
        add_rust_generated_template_file(
            result, options, templates, "worker/main.rs", GeneratedArtifactTier::Worker,
            diagnostics, rust_worker_main_values(system)
        );
    }
    if (include_worker_execution)
    {
        for (const auto& workflow : system.workflows)
        {
            add_rust_raw_worker_file(
                result, options,
                "worker/workflows/" + snake_identifier(workflow.name) + "/handlers.rs",
                generate_rust_workflow_handler_module(templates, workflow)
            );
            add_rust_raw_worker_file(
                result, options,
                "worker/workflows/" + snake_identifier(workflow.name) + "/registry.rs",
                generate_rust_workflow_registry_module(templates, workflow)
            );
        }
        add_rust_generated_template_file(
            result, options, templates, "worker/workflow_step_handlers.rs",
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_rust_generated_template_file(
            result, options, templates, "worker/workflow_runner.rs", GeneratedArtifactTier::Worker,
            diagnostics, rust_workflow_runner_values(system)
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
