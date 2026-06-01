void add_java_common_runtime_artifacts_impl(
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
            template_path_for_output(output_root / "runtime" / "EntityGcTypes.java"),
            common_artifact_path((output_root / "runtime" / "EntityGcTypes.java").generic_string()),
            diagnostics, GeneratedArtifactTier::Common
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
        add_template_file(
            result, options.output_dir, templates,
            output_root / "runtime" / "EntityGcRegistration.java",
            output_root / "runtime" / "EntityGcRegistration.java", diagnostics
        );
    }
    add_java_descriptor_module_artifacts(result, options, templates, system, diagnostics);
    if (diagnostics.has_errors())
    {
        return;
    }
    add_java_shape_type_artifacts(result, options, system);
    add_java_workflow_descriptor_artifacts(result, options, system);
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Descriptors.java.tmpl"),
        common_artifact_path(
            join_artifact_path(GeneratedJavaOutputPackagePath, "Descriptors.java").generic_string()
        ),
        diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{{"descriptors", generate_descriptors_java(system, templates)}}
    );
    add_java_package_artifacts(result, options, templates, system, diagnostics);
}

void add_java_api_artifacts_impl(
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

    add_java_api_shape_type_artifacts(result, options, templates, system);
    add_java_api_descriptor_artifacts(result, options, system);
    for (const auto& api_server : system.api_servers)
    {
        add_java_raw_api_file(
            result, options,
            "api/com/statespec/generated/servers/" + snake_identifier(api_server.name) +
                "/Constants.java",
            java_api_server_constants_file(api_server)
        );
        for (const auto& domain : crud_api_handler_domains_java(api_handler_domains(system)))
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
            add_java_raw_api_file(
                result, options,
                "api/com/statespec/generated/servers/" + snake_identifier(api_server.name) +
                    "/entities/" + snake_identifier(domain.name) + "/Catalog.java",
                java_api_server_entity_catalog_file(templates, api_server, domain)
            );
        }
        add_java_raw_api_file(
            result, options,
            "api/com/statespec/generated/servers/" + snake_identifier(api_server.name) +
                "/Catalog.java",
            java_api_server_catalog_file(templates, system, api_server)
        );
    }
    add_java_raw_api_file(
        result, options, java_api_generated_path("ApiDescriptors.java"),
        java_api_descriptors_file(system)
    );
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiCodecs.java"),
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_codec_imports", java_api_codec_imports(system)},
            {"api_shape_import", java_api_shape_import(system)},
            {"api_codec_helpers", generate_api_codec_helpers_java()},
            {"api_codec_delegates", java_api_codec_delegates(system)}
        }
    );
    for (const auto& shape : api_codec_shapes(system))
    {
        if (java_entity_api_shape_owner(system, shape.name).has_value())
        {
            continue;
        }
        add_java_raw_api_file(
            result, options, java_api_codec_shape_path(shape.name),
            java_api_codec_shape_file(system, shape)
        );
    }
    for (const auto& entity : system.entities)
    {
        if (java_entity_api_shapes(system, entity.name).empty())
        {
            continue;
        }
        add_java_raw_api_file(
            result, options,
            "api/com/statespec/generated/entities/" + snake_identifier(entity.name) +
                "/ApiConstants.java",
            java_entity_api_constants_file(system, entity)
        );
        add_java_raw_api_file(
            result, options, java_entity_api_codec_path(entity.name),
            java_entity_api_codec_file(system, entity)
        );
    }
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiHandlers.java"),
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"business_api_operation_handler_methods",
             generate_business_api_operation_handler_methods_java(system)}
        }
    );
    const auto handler_domains = crud_api_handler_domains_java(api_handler_domains(system));
    for (const auto& domain : handler_domains)
    {
        for (const auto& entity : system.entities)
        {
            if (entity.name != domain.name)
            {
                continue;
            }
            add_java_raw_api_file(
                result, options, java_entity_api_catalog_path(domain.name),
                java_entity_api_catalog_file(system, entity)
            );
            break;
        }
        add_java_raw_api_file(
            result, options, java_api_handler_domain_path(domain.name),
            java_api_handler_domain_file(system, domain)
        );
        add_java_raw_api_file(
            result, options, java_api_handler_domain_registry_path(domain.name),
            java_api_handler_domain_registry_file(domain)
        );
    }
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiHandlerRegistry.java"),
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_handler_registry_domain_imports",
             java_api_handler_registry_domain_imports(handler_domains)},
            {"api_handler_lookup_registrations",
             java_api_handler_lookup_registrations(handler_domains)},
            {"api_business_handler_lookup_entries",
             java_business_api_handler_lookup_entries(system)}
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
            GeneratedArtifactTier::Api, diagnostics
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
        if (runtime_domain_usage(system).uses_entity_gc)
        {
            add_java_raw_api_file(
                result, options, java_api_generated_path("EntityGcCatalog.java"),
                java_api_entity_gc_catalog_file(system)
            );
        }
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiMain.java"),
            GeneratedArtifactTier::Api, diagnostics, java_api_main_values(system)
        );
    }
}

void add_java_worker_artifacts_impl(
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

    add_java_generated_template_file(
        result, options, templates, java_worker_generated_path("WorkerDescriptors.java"),
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_java_generated_template_file(
        result, options, templates, java_worker_generated_path("WorkerContexts.java"),
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_java_raw_worker_file(
        result, options, java_worker_generated_path("worker/descriptors/Catalog.java"),
        java_worker_descriptor_catalog_file(system)
    );
    for (const auto& worker : system.workers)
    {
        const auto class_name = java_worker_descriptor_module_class_name(worker.name);
        add_java_raw_worker_file(
            result, options,
            java_worker_generated_path("worker/descriptors/" + class_name + ".java"),
            generate_java_worker_descriptor_module(
                worker, "com.statespec.generated.worker.descriptors", class_name
            )
        );
    }
    add_java_raw_worker_file(
        result, options, java_worker_generated_path("WorkerRegistry.java"),
        java_worker_registry_facade_file(system)
    );
    for (const auto& worker : system.workers)
    {
        add_java_raw_worker_file(
            result, options,
            java_worker_generated_path(
                "registry/" + pascal_identifier(worker.name) + "Registry.java"
            ),
            java_worker_registry_module_file(
                worker, "com.statespec.generated.registry",
                pascal_identifier(worker.name) + "Registry"
            )
        );
    }
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
        if (usage.uses_entity_gc)
        {
            add_java_raw_worker_file(
                result, options, java_worker_generated_path("WorkerEntityGcCatalog.java"),
                java_worker_entity_gc_catalog_file(system)
            );
        }
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerMain.java"),
            GeneratedArtifactTier::Worker, diagnostics, java_worker_main_values(system)
        );
    }
    if (include_worker_execution)
    {
        for (const auto& workflow : system.workflows)
        {
            add_java_raw_worker_file(
                result, options,
                java_worker_generated_path(
                    "workflows/" + snake_identifier(workflow.name) + "/Handlers.java"
                ),
                generate_java_workflow_handler_module(templates, workflow)
            );
            add_java_raw_worker_file(
                result, options,
                java_worker_generated_path(
                    "workflows/" + snake_identifier(workflow.name) + "/Registry.java"
                ),
                generate_java_workflow_registry_module(templates, workflow)
            );
        }
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkflowStepHandlers.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkflowRunner.java"),
            GeneratedArtifactTier::Worker, diagnostics, java_workflow_runner_values(system)
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
