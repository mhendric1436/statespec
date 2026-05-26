#include "generator_java_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_java_descriptor_areas.hpp"
#include "generator_java_descriptor_support.hpp"
#include "generator_java_descriptors.hpp"
#include "generator_support.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace statespec
{
namespace
{

std::string java_api_default_handler_shape_import(const IrSystem& system);
std::string java_api_shape_import(const IrSystem& system);
std::string java_workflow_worker_module_class_name(const IrWorkflow& workflow);
std::filesystem::path java_worker_generated_path(std::string_view filename);

std::string java_makefile_source_list(const std::vector<std::string>& sources)
{
    if (sources.empty())
    {
        return {};
    }

    std::ostringstream out;
    for (const auto& source : sources)
    {
        out << " \\\n  " << source;
    }
    return out.str();
}

std::string java_entity_descriptor_module_class_name(std::string_view entity_name)
{
    return pascal_identifier(std::string{entity_name}) + "DescriptorModule";
}

std::string java_workflow_descriptor_module_class_name(std::string_view workflow_name)
{
    return pascal_identifier(std::string{workflow_name}) + "DescriptorModule";
}

std::string java_shape_descriptor_module_class_name(std::string_view shape_name)
{
    return pascal_identifier(std::string{shape_name}) + "DescriptorModule";
}

std::string java_entity_descriptor_module_ref(std::string_view entity_name)
{
    return "com.statespec.generated.descriptors.entities." +
           java_entity_descriptor_module_class_name(entity_name);
}

std::string java_event_descriptor_module_file(const IrSystem& system)
{
    std::ostringstream out;
    out << "package com.statespec.generated.descriptors;\n\n";
    out << "import com.statespec.backend.Json;\n";
    out << "import com.statespec.generated.Descriptors.EventEnvelope;\n";
    out << "import java.util.Map;\n\n";
    out << "public final class EventDescriptorModule {\n";
    out << "    private EventDescriptorModule() {}\n\n";
    for (const auto& event : system.events)
    {
        out << "    public static EventEnvelope build" << pascal_identifier(event.name)
            << "Event(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "        Json " << lower_camel_identifier(field.name);
            out << (i + 1 < event.fields.size() ? "," : "") << "\n";
        }
        out << "    ) {\n";
        out << "        return new EventEnvelope(\n";
        out << "            " << java_string(event.name) << ",\n";
        out << "            Map.of(\n";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            out << "                " << java_string(field.name) << ", "
                << lower_camel_identifier(field.name);
            out << (i + 1 < event.fields.size() ? "," : "") << "\n";
        }
        out << "            )\n";
        out << "        );\n";
        out << "    }\n\n";
    }
    out << "}\n";
    return out.str();
}

std::vector<std::string> java_descriptor_module_sources(const IrSystem& system)
{
    std::vector<std::string> sources{
        "common/com/statespec/generated/descriptors/CoreDescriptorModule.java",
        "common/com/statespec/generated/descriptors/EventDescriptorModule.java",
        "common/com/statespec/generated/descriptors/ShapeDescriptorModule.java",
        "common/com/statespec/generated/descriptors/ApiDescriptorModule.java",
        "common/com/statespec/generated/descriptors/WorkerDescriptorModule.java",
        "common/com/statespec/generated/descriptors/RuntimeDescriptorModule.java",
    };
    for (const auto& shape : system.shapes)
    {
        sources.push_back(
            "common/com/statespec/generated/descriptors/shapes/" +
            java_shape_descriptor_module_class_name(shape.name) + ".java"
        );
    }
    for (const auto& entity : system.entities)
    {
        sources.push_back(
            "common/com/statespec/generated/descriptors/entities/" +
            java_entity_descriptor_module_class_name(entity.name) + ".java"
        );
    }
    return sources;
}

std::vector<std::string> java_shape_sources(const IrSystem& system)
{
    std::vector<std::string> sources;
    for (const auto& shape : system.shapes)
    {
        sources.push_back(
            "common/com/statespec/generated/shapes/" + pascal_identifier(shape.name) + ".java"
        );
    }
    return sources;
}

std::vector<std::string> java_workflow_descriptor_sources(const IrSystem& system)
{
    std::vector<std::string> sources;
    for (const auto& workflow : system.workflows)
    {
        sources.push_back(
            "common/com/statespec/generated/workflows/" +
            java_workflow_descriptor_module_class_name(workflow.name) + ".java"
        );
    }
    return sources;
}

std::string java_api_descriptor_module_class_name(std::string_view api_name)
{
    return pascal_identifier(std::string{api_name}) + "DescriptorModule";
}

std::vector<std::string> java_api_descriptor_sources(const IrSystem& system)
{
    std::vector<std::string> sources;
    for (const auto& api : system.apis)
    {
        sources.push_back(
            "api/com/statespec/generated/descriptors/" +
            java_api_descriptor_module_class_name(api.name) + ".java"
        );
    }
    return sources;
}

struct ApiHandlerDomain
{
    std::string name;
    std::vector<IrApi> apis;
};

bool api_output_mentions_entity(
    const IrApi& api,
    const IrEntity& entity
)
{
    return api.output.has_value() && api.output->find(entity.name) != std::string::npos;
}

bool api_input_mentions_entity(
    const IrApi& api,
    const IrEntity& entity
)
{
    return api.input.has_value() && api.input->find(entity.name) != std::string::npos;
}

bool api_name_mentions_entity(
    const IrApi& api,
    const IrEntity& entity
)
{
    return api.name.find(entity.name) != std::string::npos;
}

std::vector<ApiHandlerDomain> api_handler_domains(const IrSystem& system)
{
    std::vector<ApiHandlerDomain> domains;
    for (const auto& entity : system.entities)
    {
        domains.push_back(ApiHandlerDomain{entity.name, {}});
    }
    ApiHandlerDomain operations{"Operations", {}};
    for (const auto& api : system.apis)
    {
        bool assigned = false;
        auto assign_matching_entity = [&](auto matches)
        {
            for (std::size_t i = 0; i < system.entities.size(); ++i)
            {
                if (matches(api, system.entities[i]))
                {
                    domains[i].apis.push_back(api);
                    assigned = true;
                    return;
                }
            }
        };
        assign_matching_entity(api_output_mentions_entity);
        if (!assigned)
        {
            assign_matching_entity(api_input_mentions_entity);
        }
        if (!assigned)
        {
            assign_matching_entity(api_name_mentions_entity);
        }
        if (!assigned)
        {
            operations.apis.push_back(api);
        }
    }
    domains.erase(
        std::remove_if(
            domains.begin(), domains.end(), [](const auto& domain) { return domain.apis.empty(); }
        ),
        domains.end()
    );
    if (!operations.apis.empty())
    {
        domains.push_back(std::move(operations));
    }
    return domains;
}

IrSystem with_domain_apis(
    const IrSystem& system,
    const std::vector<IrApi>& apis
)
{
    auto filtered = system;
    filtered.apis = apis;
    return filtered;
}

std::string java_api_handler_domain_class_name(std::string_view domain_name)
{
    return "ApiHandlerRegistry" + pascal_identifier(std::string{domain_name});
}

std::filesystem::path java_api_handler_domain_path(std::string_view domain_name)
{
    return std::filesystem::path{"api/com/statespec/generated"} /
           (java_api_handler_domain_class_name(domain_name) + ".java");
}

std::vector<std::string> java_api_handler_domain_sources(const IrSystem& system)
{
    std::vector<std::string> sources;
    for (const auto& domain : api_handler_domains(system))
    {
        sources.push_back(java_api_handler_domain_path(domain.name).generic_string());
    }
    return sources;
}

std::string java_api_handler_registry_delegates(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        const auto class_name = java_api_handler_domain_class_name(domain.name);
        for (const auto& api : domain.apis)
        {
            out << "        @Override\n";
            out << "        public Descriptors.ApiResponse handle" << pascal_identifier(api.name)
                << "(Descriptors.ApiRequestContext context) throws Exception {\n";
            out << "            return new " << class_name << "(backend).handle"
                << pascal_identifier(api.name) << "(context);\n";
            out << "        }\n\n";
        }
    }
    return out.str();
}

std::string java_api_handler_domain_file(
    const IrSystem& system,
    const ApiHandlerDomain& domain
)
{
    const auto filtered = with_domain_apis(system, domain.apis);
    std::ostringstream out;
    out << "package com.statespec.generated;\n\n";
    out << "import static com.statespec.generated.ApiHandlerRegistry.*;\n";
    out << "import com.statespec.generated.descriptors.entities.*;\n";
    out << java_api_default_handler_shape_import(filtered);
    out << "import java.util.Optional;\n\n";
    out << "final class " << java_api_handler_domain_class_name(domain.name) << " {\n";
    out << "    private final com.statespec.backend.Backend backend;\n\n";
    out << "    " << java_api_handler_domain_class_name(domain.name)
        << "(com.statespec.backend.Backend backend) {\n";
    out << "        this.backend = backend;\n";
    out << "    }\n\n";
    out << generate_api_operation_default_handler_domain_methods_java(filtered);
    out << "}\n";
    return out.str();
}

std::vector<IrShape> api_codec_shapes(const IrSystem& system)
{
    std::vector<IrShape> shapes;
    for (const auto& shape : system.shapes)
    {
        const auto used = std::any_of(
            system.apis.begin(), system.apis.end(),
            [&](const auto& api)
            {
                return (api.input.has_value() && *api.input == shape.name) ||
                       (api.output.has_value() && *api.output == shape.name);
            }
        );
        if (used)
        {
            shapes.push_back(shape);
        }
    }
    return shapes;
}

IrSystem with_codec_shape_apis(
    const IrSystem& system,
    std::string_view shape_name
)
{
    auto filtered = system;
    filtered.apis.clear();
    for (const auto& api : system.apis)
    {
        IrApi scoped = api;
        if (!scoped.input.has_value() || *scoped.input != shape_name)
        {
            scoped.input.reset();
        }
        if (!scoped.output.has_value() || *scoped.output != shape_name)
        {
            scoped.output.reset();
        }
        if (scoped.input.has_value() || scoped.output.has_value())
        {
            filtered.apis.push_back(std::move(scoped));
        }
    }
    return filtered;
}

std::string java_api_codec_shape_class_name(std::string_view shape_name)
{
    return "ApiCodecs" + pascal_identifier(std::string{shape_name});
}

std::filesystem::path java_api_codec_shape_path(std::string_view shape_name)
{
    return std::filesystem::path{"api/com/statespec/generated/codecs"} /
           (java_api_codec_shape_class_name(shape_name) + ".java");
}

std::vector<std::string> java_api_codec_shape_sources(const IrSystem& system)
{
    std::vector<std::string> sources;
    for (const auto& shape : api_codec_shapes(system))
    {
        sources.push_back(java_api_codec_shape_path(shape.name).generic_string());
    }
    return sources;
}

std::string java_api_codec_shape_file(
    const IrSystem& system,
    const IrShape& shape
)
{
    const auto filtered = with_codec_shape_apis(system, shape.name);
    std::ostringstream out;
    out << "package com.statespec.generated.codecs;\n\n";
    out << "import static com.statespec.generated.ApiCodecs.*;\n";
    out << "import com.statespec.backend.Json;\n";
    out << "import com.statespec.generated.Descriptors;\n";
    out << java_api_shape_import(filtered);
    out << "import java.util.Map;\n";
    out << "import java.util.Optional;\n";
    out << "import java.util.TreeMap;\n\n";
    out << "public final class " << java_api_codec_shape_class_name(shape.name) << " {\n";
    out << "    private " << java_api_codec_shape_class_name(shape.name) << "() {}\n\n";
    out << generate_api_codec_operations_java(filtered);
    out << "}\n";
    return out.str();
}

std::string java_api_codec_delegates(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& shape : api_codec_shapes(system))
    {
        const auto filtered = with_codec_shape_apis(system, shape.name);
        const auto class_name = java_api_codec_shape_class_name(shape.name);
        for (const auto& api : filtered.apis)
        {
            if (api.input.has_value())
            {
                out << "    public static " << pascal_identifier(*api.input) << " decode"
                    << pascal_identifier(api.name)
                    << "Request(Descriptors.ApiRequestContext request) {\n";
                out << "        return " << class_name << ".decode" << pascal_identifier(api.name)
                    << "Request(request);\n";
                out << "    }\n\n";
            }
            if (api.output.has_value())
            {
                out << "    public static " << pascal_identifier(*api.output) << " decode"
                    << pascal_identifier(api.name)
                    << "Response(Descriptors.ApiResponse response) {\n";
                out << "        return " << class_name << ".decode" << pascal_identifier(api.name)
                    << "Response(response);\n";
                out << "    }\n\n";
                out << "    public static Descriptors.ApiResponse encode"
                    << pascal_identifier(api.name) << "Response(" << pascal_identifier(*api.output)
                    << " response) {\n";
                out << "        return " << class_name << ".encode" << pascal_identifier(api.name)
                    << "Response(response);\n";
                out << "    }\n\n";
                out << "    public static Descriptors.ApiResponse encode"
                    << pascal_identifier(api.name) << "Response(" << pascal_identifier(*api.output)
                    << " response, int statusCode) {\n";
                out << "        return " << class_name << ".encode" << pascal_identifier(api.name)
                    << "Response(response, statusCode);\n";
                out << "    }\n\n";
            }
        }
    }
    return out.str();
}

TemplateRenderer::Values java_makefile_values(
    BindingGenerationTier tier,
    const IrSystem& system
)
{
    const auto usage = runtime_domain_usage(system);
    const auto include_api =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api;
    const auto include_worker =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker;
    const auto include_api_composition = include_api && !system.api_servers.empty();
    const auto include_worker_composition = include_worker && !system.workers.empty();
    const auto include_worker_execution =
        include_worker && (include_worker_composition || usage.uses_workflows);

    std::vector<std::string> common_sources{
        "common/com/statespec/backend/Json.java",
        "common/com/statespec/backend/Backend.java",
        "common/com/statespec/backend/ExternalSystem.java",
        "common/com/statespec/backend/FeatureFlag.java",
        "common/com/statespec/backend/Lease.java",
        "common/com/statespec/backend/Log.java",
        "common/com/statespec/backend/Metric.java",
        "common/com/statespec/backend/Queue.java",
        "common/com/statespec/backend/SchemaCompatibility.java",
        "common/com/statespec/backend/Workflow.java",
        "common/com/statespec/backend/memory/InMemoryBackend.java",
        "common/com/statespec/backend/memory/InMemoryTransaction.java",
        "common/com/statespec/generated/Descriptors.java",
    };
    for (const auto& source : java_descriptor_module_sources(system))
    {
        common_sources.push_back(source);
    }
    for (const auto& source : java_shape_sources(system))
    {
        common_sources.push_back(source);
    }
    for (const auto& source : java_workflow_descriptor_sources(system))
    {
        common_sources.push_back(source);
    }
    auto add_common = [&](bool used, std::string source)
    {
        if (used)
        {
            common_sources.push_back(std::move(source));
        }
    };
    add_common(usage.uses_any_runtime_domain, "common/com/statespec/backend/runtime/Codec.java");
    add_common(
        usage.uses_feature_flags, "common/com/statespec/backend/runtime/FeatureFlagCodec.java"
    );
    add_common(
        usage.uses_feature_flags, "common/com/statespec/backend/runtime/FeatureFlagStore.java"
    );
    add_common(usage.uses_queues, "common/com/statespec/backend/runtime/QueueCodec.java");
    add_common(usage.uses_queues, "common/com/statespec/backend/runtime/QueueStore.java");
    add_common(usage.uses_leases, "common/com/statespec/backend/runtime/LeaseCodec.java");
    add_common(usage.uses_leases, "common/com/statespec/backend/runtime/LeaseStore.java");
    add_common(usage.uses_workflows, "common/com/statespec/backend/runtime/WorkflowCodec.java");
    add_common(usage.uses_workflows, "common/com/statespec/backend/runtime/WorkflowStore.java");
    add_common(
        usage.uses_observability, "common/com/statespec/backend/runtime/ObservabilityCodec.java"
    );
    add_common(usage.uses_logs, "common/com/statespec/backend/runtime/LogCodec.java");
    add_common(usage.uses_logs, "common/com/statespec/backend/runtime/LogSink.java");
    add_common(usage.uses_metrics, "common/com/statespec/backend/runtime/MetricCodec.java");
    add_common(usage.uses_metrics, "common/com/statespec/backend/runtime/MetricSink.java");
    add_common(
        usage.uses_entity_gc, "common/com/statespec/backend/runtime/EntityGcDescriptors.java"
    );
    add_common(
        usage.uses_entity_gc, "common/com/statespec/backend/runtime/EntityGcRepository.java"
    );
    add_common(usage.uses_entity_gc, "common/com/statespec/backend/runtime/EntityGcWorkers.java");

    std::vector<std::string> api_sources;
    if (include_api)
    {
        api_sources = {
            "api/com/statespec/generated/ApiDescriptors.java",
            "api/com/statespec/generated/ApiCodecs.java",
            "api/com/statespec/generated/ApiHandlers.java",
            "api/com/statespec/generated/ApiHandlerRegistry.java",
            "api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java",
        };
        for (const auto& source : java_api_descriptor_sources(system))
        {
            api_sources.push_back(source);
        }
        for (const auto& source : java_api_codec_shape_sources(system))
        {
            api_sources.push_back(source);
        }
        for (const auto& source : java_api_handler_domain_sources(system))
        {
            api_sources.push_back(source);
        }
        if (include_api_composition)
        {
            api_sources.push_back("api/com/statespec/generated/ApiApplication.java");
            api_sources.push_back("api/com/statespec/generated/ApiProcess.java");
            api_sources.push_back("api/com/statespec/generated/ApiDispatcher.java");
            api_sources.push_back("api/com/statespec/generated/ApiServer.java");
            api_sources.push_back("api/com/statespec/generated/ApiTransport.java");
            api_sources.push_back("api/com/statespec/generated/ApiRoutes.java");
            api_sources.push_back("api/com/statespec/generated/ApiMain.java");
        }
    }

    std::vector<std::string> worker_sources;
    if (include_worker)
    {
        worker_sources = {
            "worker/com/statespec/generated/WorkerDescriptors.java",
            "worker/com/statespec/generated/WorkerContexts.java",
            "worker/com/statespec/generated/WorkerRegistry.java",
        };
        if (include_worker_composition)
        {
            worker_sources.push_back("worker/com/statespec/generated/WorkerApplication.java");
            worker_sources.push_back("worker/com/statespec/generated/WorkerProcess.java");
            worker_sources.push_back("worker/com/statespec/generated/WorkerRuntime.java");
            worker_sources.push_back("worker/com/statespec/generated/WorkerMain.java");
        }
        if (include_worker_execution)
        {
            for (const auto& workflow : system.workflows)
            {
                worker_sources.push_back(java_worker_generated_path(
                    "workflows/" + java_workflow_worker_module_class_name(workflow) + ".java"
                ));
            }
            worker_sources.push_back("worker/com/statespec/generated/WorkflowStepHandlers.java");
            worker_sources.push_back("worker/com/statespec/generated/WorkflowRunner.java");
        }
        if (usage.uses_queues)
        {
            worker_sources.push_back("worker/com/statespec/generated/WorkerQueues.java");
        }
        if (usage.uses_leases)
        {
            worker_sources.push_back("worker/com/statespec/generated/WorkerLeases.java");
        }
        if (usage.uses_workflows)
        {
            worker_sources.push_back("worker/com/statespec/generated/WorkerWorkflows.java");
        }
    }

    std::ostringstream build_target_additions;
    std::ostringstream package_target_additions;
    std::ostringstream phony_targets;
    std::ostringstream api_rules;
    std::ostringstream worker_rules;
    if (include_api)
    {
        build_target_additions << " build-api";
        package_target_additions << " package-api";
        phony_targets << " build-api package-api";
        api_rules << "build-api: $(BUILD_DIR)\n";
        api_rules << "\t$(JAVAC) -d $(BUILD_DIR) $(COMMON_SOURCES) "
                     "$(COMMON_EXTENSION_SOURCES) $(API_SOURCES) $(API_EXTENSION_SOURCES)\n\n";
        api_rules << "package-api: build-api $(DIST_DIR)\n";
        api_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-api-java.tgz common api "
                     "Makefile\n\n";
    }
    if (include_worker)
    {
        build_target_additions << " build-worker";
        package_target_additions << " package-worker";
        phony_targets << " build-worker package-worker";
        worker_rules << "build-worker: $(BUILD_DIR)\n";
        worker_rules << "\t$(JAVAC) -d $(BUILD_DIR) $(COMMON_SOURCES) "
                        "$(COMMON_EXTENSION_SOURCES) $(WORKER_SOURCES) "
                        "$(WORKER_EXTENSION_SOURCES)\n\n";
        worker_rules << "package-worker: build-worker $(DIST_DIR)\n";
        worker_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-worker-java.tgz common worker "
                        "Makefile\n\n";
    }

    return TemplateRenderer::Values{
        {"common_sources", java_makefile_source_list(common_sources)},
        {"api_sources", java_makefile_source_list(api_sources)},
        {"worker_sources", java_makefile_source_list(worker_sources)},
        {"build_target_additions", build_target_additions.str()},
        {"package_target_additions", package_target_additions.str()},
        {"phony_targets", phony_targets.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
    };
}

TemplateRenderer::Values java_runtime_bootstrap_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream imports;
    std::ostringstream fields;
    std::ostringstream initializers;
    std::ostringstream arguments;
    std::ostringstream worker_imports;
    std::ostringstream worker_run_once;

    auto add = [&](bool used, std::string_view type, std::string_view field)
    {
        if (!used)
        {
            return;
        }
        imports << "import com.statespec.backend.runtime." << type << ";\n";
        fields << "    private final " << type << " " << field << ";\n";
        initializers << "        this." << field << " = new " << type << "();\n";
        arguments << ", " << field;
    };
    add(usage.uses_feature_flags, "FeatureFlagStore", "featureFlags");
    add(usage.uses_queues, "QueueStore", "queues");
    add(usage.uses_leases, "LeaseStore", "leases");
    add(usage.uses_workflows, "WorkflowStore", "workflows");
    add(usage.uses_logs, "LogSink", "logs");
    add(usage.uses_metrics, "MetricSink", "metrics");
    if (usage.uses_workflows)
    {
        worker_imports << "import java.time.Duration;\n";
        worker_imports << "import java.util.Optional;\n";
        worker_run_once
            << "    public Optional<com.statespec.backend.Workflow.WorkflowExecutionRecord> "
               "runOnce(\n"
            << "        Descriptors.WorkerContext context,\n"
            << "        WorkflowStepHandlers.Handler handler,\n"
            << "        String workflowExecutionId\n"
            << "    ) throws Exception {\n"
            << "        if (context.executes().isEmpty()) {\n"
            << "            return Optional.empty();\n"
            << "        }\n"
            << "        WorkflowRunner runner =\n"
            << "            new WorkflowRunner(backend, workflows, handler, context.workerName(), "
               "Duration.ofSeconds(30), 3);\n"
            << "        return runner.runOnce(workflowExecutionId, "
               "context.executes().orElseThrow(), "
               "1);\n"
            << "    }\n";
    }
    else
    {
        worker_imports << "import java.util.Optional;\n";
        worker_run_once
            << "    public Optional<com.statespec.backend.Workflow.WorkflowExecutionRecord> "
               "runOnce(\n"
            << "        Descriptors.WorkerContext context,\n"
            << "        WorkflowStepHandlers.Handler handler,\n"
            << "        String workflowExecutionId\n"
            << "    ) {\n"
            << "        return Optional.empty();\n"
            << "    }\n";
    }
    return TemplateRenderer::Values{
        {"runtime_store_imports", imports.str()},
        {"runtime_store_fields", fields.str()},
        {"runtime_store_initializers", initializers.str()},
        {"runtime_bootstrap_arguments", arguments.str()},
        {"worker_runtime_imports", worker_imports.str()},
        {"worker_runtime_run_once", worker_run_once.str()},
    };
}

TemplateRenderer::Values java_api_runtime_bootstrap_values(const IrSystem& system)
{
    auto values = java_runtime_bootstrap_values(system);
    values.erase("worker_runtime_imports");
    values.erase("worker_runtime_run_once");
    return values;
}

std::string java_workflow_worker_module_class_name(const IrWorkflow& workflow)
{
    return pascal_identifier(workflow.name) + "WorkerModule";
}

std::string generate_java_workflow_worker_module(const IrWorkflow& workflow)
{
    const auto class_name = java_workflow_worker_module_class_name(workflow);
    std::ostringstream out;
    out << "package com.statespec.generated.workflows;\n\n";
    out << "import com.statespec.generated.WorkflowStepHandlers;\n";
    out << "import com.statespec.backend.Workflow;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class " << class_name << " {\n";
    out << "    private " << class_name << "() {}\n\n";
    out << "    public static boolean dispatchStep(\n";
    out << "        WorkflowStepHandlers.Handler handler,\n";
    out << "        WorkflowStepHandlers.Context context,\n";
    out << "        Workflow.WorkflowExecutionRecord record\n";
    out << "    ) throws Exception {\n";
    out << "        if (!record.workflowName().equals(" << java_string(workflow.name) << ")) {\n";
    out << "            return false;\n";
    out << "        }\n";
    for (const auto& step : workflow.steps)
    {
        out << "        if (record.currentStep().equals(" << java_string(step.name) << ")) {\n";
        out << "            handler.handle" << pascal_identifier(workflow.name + "_" + step.name)
            << "(context);\n";
        out << "            return true;\n";
        out << "        }\n";
    }
    out << "        return false;\n";
    out << "    }\n\n";
    out << "    public static Optional<String> nextStep(Workflow.WorkflowExecutionRecord record) "
           "{\n";
    out << "        if (!record.workflowName().equals(" << java_string(workflow.name) << ")) {\n";
    out << "            return Optional.empty();\n";
    out << "        }\n";
    for (const auto& step : workflow.steps)
    {
        for (const auto& statement : step.statements)
        {
            if (statement.kind != "transition_to" || !statement.target.has_value())
            {
                continue;
            }
            out << "        if (record.currentStep().equals(" << java_string(step.name) << ")) {\n";
            out << "            return Optional.of(" << java_string(*statement.target) << ");\n";
            out << "        }\n";
        }
    }
    out << "        return Optional.empty();\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

TemplateRenderer::Values java_workflow_runner_values(const IrSystem& system)
{
    std::ostringstream imports;
    std::ostringstream dispatch_cases;
    std::ostringstream next_cases;
    for (const auto& workflow : system.workflows)
    {
        const auto class_name = java_workflow_worker_module_class_name(workflow);
        imports << "import com.statespec.generated.workflows." << class_name << ";\n";
        dispatch_cases << "            if (!handled) {\n";
        dispatch_cases << "                handled = " << class_name
                       << ".dispatchStep(handler, context, record);\n";
        dispatch_cases << "            }\n";
        next_cases << "            if (nextStep.isEmpty()) {\n";
        next_cases << "                nextStep = " << class_name << ".nextStep(record);\n";
        next_cases << "            }\n";
    }
    return TemplateRenderer::Values{
        {"workflow_step_module_imports", imports.str()},
        {"workflow_step_module_dispatch_cases", dispatch_cases.str()},
        {"workflow_step_module_next_cases", next_cases.str()},
    };
}

TemplateRenderer::Values java_entity_gc_descriptor_values(const IrSystem& system)
{
    std::vector<std::string> descriptor_entries;
    std::ostringstream descriptors;
    for (const auto& entity : system.entities)
    {
        std::ostringstream terminal_states;
        bool first_terminal_state = true;
        for (const auto& state : entity.states)
        {
            if (!state.garbage_collection.has_value())
            {
                continue;
            }
            if (!first_terminal_state)
            {
                terminal_states << ",\n";
            }
            const auto module_ref = java_entity_descriptor_module_ref(entity.name);
            terminal_states << "                new TerminalState(" << module_ref << "."
                            << java_entity_state_constant_name(entity.name, state.name) << ", "
                            << java_string(state.garbage_collection->after) << ", "
                            << java_string(state.garbage_collection->mode) << ")";
            first_terminal_state = false;
        }
        if (terminal_states.str().empty())
        {
            continue;
        }
        std::ostringstream descriptor;
        descriptor << "            new Descriptor(\n"
                   << "                " << java_entity_descriptor_module_ref(entity.name) << "."
                   << java_entity_name_constant_name(entity.name) << ",\n"
                   << "                " << java_entity_descriptor_module_ref(entity.name) << "."
                   << java_entity_name_constant_name(entity.name) << ",\n"
                   << "                List.of(\n"
                   << terminal_states.str() << "\n"
                   << "                )\n"
                   << "            )";
        descriptor_entries.push_back(descriptor.str());
    }
    for (std::size_t i = 0; i < descriptor_entries.size(); ++i)
    {
        descriptors << descriptor_entries[i];
        if (i + 1 < descriptor_entries.size())
        {
            descriptors << ",";
        }
        descriptors << "\n";
    }
    return TemplateRenderer::Values{{"entity_gc_descriptors", descriptors.str()}};
}

std::filesystem::path java_api_generated_path(std::string_view filename)
{
    return api_artifact_path(
        join_artifact_path(GeneratedJavaOutputPackagePath, filename).generic_string()
    );
}

std::filesystem::path java_worker_generated_path(std::string_view filename)
{
    return worker_artifact_path(
        join_artifact_path(GeneratedJavaOutputPackagePath, filename).generic_string()
    );
}

void add_java_generated_template_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const std::filesystem::path& relative_output_path,
    GeneratedArtifactTier tier,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {}
)
{
    add_generated_template_file(
        result, options.output_dir, templates, template_path_for_output(relative_output_path),
        relative_output_path, diagnostics, tier, values
    );
}

TemplateRenderer::Values java_descriptor_module_values(
    std::string_view package_name,
    std::string_view class_name,
    std::string_view module_name,
    std::string content = {}
)
{
    return TemplateRenderer::Values{
        {"descriptor_module_package", std::string{package_name}},
        {"descriptor_module_class", std::string{class_name}},
        {"descriptor_module_name", std::string{module_name}},
        {"descriptor_module_content", std::move(content)},
    };
}

TemplateRenderer::Values java_entity_descriptor_module_values(const IrEntity& entity)
{
    IrSystem one_entity_system;
    one_entity_system.entities.push_back(entity);
    auto content = generate_java_entity_descriptors(one_entity_system);
    return java_descriptor_module_values(
        "com.statespec.generated.descriptors.entities",
        java_entity_descriptor_module_class_name(entity.name), "entity descriptor " + entity.name,
        std::move(content)
    );
}

TemplateRenderer::Values java_shape_descriptor_module_values(
    std::string_view package_name,
    std::string_view class_name,
    const IrSystem& system
)
{
    std::ostringstream content;
    content << "    public static List<ShapeDescriptor> shapeDescriptors() {\n";
    if (system.shapes.empty())
    {
        content << "        return List.of();\n";
        content << "    }\n\n";
        return java_descriptor_module_values(
            package_name, class_name, "shape descriptors", content.str()
        );
    }
    content << "        return List.of(\n";
    for (std::size_t i = 0; i < system.shapes.size(); ++i)
    {
        const auto& shape = system.shapes[i];
        content << "            com.statespec.generated.descriptors.shapes."
                << java_shape_descriptor_module_class_name(shape.name) << ".shapeDescriptors()";
        content << (i + 1 < system.shapes.size() ? ",\n" : "\n");
    }
    content << "        ).stream().flatMap(List::stream).toList();\n";
    content << "    }\n\n";
    return java_descriptor_module_values(
        package_name, class_name, "shape descriptors", content.str()
    );
}

TemplateRenderer::Values java_shape_descriptor_module_values(const IrShape& shape)
{
    IrSystem one_shape_system;
    one_shape_system.shapes.push_back(shape);
    return java_descriptor_module_values(
        "com.statespec.generated.descriptors.shapes",
        java_shape_descriptor_module_class_name(shape.name), "shape descriptor " + shape.name,
        generate_java_shape_descriptors(one_shape_system)
    );
}

void add_java_raw_common_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const std::filesystem::path& relative_output_path,
    std::string content
)
{
    const auto relative_path = common_artifact_path(relative_output_path.generic_string());
    result.files.push_back(
        GeneratedFile{
            (options.output_dir / relative_path).string(),
            std::move(content),
            GeneratedArtifactTier::Common,
            relative_path.generic_string(),
        }
    );
}

std::string java_shape_type_file(const IrShape& shape)
{
    std::ostringstream out;
    out << "package com.statespec.generated.shapes;\n\n";
    out << "import com.statespec.backend.Json;\n";
    out << "import java.util.Optional;\n\n";
    out << "public record " << pascal_identifier(shape.name) << "(\n";
    for (std::size_t i = 0; i < shape.fields.size(); ++i)
    {
        const auto& field = shape.fields[i];
        out << "    " << java_shape_type(field.type) << " " << field.name;
        out << (i + 1 < shape.fields.size() ? "," : "") << "\n";
    }
    out << ") {}\n";
    return out.str();
}

void add_java_shape_type_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    const auto shape_path = join_artifact_path(GeneratedJavaOutputPackagePath, "shapes");
    for (const auto& shape : system.shapes)
    {
        add_java_raw_common_file(
            result, options, shape_path / (pascal_identifier(shape.name) + ".java"),
            java_shape_type_file(shape)
        );
    }
}

void add_java_workflow_descriptor_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    const auto workflow_path = join_artifact_path(GeneratedJavaOutputPackagePath, "workflows");
    const std::string workflow_package = "com.statespec.generated.workflows";
    for (const auto& workflow : system.workflows)
    {
        const auto class_name = java_workflow_descriptor_module_class_name(workflow.name);
        add_java_raw_common_file(
            result, options, workflow_path / (class_name + ".java"),
            generate_java_workflow_descriptor(workflow, workflow_package, class_name)
        );
    }
}

std::string java_api_shape_import(const IrSystem& system)
{
    for (const auto& api : system.apis)
    {
        if (api.input.has_value() || api.output.has_value())
        {
            return "import com.statespec.generated.shapes.*;\n";
        }
    }
    return {};
}

std::string java_api_default_handler_shape_import(const IrSystem& system)
{
    for (const auto& api : system.apis)
    {
        if (api.output.has_value())
        {
            return "import com.statespec.generated.shapes.*;\n";
        }
    }
    return {};
}

void add_java_raw_api_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const std::filesystem::path& relative_output_path,
    std::string content
)
{
    const auto relative_path = artifact_path(relative_output_path.generic_string());
    result.files.push_back(
        GeneratedFile{
            (options.output_dir / relative_path).string(),
            std::move(content),
            GeneratedArtifactTier::Api,
            relative_path.generic_string(),
        }
    );
}

void add_java_raw_worker_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const std::filesystem::path& relative_output_path,
    std::string content
)
{
    const auto relative_path = artifact_path(relative_output_path.generic_string());
    result.files.push_back(
        GeneratedFile{
            (options.output_dir / relative_path).string(),
            std::move(content),
            GeneratedArtifactTier::Worker,
            relative_path.generic_string(),
        }
    );
}

bool java_api_server_serves(
    const IrApiServer& api_server,
    const std::string& api_name
)
{
    for (const auto& served_api : api_server.serves)
    {
        if (served_api == api_name)
        {
            return true;
        }
    }
    return false;
}

std::string java_api_descriptor_module(
    const IrSystem& system,
    const IrApi& api
)
{
    std::ostringstream out;
    out << "package com.statespec.generated.descriptors;\n\n";
    out << "import com.statespec.generated.Descriptors;\n";
    out << "import java.util.List;\n";
    out << "import java.util.Optional;\n\n";
    out << "public final class " << java_api_descriptor_module_class_name(api.name) << " {\n";
    out << "    private " << java_api_descriptor_module_class_name(api.name) << "() {}\n\n";
    out << "    public static List<Descriptors.ApiDescriptor> apiDescriptors() {\n";
    out << "        return List.of(\n";
    out << "            new Descriptors.ApiDescriptor(\n";
    out << "                " << java_string(api.name) << ",\n";
    out << "                " << java_optional_string_expr(api.method) << ",\n";
    out << "                " << java_optional_string_expr(api.path) << ",\n";
    out << "                " << java_optional_string_expr(api.input) << ",\n";
    out << "                " << java_optional_string_expr(api.output) << ",\n";
    out << "                " << java_optional_string_expr(api.error) << ",\n";
    out << "                " << java_optional_string_expr(api.starts_workflow) << ",\n";
    out << "                " << java_optional_string_expr(api.enqueues) << "\n";
    out << "            )\n";
    out << "        );\n";
    out << "    }\n\n";
    out << "    public static List<Descriptors.ApiRouteDescriptor> apiRouteDescriptors() {\n";
    out << "        return List.of(\n";
    bool first_route = true;
    for (const auto& api_server : system.api_servers)
    {
        if (!java_api_server_serves(api_server, api.name))
        {
            continue;
        }
        if (!first_route)
        {
            out << ",\n";
        }
        first_route = false;
        out << "            new Descriptors.ApiRouteDescriptor(\n";
        out << "                " << java_string(api_server.name + "." + api.name) << ",\n";
        out << "                " << java_string(api_server.name) << ",\n";
        out << "                " << java_string(api.name) << ",\n";
        out << "                " << java_optional_string_expr(api.method) << ",\n";
        out << "                " << java_optional_string_expr(api.path) << ",\n";
        out << "                " << java_optional_string_expr(api.input) << ",\n";
        out << "                " << java_optional_string_expr(api.output) << ",\n";
        out << "                " << java_optional_string_expr(api.error) << "\n";
        out << "            )";
    }
    out << "\n";
    out << "        );\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

void add_java_api_descriptor_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    const auto descriptor_path = java_api_generated_path("descriptors");
    for (const auto& api : system.apis)
    {
        add_java_raw_api_file(
            result, options,
            descriptor_path / (java_api_descriptor_module_class_name(api.name) + ".java"),
            java_api_descriptor_module(system, api)
        );
    }
}

TemplateRenderer::Values java_api_descriptor_values(const IrSystem& system)
{
    std::ostringstream api_aggregation;
    std::ostringstream route_aggregation;
    std::ostringstream server_descriptors;
    for (const auto& api : system.apis)
    {
        const auto class_name = java_api_descriptor_module_class_name(api.name);
        api_aggregation << "        descriptors.addAll(" << class_name << ".apiDescriptors());\n";
        route_aggregation << "        descriptors.addAll(" << class_name
                          << ".apiRouteDescriptors());\n";
    }
    server_descriptors << "        return List.of(\n";
    for (std::size_t server_index = 0; server_index < system.api_servers.size(); ++server_index)
    {
        const auto& api_server = system.api_servers[server_index];
        server_descriptors << "            new Descriptors.ApiServerDescriptor(\n";
        server_descriptors << "                " << java_string(api_server.name) << ",\n";
        server_descriptors << "                List.of(";
        for (std::size_t i = 0; i < api_server.serves.size(); ++i)
        {
            if (i > 0)
            {
                server_descriptors << ", ";
            }
            server_descriptors << java_string(api_server.serves[i]);
        }
        server_descriptors << "),\n";
        server_descriptors << "                " << api_server.concurrency.value_or(1) << "\n";
        server_descriptors << "            )"
                           << (server_index + 1 < system.api_servers.size() ? "," : "") << "\n";
    }
    server_descriptors << "        );\n";
    return TemplateRenderer::Values{
        {"api_descriptor_aggregation", api_aggregation.str()},
        {"api_route_descriptor_aggregation", route_aggregation.str()},
        {"api_server_descriptors", server_descriptors.str()},
    };
}

void add_java_descriptor_module_artifact(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const std::filesystem::path& relative_output_path,
    std::string_view package_name,
    std::string_view class_name,
    std::string_view module_name,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {}
)
{
    auto resolved_values =
        values.empty() ? java_descriptor_module_values(package_name, class_name, module_name)
                       : values;
    add_generated_template_file(
        result, options.output_dir, templates,
        generated_template_path("DescriptorModule.java.tmpl"),
        common_artifact_path(relative_output_path.generic_string()), diagnostics,
        GeneratedArtifactTier::Common, resolved_values
    );
}

void add_java_descriptor_module_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto descriptor_package_path =
        join_artifact_path(GeneratedJavaOutputPackagePath, "descriptors");
    const auto entity_descriptor_package_path = descriptor_package_path / "entities";
    const auto shape_descriptor_package_path = descriptor_package_path / "shapes";
    const std::string descriptor_package = "com.statespec.generated.descriptors";
    const std::string entity_descriptor_package = descriptor_package + ".entities";

    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "CoreDescriptorModule.java",
        descriptor_package, "CoreDescriptorModule", "core descriptors", diagnostics
    );
    add_java_raw_common_file(
        result, options, descriptor_package_path / "EventDescriptorModule.java",
        java_event_descriptor_module_file(system)
    );
    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "ShapeDescriptorModule.java",
        descriptor_package, "ShapeDescriptorModule", "shape descriptors", diagnostics,
        java_shape_descriptor_module_values(descriptor_package, "ShapeDescriptorModule", system)
    );
    for (const auto& shape : system.shapes)
    {
        const auto class_name = java_shape_descriptor_module_class_name(shape.name);
        add_generated_template_file(
            result, options.output_dir, templates,
            generated_template_path("DescriptorModule.java.tmpl"),
            common_artifact_path(
                (shape_descriptor_package_path / (class_name + ".java")).generic_string()
            ),
            diagnostics, GeneratedArtifactTier::Common, java_shape_descriptor_module_values(shape)
        );
    }
    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "ApiDescriptorModule.java",
        descriptor_package, "ApiDescriptorModule", "API descriptors", diagnostics
    );
    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "WorkerDescriptorModule.java",
        descriptor_package, "WorkerDescriptorModule", "worker descriptors", diagnostics
    );
    add_java_descriptor_module_artifact(
        result, options, templates, descriptor_package_path / "RuntimeDescriptorModule.java",
        descriptor_package, "RuntimeDescriptorModule", "runtime descriptors", diagnostics
    );
    for (const auto& entity : system.entities)
    {
        const auto class_name = java_entity_descriptor_module_class_name(entity.name);
        add_generated_template_file(
            result, options.output_dir, templates,
            generated_template_path("DescriptorModule.java.tmpl"),
            common_artifact_path(
                (entity_descriptor_package_path / (class_name + ".java")).generic_string()
            ),
            diagnostics, GeneratedArtifactTier::Common, java_entity_descriptor_module_values(entity)
        );
    }
}

} // namespace

void add_java_common_runtime_artifacts(
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
            template_path_for_output(output_root / "runtime" / "EntityGcDescriptors.java"),
            common_artifact_path(
                (output_root / "runtime" / "EntityGcDescriptors.java").generic_string()
            ),
            diagnostics, GeneratedArtifactTier::Common, java_entity_gc_descriptor_values(system)
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
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Makefile.tmpl"),
        artifact_path(GeneratedArtifactMakefile), diagnostics, GeneratedArtifactTier::Common,
        java_makefile_values(options.tier, system), common_artifact_path(GeneratedArtifactMakefile)
    );
}

void add_java_api_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    const auto include_api_composition = !system.api_servers.empty();

    add_java_api_descriptor_artifacts(result, options, system);
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiDescriptors.java"),
        GeneratedArtifactTier::Api, diagnostics, java_api_descriptor_values(system)
    );
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiCodecs.java"),
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_shape_import", java_api_shape_import(system)},
            {"api_codec_helpers", generate_api_codec_helpers_java()},
            {"api_codec_delegates", java_api_codec_delegates(system)}
        }
    );
    for (const auto& shape : api_codec_shapes(system))
    {
        add_java_raw_api_file(
            result, options, java_api_codec_shape_path(shape.name),
            java_api_codec_shape_file(system, shape)
        );
    }
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiHandlers.java"),
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_operation_handler_methods", generate_api_operation_handler_methods_java(system)}
        }
    );
    const auto handler_domains = api_handler_domains(system);
    for (const auto& domain : handler_domains)
    {
        add_java_raw_api_file(
            result, options, java_api_handler_domain_path(domain.name),
            java_api_handler_domain_file(system, domain)
        );
    }
    add_java_generated_template_file(
        result, options, templates, java_api_generated_path("ApiHandlerRegistry.java"),
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_operation_default_handler_methods",
             java_api_handler_registry_delegates(handler_domains)},
            {"api_shape_import", {}}
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
            GeneratedArtifactTier::Api, diagnostics,
            TemplateRenderer::Values{
                {"api_operation_dispatch_cases", generate_api_operation_dispatch_cases_java(system)}
            }
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
        add_java_generated_template_file(
            result, options, templates, java_api_generated_path("ApiMain.java"),
            GeneratedArtifactTier::Api, diagnostics
        );
    }
}

void add_java_worker_artifacts(
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

    add_java_generated_template_file(
        result, options, templates, java_worker_generated_path("WorkerDescriptors.java"),
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_java_generated_template_file(
        result, options, templates, java_worker_generated_path("WorkerContexts.java"),
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_java_generated_template_file(
        result, options, templates, java_worker_generated_path("WorkerRegistry.java"),
        GeneratedArtifactTier::Worker, diagnostics
    );
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
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkerMain.java"),
            GeneratedArtifactTier::Worker, diagnostics
        );
    }
    if (include_worker_execution)
    {
        for (const auto& workflow : system.workflows)
        {
            add_java_raw_worker_file(
                result, options,
                java_worker_generated_path(
                    "workflows/" + java_workflow_worker_module_class_name(workflow) + ".java"
                ),
                generate_java_workflow_worker_module(workflow)
            );
        }
        add_java_generated_template_file(
            result, options, templates, java_worker_generated_path("WorkflowStepHandlers.java"),
            GeneratedArtifactTier::Worker, diagnostics,
            TemplateRenderer::Values{
                {"workflow_step_handler_methods",
                 generate_workflow_step_handler_methods_java(system)},
                {"default_workflow_step_handler_methods",
                 generate_default_workflow_step_handler_methods_java(system)},
                {"workflow_step_handler_keys", generate_workflow_step_handler_keys_java(system)}
            }
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

} // namespace statespec
