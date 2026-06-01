void add_go_common_runtime_artifacts_impl(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto usage = runtime_domain_usage(system);

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
        result, options.output_dir, templates, "backend/schema_compatibility.go",
        "backend/schema_compatibility.go", diagnostics
    );
    add_template_file(
        result, options.output_dir, templates, "backend/workflow.go", "backend/workflow.go",
        diagnostics
    );
    add_go_raw_common_file(
        result, options, "backend/entity_repository.go",
        std::string{"package backend\n\nimport (\n\t\"context\"\n\t\"strings\"\n)\n\n"} +
            templates.load("generated/entity_repository.go.tmpl")
    );
    add_go_raw_common_file(
        result, options, "backend/runtime_registration.go",
        std::string{"package backend\n\nimport \"context\"\n\n"} +
            generate_go_runtime_registration(system, templates)
    );
    add_go_raw_common_file(
        result, options, "backend/descriptortypes/types.go", go_descriptor_types_file()
    );
    add_go_raw_common_file(result, options, "backend/shape_types.go", go_shape_types_file());

    if (diagnostics.has_errors())
    {
        return;
    }

    add_go_common_generated_template_file(
        result, options, templates, "backend/memory/backend.go", diagnostics
    );
    add_go_common_generated_template_file(
        result, options, templates, "backend/memory/transaction.go", diagnostics
    );
    if (usage.uses_any_runtime_domain)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec.go", diagnostics
        );
    }
    if (usage.uses_feature_flags)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_feature_flags.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/feature_flags.go", diagnostics
        );
    }
    if (usage.uses_queues)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_queues.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/queues.go", diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_leases.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/leases.go", diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_workflows.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/workflows.go", diagnostics
        );
    }
    if (usage.uses_observability)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_observability.go", diagnostics
        );
    }
    if (usage.uses_logs)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_logs.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/logs.go", diagnostics
        );
    }
    if (usage.uses_metrics)
    {
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/codec_metrics.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/metrics.go", diagnostics
        );
    }
    if (usage.uses_entity_gc)
    {
        add_go_raw_common_file(
            result, options, "backend/runtime/entitygc/types.go",
            "package entitygc\n\n"
            "type EntityGCTerminalStateDescriptor struct {\n"
            "\tState string\n"
            "\tAfter string\n"
            "\tMode  string\n"
            "}\n\n"
            "type EntityGCDescriptor struct {\n"
            "\tEntity         string\n"
            "\tCollection     string\n"
            "\tStatusField    string\n"
            "\tTerminalStates []EntityGCTerminalStateDescriptor\n"
            "}\n"
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/entity_gc_types.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/entity_gc_repository.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/entity_gc_workers.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/entity_gc_registration.go", diagnostics
        );
    }
    add_go_descriptor_module_artifacts(result, options, templates, system, diagnostics);
    if (diagnostics.has_errors())
    {
        return;
    }
    add_go_shape_type_artifacts(result, options, system);
    add_go_entity_descriptor_artifacts(result, options, templates, system, diagnostics);
    add_go_workflow_descriptor_artifacts(result, options, system);
    if (diagnostics.has_errors())
    {
        return;
    }
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("descriptors.go.tmpl"),
        common_artifact_path("backend/descriptors.go"), diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{{"descriptors", generate_descriptors_go(system, templates)}}
    );
    add_go_package_artifacts(result, options, templates, system, diagnostics);
}

void add_go_api_artifacts_impl(
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

    add_go_api_shape_type_artifacts(result, options, templates, system);
    add_go_api_descriptor_artifacts(result, options, system);
    add_go_raw_api_file(
        result, options, "api/backend/descriptors/catalog.go",
        go_api_descriptor_catalog_file(system)
    );
    for (const auto& api_server : system.api_servers)
    {
        add_go_raw_api_file(
            result, options,
            "api/backend/servers/" + snake_identifier(api_server.name) + "/constants.go",
            go_api_server_constants_file(api_server)
        );
        for (const auto& domain : crud_api_handler_domains_go(api_handler_domains(system)))
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
            add_go_raw_api_file(
                result, options,
                "api/backend/servers/" + snake_identifier(api_server.name) + "/entities/" +
                    snake_identifier(domain.name) + "/catalog.go",
                go_api_server_entity_catalog_file(templates, api_server, domain)
            );
        }
        add_go_raw_api_file(
            result, options,
            "api/backend/servers/" + snake_identifier(api_server.name) + "/catalog.go",
            go_api_server_catalog_file(templates, system, api_server)
        );
    }
    add_go_raw_api_file(
        result, options, "api/backend/api_descriptors.go", go_api_descriptors_file(system)
    );
    add_go_generated_template_file(
        result, options, templates, "api/backend/api_codecs.go", GeneratedArtifactTier::Api,
        diagnostics,
        TemplateRenderer::Values{
            {"api_codec_helpers", generate_api_codec_helpers_go()},
            {"api_shape_import", go_api_shape_import(system)},
            {"api_codec_imports", go_api_codec_imports(system)},
            {"api_codec_delegates", go_api_codec_delegates(system)}
        }
    );
    add_go_raw_api_file(result, options, go_api_codec_support_path(), go_api_codec_support_file());
    for (const auto& shape : api_codec_shapes(system))
    {
        if (go_entity_api_shape_owner(system, shape.name).has_value())
        {
            continue;
        }
        add_go_raw_api_file(
            result, options, go_api_codec_shape_path(shape.name),
            go_api_codec_shape_file(system, shape)
        );
    }
    for (const auto& entity : system.entities)
    {
        if (go_entity_api_shapes(system, entity.name).empty())
        {
            continue;
        }
        add_go_raw_api_file(
            result, options, go_entity_api_codec_path(entity.name),
            go_entity_api_codec_file(system, entity)
        );
    }
    add_go_generated_template_file(
        result, options, templates, "api/backend/api_handlers.go", GeneratedArtifactTier::Api,
        diagnostics,
        TemplateRenderer::Values{
            {"business_api_operation_handler_methods",
             generate_business_api_operation_handler_methods_go(system)}
        }
    );
    const auto handler_domains = crud_api_handler_domains_go(api_handler_domains(system));
    for (const auto& domain : handler_domains)
    {
        for (const auto& entity : system.entities)
        {
            if (entity.name != domain.name)
            {
                continue;
            }
            add_go_raw_api_file(
                result, options, go_entity_api_catalog_path(domain.name),
                go_entity_api_catalog_file(system, entity)
            );
            break;
        }
        add_go_raw_api_file(
            result, options, go_api_handler_domain_path(domain.name),
            go_api_handler_domain_file(system, domain)
        );
        add_go_raw_api_file(
            result, options, go_api_handler_domain_registry_path(domain.name),
            go_api_handler_domain_registry_file(domain)
        );
    }
    add_go_generated_template_file(
        result, options, templates, "api/backend/api_handler_registry.go",
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_handler_lookup_registrations",
             go_api_handler_lookup_registrations(handler_domains)},
            {"api_business_handler_lookup_entries", go_business_api_handler_lookup_entries(system)},
            {"api_handler_registry_domain_imports",
             go_api_handler_registry_domain_imports(handler_domains)}
        }
    );
    add_go_generated_template_file(
        result, options, templates, "api/backend/external_system_operator_metadata_api.go",
        GeneratedArtifactTier::Api, diagnostics
    );
    if (include_api_composition)
    {
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_application.go",
            GeneratedArtifactTier::Api, diagnostics, go_api_runtime_bootstrap_values(system)
        );
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_process.go", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_transport.go", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_dispatcher.go", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_server.go", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_routes.go", GeneratedArtifactTier::Api,
            diagnostics
        );
        if (runtime_domain_usage(system).uses_entity_gc)
        {
            add_go_raw_api_file(
                result, options, "api/backend/entity_gc_catalog.go",
                go_api_entity_gc_catalog_file(system)
            );
        }
        add_go_generated_template_file(
            result, options, templates, "api/cmd/api/main.go", GeneratedArtifactTier::Api,
            diagnostics, go_api_main_values(system)
        );
    }
}

void add_go_worker_artifacts_impl(
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

    add_go_generated_template_file(
        result, options, templates, "worker/backend/worker_descriptors.go",
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_go_generated_template_file(
        result, options, templates, "worker/backend/worker_contexts.go",
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_go_raw_worker_file(
        result, options, "worker/backend/descriptors/catalog.go",
        go_worker_descriptor_catalog_file(system)
    );
    for (const auto& worker : system.workers)
    {
        add_go_raw_worker_file(
            result, options, "worker/backend/descriptors/" + snake_identifier(worker.name) + ".go",
            go_worker_descriptor_module_file(worker)
        );
    }
    add_go_raw_worker_file(
        result, options, "worker/backend/worker_registry.go", go_worker_registry_facade(system)
    );
    for (const auto& worker : system.workers)
    {
        add_go_raw_worker_file(
            result, options, "worker/backend/registry/" + snake_identifier(worker.name) + ".go",
            go_worker_registry_module(worker)
        );
    }
    if (include_worker_composition)
    {
        add_go_generated_template_file(
            result, options, templates, "worker/backend/worker_application.go",
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "worker/backend/worker_process.go",
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "worker/backend/worker_runtime.go",
            GeneratedArtifactTier::Worker, diagnostics, go_worker_runtime_bootstrap_values(system)
        );
        if (usage.uses_entity_gc)
        {
            add_go_raw_worker_file(
                result, options, "worker/backend/entity_gc_catalog.go",
                go_worker_entity_gc_catalog_file(system)
            );
        }
        add_go_generated_template_file(
            result, options, templates, "worker/cmd/worker/main.go", GeneratedArtifactTier::Worker,
            diagnostics, go_worker_main_values(system)
        );
    }
    if (include_worker_execution)
    {
        if (!system.workflows.empty())
        {
            add_go_raw_worker_file(
                result, options, "worker/backend/workflows/context/context.go",
                generate_go_workflow_context_module()
            );
            add_go_raw_worker_file(
                result, options, "worker/backend/workflows/workflows.go",
                generate_go_workflow_module_support()
            );
        }
        for (const auto& workflow : system.workflows)
        {
            add_go_raw_worker_file(
                result, options,
                "worker/backend/workflows/" + snake_identifier(workflow.name) + "/handlers.go",
                generate_go_workflow_handler_module(templates, workflow)
            );
            add_go_raw_worker_file(
                result, options,
                "worker/backend/workflows/" + snake_identifier(workflow.name) + "/registry.go",
                generate_go_workflow_registry_module(templates, workflow)
            );
        }
        add_go_generated_template_file(
            result, options, templates, "worker/backend/workflow_step_handlers.go",
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "worker/backend/workflow_runner.go",
            GeneratedArtifactTier::Worker, diagnostics, go_workflow_runner_values(system)
        );
    }
    if (usage.uses_queues)
    {
        add_go_generated_template_file(
            result, options, templates, "worker/backend/worker_queues.go",
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
    if (usage.uses_leases)
    {
        add_go_generated_template_file(
            result, options, templates, "worker/backend/worker_leases.go",
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
    if (usage.uses_workflows)
    {
        add_go_generated_template_file(
            result, options, templates, "worker/backend/worker_workflows.go",
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
}
