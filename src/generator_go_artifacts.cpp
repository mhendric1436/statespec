#include "generator_go_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_go_descriptor_areas.hpp"
#include "generator_go_descriptor_support.hpp"
#include "generator_go_descriptors.hpp"
#include "generator_support.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"
#include "type_syntax.hpp"

#include <algorithm>
#include <sstream>
#include <utility>
#include <vector>

namespace statespec
{

namespace
{

TemplateRenderer::Values go_makefile_values(
    BindingGenerationTier tier,
    const IrSystem& system
)
{
    const auto include_api =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api;
    const auto include_worker =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker;
    const auto include_api_composition = include_api && !system.api_servers.empty();
    const auto include_worker_composition = include_worker && !system.workers.empty();

    std::ostringstream target_additions;
    std::ostringstream phony_targets;
    std::ostringstream api_package_additions;
    std::ostringstream worker_package_additions;
    std::ostringstream api_build_dependency;
    std::ostringstream worker_build_dependency;
    std::ostringstream api_build_command;
    std::ostringstream worker_build_command;
    std::ostringstream api_rules;
    std::ostringstream worker_rules;
    if (include_api_composition)
    {
        api_package_additions << " ./api/cmd/api";
        api_build_dependency << " $(BIN_DIR)";
        api_build_command << "\t$(GO) build -o $(BIN_DIR)/statespec-api ./api/cmd/api\n";
    }
    if (include_worker_composition)
    {
        worker_package_additions << " ./worker/cmd/worker";
        worker_build_dependency << " $(BIN_DIR)";
        worker_build_command
            << "\t$(GO) build -o $(BIN_DIR)/statespec-worker ./worker/cmd/worker\n";
    }
    if (include_api)
    {
        if (!include_api_composition)
        {
            api_build_command << "\t$(GO) build $(API_PACKAGES)\n";
        }
        target_additions << "\nCHECK_TARGETS += check-api";
        target_additions << "\nBUILD_TARGETS += build-api";
        target_additions << "\nPACKAGE_TARGETS += package-api";
        phony_targets << " check-api build-api package-api";
        api_rules << "check-api:\n";
        api_rules << "\t$(GO) test $(API_PACKAGES)\n\n";
        api_rules << "build-api:" << api_build_dependency.str() << "\n";
        api_rules << "\t$(GO) build ./api/backend\n";
        api_rules << api_build_command.str();
        api_rules << "\n";
        api_rules << "package-api: build-api $(DIST_DIR)\n";
        api_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-api-go.tgz common api go.mod "
                     "Makefile\n\n";
    }
    if (include_worker)
    {
        if (!include_worker_composition)
        {
            worker_build_command << "\t$(GO) build $(WORKER_PACKAGES)\n";
        }
        target_additions << "\nCHECK_TARGETS += check-worker";
        target_additions << "\nBUILD_TARGETS += build-worker";
        target_additions << "\nPACKAGE_TARGETS += package-worker";
        phony_targets << " check-worker build-worker package-worker";
        worker_rules << "check-worker:\n";
        worker_rules << "\t$(GO) test $(WORKER_PACKAGES)\n\n";
        worker_rules << "build-worker:" << worker_build_dependency.str() << "\n";
        worker_rules << "\t$(GO) build ./worker/backend\n";
        worker_rules << worker_build_command.str();
        worker_rules << "\n";
        worker_rules << "package-worker: build-worker $(DIST_DIR)\n";
        worker_rules << "\ttar -czf $(DIST_DIR)/statespec-generated-worker-go.tgz common worker "
                        "go.mod Makefile\n\n";
    }
    return TemplateRenderer::Values{
        {"target_additions", target_additions.str()},
        {"phony_targets", phony_targets.str()},
        {"api_package_additions", api_package_additions.str()},
        {"worker_package_additions", worker_package_additions.str()},
        {"api_rules", api_rules.str()},
        {"worker_rules", worker_rules.str()},
    };
}

TemplateRenderer::Values go_runtime_bootstrap_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream runtime_import;
    std::ostringstream fields;
    std::ostringstream initializers;
    std::ostringstream arguments;
    std::ostringstream worker_run_once;
    std::ostringstream worker_time_import;

    auto add = [&](bool used, std::string_view field, std::string_view type, std::string_view ctor)
    {
        if (!used)
        {
            return;
        }
        fields << "\t" << field << " *runtime." << type << "\n";
        initializers << "\t\t" << field << ": runtime." << ctor << "(),\n";
        arguments << ", app." << field;
    };
    if (usage.uses_any_runtime_domain)
    {
        runtime_import << "\truntime \"statespec-generated/common/backend/runtime\"\n";
    }
    add(usage.uses_feature_flags, "FeatureFlags", "FeatureFlagStore", "NewFeatureFlagStore");
    add(usage.uses_queues, "Queues", "QueueStore", "NewQueueStore");
    add(usage.uses_leases, "Leases", "LeaseStore", "NewLeaseStore");
    add(usage.uses_workflows, "Workflows", "WorkflowStore", "NewWorkflowStore");
    add(usage.uses_logs, "Logs", "LogSink", "NewLogSink");
    add(usage.uses_metrics, "Metrics", "MetricSink", "NewMetricSink");
    worker_time_import << "\t\"time\"\n";
    if (usage.uses_workflows)
    {
        worker_run_once
            << "func (runtime *WorkerTierRuntime) RunOnce(ctx context.Context, workerContext "
               "common.WorkerContext, handler WorkflowStepHandler, workflowExecutionID string) "
               "(*common.WorkflowExecutionRecord, error) {\n"
            << "\tif workerContext.Executes == nil {\n"
            << "\t\treturn nil, nil\n"
            << "\t}\n"
            << "\trunner := WorkflowRunner{\n"
            << "\t\tBackend:       runtime.Backend,\n"
            << "\t\tWorkflowStore: runtime.Workflows,\n"
            << "\t\tHandler:       handler,\n"
            << "\t\tWorkerName:    workerContext.WorkerName,\n"
            << "\t\tLeaseDuration: 30 * time.Second,\n"
            << "\t\tMaxAttempts:   3,\n"
            << "\t}\n"
            << "\treturn runner.RunOnce(ctx, workflowExecutionID, *workerContext.Executes, 1)\n"
            << "}\n";
    }
    else
    {
        worker_run_once
            << "func (runtime *WorkerTierRuntime) RunOnce(ctx context.Context, workerContext "
               "common.WorkerContext, handler WorkflowStepHandler, workflowExecutionID string) "
               "(*common.WorkflowExecutionRecord, error) {\n"
            << "\treturn nil, nil\n"
            << "}\n";
    }
    return TemplateRenderer::Values{
        {"runtime_store_import", runtime_import.str()},
        {"runtime_store_fields", fields.str()},
        {"runtime_store_initializers", initializers.str()},
        {"runtime_bootstrap_arguments", arguments.str()},
        {"worker_runtime_time_import", worker_time_import.str()},
        {"worker_runtime_run_once", worker_run_once.str()},
    };
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

TemplateRenderer::Values go_api_runtime_bootstrap_values(const IrSystem& system)
{
    auto values = go_runtime_bootstrap_values(system);
    values.erase("worker_runtime_time_import");
    values.erase("worker_runtime_run_once");
    return values;
}

bool go_api_uses_shapes(const IrSystem& system)
{
    for (const auto& api : system.apis)
    {
        if (api.input.has_value() || api.output.has_value())
        {
            return true;
        }
    }
    return false;
}

bool go_api_default_handlers_use_shapes(const IrSystem& system)
{
    for (const auto& api : system.apis)
    {
        if (api.output.has_value())
        {
            return true;
        }
    }
    return false;
}

std::string go_api_shape_import(const IrSystem& system)
{
    if (!go_api_uses_shapes(system))
    {
        return {};
    }
    return "\tshapes \"statespec-generated/common/backend/shapes\"\n";
}

std::string go_api_default_handler_shape_import(const IrSystem& system)
{
    if (!go_api_default_handlers_use_shapes(system))
    {
        return {};
    }
    return "\tshapes \"statespec-generated/common/backend/shapes\"\n";
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

std::string go_api_handler_domain_type_name(std::string_view domain_name)
{
    return "Default" + pascal_identifier(std::string{domain_name}) + "APIHandlerRegistry";
}

std::string go_api_handler_domain_path(std::string_view domain_name)
{
    return "api/backend/api_handler_registry_" + snake_identifier(std::string{domain_name}) + ".go";
}

std::string go_api_handler_registry_delegates(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        const auto type_name = go_api_handler_domain_type_name(domain.name);
        for (const auto& api : domain.apis)
        {
            out << "func (handler DefaultAPITierHandler) Handle" << pascal_identifier(api.name)
                << "(ctx context.Context, request common.APIRequestContext) "
                   "(common.APIResponse, error) {\n";
            out << "\treturn " << type_name << "{Backend: handler.Backend}.Handle"
                << pascal_identifier(api.name) << "(ctx, request)\n";
            out << "}\n\n";
        }
    }
    return out.str();
}

std::string go_api_handler_domain_file(
    const IrSystem& system,
    const ApiHandlerDomain& domain
)
{
    const auto filtered = with_domain_apis(system, domain.apis);
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import (\n";
    out << "\t\"context\"\n";
    out << "\t\"fmt\"\n";
    out << "\t\"time\"\n\n";
    out << "\tcommon \"statespec-generated/common/backend\"\n";
    out << go_api_default_handler_shape_import(filtered);
    out << ")\n\n";
    out << "var _ = fmt.Errorf\n";
    out << "var _ = time.Now\n\n";
    out << "type " << go_api_handler_domain_type_name(domain.name) << " struct {\n";
    out << "\tBackend common.Backend\n";
    out << "}\n\n";
    out << generate_api_operation_default_handler_methods_go_for_receiver(
        filtered, go_api_handler_domain_type_name(domain.name)
    );
    return out.str();
}

TemplateRenderer::Values go_entity_gc_descriptor_values(const IrSystem& system)
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
            terminal_states << "\t\t\t{State: common."
                            << go_entity_state_constant_name(entity.name, state.name)
                            << ", After: " << go_string(state.garbage_collection->after)
                            << ", Mode: " << go_string(state.garbage_collection->mode) << "},\n";
        }
        if (terminal_states.str().empty())
        {
            continue;
        }
        descriptors << "\t\t{\n"
                    << "\t\t\tEntity: common." << go_entity_name_constant_name(entity.name) << ",\n"
                    << "\t\t\tCollection: common." << go_entity_name_constant_name(entity.name)
                    << ",\n"
                    << "\t\t\tTerminalStates: []EntityGCTerminalStateDescriptor{\n"
                    << terminal_states.str() << "\t\t\t},\n"
                    << "\t\t},\n";
    }
    return TemplateRenderer::Values{{"entity_gc_descriptors", descriptors.str()}};
}

void add_go_common_generated_template_file(
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

TemplateRenderer::Values go_descriptor_module_values(
    std::string_view package_name,
    std::string_view module_name
)
{
    return TemplateRenderer::Values{
        {"descriptor_module_package", std::string{package_name}},
        {"descriptor_module_name", std::string{module_name}},
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

TemplateRenderer::Values go_entity_descriptor_values(const IrEntity& entity)
{
    IrSystem one_entity_system;
    one_entity_system.entities.push_back(entity);
    auto content = generate_go_entity_descriptors(one_entity_system);
    const auto type_name = pascal_identifier(entity.name);
    content = replace_all_copy(content, "EntityDescriptors()", type_name + "EntityDescriptors()");
    content =
        replace_all_copy(content, "CollectionDescriptors()", type_name + "CollectionDescriptors()");
    return TemplateRenderer::Values{{"entity_descriptor_content", content}};
}

TemplateRenderer::Values go_shape_descriptor_values(const IrSystem& system)
{
    return TemplateRenderer::Values{
        {"shape_descriptor_content", generate_go_shape_descriptors(system)}
    };
}

void add_go_raw_common_file(
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

std::string go_shape_field_type(const std::string& type)
{
    auto mapped = go_shape_type(type);
    const auto json_pos = mapped.find("JSON");
    if (json_pos != std::string::npos)
    {
        mapped.replace(json_pos, 4, "common.JSON");
    }
    return mapped;
}

bool go_shape_uses_json(const IrShape& shape)
{
    for (const auto& field : shape.fields)
    {
        if (strip_optional_type(field.type) == "json")
        {
            return true;
        }
    }
    return false;
}

std::string go_shape_type_file(const IrShape& shape)
{
    std::ostringstream out;
    out << "package shapes\n\n";
    if (go_shape_uses_json(shape))
    {
        out << "import common \"statespec-generated/common/backend\"\n\n";
    }
    out << "type " << pascal_identifier(shape.name) << " struct {\n";
    for (const auto& field : shape.fields)
    {
        out << "\t" << pascal_identifier(field.name) << " " << go_shape_field_type(field.type)
            << " `json:\"" << field.name << "\"`\n";
    }
    out << "}\n";
    return out.str();
}

void add_go_shape_type_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    for (const auto& shape : system.shapes)
    {
        add_go_raw_common_file(
            result, options, "backend/shapes/" + snake_identifier(shape.name) + ".go",
            go_shape_type_file(shape)
        );
    }
}

void add_go_shape_descriptor_artifact(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("shape_descriptors.go.tmpl"),
        common_artifact_path("backend/shape_descriptors.go"), diagnostics,
        GeneratedArtifactTier::Common, go_shape_descriptor_values(system)
    );
}

void add_go_entity_descriptor_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    for (const auto& entity : system.entities)
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            generated_template_path("entity_descriptors.go.tmpl"),
            common_artifact_path("backend/" + snake_identifier(entity.name) + "_descriptors.go"),
            diagnostics, GeneratedArtifactTier::Common, go_entity_descriptor_values(entity)
        );
    }
}

void add_go_descriptor_module_artifact(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    std::string_view relative_output_path,
    std::string_view package_name,
    std::string_view module_name,
    DiagnosticBag& diagnostics
)
{
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("descriptor_module.go.tmpl"),
        common_artifact_path(relative_output_path), diagnostics, GeneratedArtifactTier::Common,
        go_descriptor_module_values(package_name, module_name)
    );
}

void add_go_descriptor_module_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag& diagnostics
)
{
    add_go_descriptor_module_artifact(
        result, options, templates, "backend/descriptors/core.go", "descriptors",
        "core descriptors", diagnostics
    );
    add_go_descriptor_module_artifact(
        result, options, templates, "backend/descriptors/shapes.go", "descriptors",
        "shape descriptors", diagnostics
    );
    add_go_shape_descriptor_artifact(result, options, templates, system, diagnostics);
    add_go_descriptor_module_artifact(
        result, options, templates, "backend/descriptors/apis.go", "descriptors", "API descriptors",
        diagnostics
    );
    add_go_descriptor_module_artifact(
        result, options, templates, "backend/descriptors/workers.go", "descriptors",
        "worker descriptors", diagnostics
    );
    add_go_descriptor_module_artifact(
        result, options, templates, "backend/descriptors/runtime.go", "descriptors",
        "runtime descriptors", diagnostics
    );
    for (const auto& entity : system.entities)
    {
        add_go_descriptor_module_artifact(
            result, options, templates,
            "backend/descriptors/entities/" + snake_identifier(entity.name) + ".go", "entities",
            "entity descriptor " + entity.name, diagnostics
        );
    }
}

void add_go_generated_template_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    std::string_view relative_output_path,
    GeneratedArtifactTier tier,
    DiagnosticBag& diagnostics,
    const TemplateRenderer::Values& values = {},
    const std::filesystem::path& relative_artifact_path = {}
)
{
    add_generated_template_file(
        result, options.output_dir, templates, template_path_for_output(relative_output_path),
        artifact_path(relative_output_path), diagnostics, tier, values, relative_artifact_path
    );
}

void add_go_raw_api_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    std::string_view relative_output_path,
    std::string content
)
{
    const auto relative_path = artifact_path(relative_output_path);
    result.files.push_back(
        GeneratedFile{
            (options.output_dir / relative_path).string(),
            std::move(content),
            GeneratedArtifactTier::Api,
            relative_path.generic_string(),
        }
    );
}

std::string go_api_descriptor_function_name(const IrApi& api)
{
    return pascal_identifier(api.name) + "ApiDescriptors";
}

std::string go_api_route_descriptor_function_name(const IrApi& api)
{
    return pascal_identifier(api.name) + "ApiRouteDescriptors";
}

bool go_api_server_serves(
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

std::string go_api_descriptor_module(
    const IrSystem& system,
    const IrApi& api
)
{
    std::ostringstream out;
    const auto string_ptr = lower_camel_identifier(api.name) + "StringPtr";
    auto optional_string = [&](const std::optional<std::string>& value)
    { return value.has_value() ? string_ptr + "(" + go_string(*value) + ")" : "nil"; };
    out << "package descriptors\n\n";
    out << "import common \"statespec-generated/common/backend\"\n\n";
    out << "func " << string_ptr << "(value string) *string { return &value }\n\n";
    out << "func " << go_api_descriptor_function_name(api) << "() []common.ApiDescriptor {\n";
    out << "\treturn []common.ApiDescriptor{\n";
    out << "\t\t{\n";
    out << "\t\t\tName: " << go_string(api.name) << ",\n";
    out << "\t\t\tMethod: " << optional_string(api.method) << ",\n";
    out << "\t\t\tPath: " << optional_string(api.path) << ",\n";
    out << "\t\t\tInput: " << optional_string(api.input) << ",\n";
    out << "\t\t\tOutput: " << optional_string(api.output) << ",\n";
    out << "\t\t\tError: " << optional_string(api.error) << ",\n";
    out << "\t\t\tStartsWorkflow: " << optional_string(api.starts_workflow) << ",\n";
    out << "\t\t\tEnqueues: " << optional_string(api.enqueues) << ",\n";
    out << "\t\t},\n";
    out << "\t}\n";
    out << "}\n\n";
    out << "func " << go_api_route_descriptor_function_name(api)
        << "() []common.ApiRouteDescriptor {\n";
    out << "\treturn []common.ApiRouteDescriptor{\n";
    for (const auto& api_server : system.api_servers)
    {
        if (!go_api_server_serves(api_server, api.name))
        {
            continue;
        }
        out << "\t\t{\n";
        out << "\t\t\tName: " << go_string(api_server.name + "." + api.name) << ",\n";
        out << "\t\t\tServerName: " << go_string(api_server.name) << ",\n";
        out << "\t\t\tApiName: " << go_string(api.name) << ",\n";
        out << "\t\t\tMethod: " << optional_string(api.method) << ",\n";
        out << "\t\t\tPath: " << optional_string(api.path) << ",\n";
        out << "\t\t\tInput: " << optional_string(api.input) << ",\n";
        out << "\t\t\tOutput: " << optional_string(api.output) << ",\n";
        out << "\t\t\tError: " << optional_string(api.error) << ",\n";
        out << "\t\t},\n";
    }
    out << "\t}\n";
    out << "}\n";
    return out.str();
}

void add_go_api_descriptor_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    for (const auto& api : system.apis)
    {
        add_go_raw_api_file(
            result, options, "api/backend/descriptors/" + snake_identifier(api.name) + ".go",
            go_api_descriptor_module(system, api)
        );
    }
}

TemplateRenderer::Values go_api_descriptor_values(const IrSystem& system)
{
    std::ostringstream api_aggregation;
    std::ostringstream server_descriptors;
    for (const auto& api : system.apis)
    {
        api_aggregation << "\tresult = append(result, descriptors."
                        << go_api_descriptor_function_name(api) << "()...)\n";
    }
    server_descriptors << "\treturn []common.ApiServerDescriptor{\n";
    for (const auto& api_server : system.api_servers)
    {
        server_descriptors << "\t\t{\n";
        server_descriptors << "\t\t\tName: " << go_string(api_server.name) << ",\n";
        server_descriptors << "\t\t\tServes: []string{";
        for (std::size_t i = 0; i < api_server.serves.size(); ++i)
        {
            if (i > 0)
            {
                server_descriptors << ", ";
            }
            server_descriptors << go_string(api_server.serves[i]);
        }
        server_descriptors << "},\n";
        server_descriptors << "\t\t\tConcurrency: " << api_server.concurrency.value_or(1) << ",\n";
        server_descriptors << "\t\t},\n";
    }
    server_descriptors << "\t}\n";
    return TemplateRenderer::Values{
        {"api_descriptor_aggregation", api_aggregation.str()},
        {"api_server_descriptors", server_descriptors.str()},
    };
}

TemplateRenderer::Values go_api_route_values(const IrSystem& system)
{
    std::ostringstream route_aggregation;
    for (const auto& api : system.apis)
    {
        route_aggregation << "\tresult = append(result, descriptors."
                          << go_api_route_descriptor_function_name(api) << "()...)\n";
    }
    return TemplateRenderer::Values{
        {"api_route_descriptor_aggregation", route_aggregation.str()},
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
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/entity_gc_descriptors.go", diagnostics,
            go_entity_gc_descriptor_values(system)
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/entity_gc_repository.go", diagnostics
        );
        add_go_common_generated_template_file(
            result, options, templates, "backend/runtime/entity_gc_workers.go", diagnostics
        );
    }
    add_go_descriptor_module_artifacts(result, options, templates, system, diagnostics);
    if (diagnostics.has_errors())
    {
        return;
    }
    add_go_shape_type_artifacts(result, options, system);
    add_go_entity_descriptor_artifacts(result, options, templates, system, diagnostics);
    if (diagnostics.has_errors())
    {
        return;
    }
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("descriptors.go.tmpl"),
        common_artifact_path("backend/descriptors.go"), diagnostics, GeneratedArtifactTier::Common,
        TemplateRenderer::Values{{"descriptors", generate_descriptors_go(system, templates)}}
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("go.mod.tmpl"),
        artifact_path(GeneratedArtifactGoMod), diagnostics, GeneratedArtifactTier::Common, {},
        common_artifact_path(GeneratedArtifactGoMod)
    );
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("Makefile.tmpl"),
        artifact_path(GeneratedArtifactMakefile), diagnostics, GeneratedArtifactTier::Common,
        go_makefile_values(options.tier, system), common_artifact_path(GeneratedArtifactMakefile)
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
    const auto include_api_composition = !system.api_servers.empty();

    add_go_api_descriptor_artifacts(result, options, system);
    add_go_generated_template_file(
        result, options, templates, "api/backend/api_descriptors.go", GeneratedArtifactTier::Api,
        diagnostics, go_api_descriptor_values(system)
    );
    add_go_generated_template_file(
        result, options, templates, "api/backend/api_codecs.go", GeneratedArtifactTier::Api,
        diagnostics,
        TemplateRenderer::Values{
            {"api_shape_import", go_api_shape_import(system)},
            {"api_codecs", generate_api_codecs_go(system)}
        }
    );
    add_go_generated_template_file(
        result, options, templates, "api/backend/api_handlers.go", GeneratedArtifactTier::Api,
        diagnostics,
        TemplateRenderer::Values{
            {"api_operation_handler_methods", generate_api_operation_handler_methods_go(system)}
        }
    );
    const auto handler_domains = api_handler_domains(system);
    for (const auto& domain : handler_domains)
    {
        add_go_raw_api_file(
            result, options, go_api_handler_domain_path(domain.name),
            go_api_handler_domain_file(system, domain)
        );
    }
    add_go_generated_template_file(
        result, options, templates, "api/backend/api_handler_registry.go",
        GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_operation_default_handler_methods",
             go_api_handler_registry_delegates(handler_domains)},
            {"api_shape_import", {}}
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
            diagnostics,
            TemplateRenderer::Values{
                {"api_operation_dispatch_cases", generate_api_operation_dispatch_cases_go(system)}
            }
        );
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_server.go", GeneratedArtifactTier::Api,
            diagnostics
        );
        add_go_generated_template_file(
            result, options, templates, "api/backend/api_routes.go", GeneratedArtifactTier::Api,
            diagnostics, go_api_route_values(system)
        );
        add_go_generated_template_file(
            result, options, templates, "api/cmd/api/main.go", GeneratedArtifactTier::Api,
            diagnostics
        );
    }
}

void add_go_worker_artifacts(
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

    add_go_generated_template_file(
        result, options, templates, "worker/backend/worker_descriptors.go",
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_go_generated_template_file(
        result, options, templates, "worker/backend/worker_contexts.go",
        GeneratedArtifactTier::Worker, diagnostics
    );
    add_go_generated_template_file(
        result, options, templates, "worker/backend/worker_registry.go",
        GeneratedArtifactTier::Worker, diagnostics
    );
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
        add_go_generated_template_file(
            result, options, templates, "worker/cmd/worker/main.go", GeneratedArtifactTier::Worker,
            diagnostics
        );
    }
    if (include_worker_execution)
    {
        add_go_generated_template_file(
            result, options, templates, "worker/backend/workflow_step_handlers.go",
            GeneratedArtifactTier::Worker, diagnostics,
            TemplateRenderer::Values{
                {"workflow_step_handler_methods",
                 generate_workflow_step_handler_methods_go(system)},
                {"default_workflow_step_handler_methods",
                 generate_default_workflow_step_handler_methods_go(system)},
                {"workflow_step_handler_imports",
                 generate_workflow_step_handler_imports_go(system)},
                {"workflow_step_handler_keys", generate_workflow_step_handler_keys_go(system)}
            }
        );
        add_go_generated_template_file(
            result, options, templates, "worker/backend/workflow_runner.go",
            GeneratedArtifactTier::Worker, diagnostics,
            TemplateRenderer::Values{
                {"workflow_step_dispatch_cases", generate_workflow_step_dispatch_cases_go(system)},
                {"workflow_step_next_cases", generate_workflow_step_next_cases_go(system)}
            }
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

} // namespace statespec
