#include "generator_go_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_entity_index_helpers.hpp"
#include "generator_go_descriptor_areas.hpp"
#include "generator_go_descriptor_support.hpp"
#include "generator_go_descriptors.hpp"
#include "generator_go_package_artifacts.hpp"
#include "generator_support.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"
#include "string_utils.hpp"
#include "type_syntax.hpp"

#include <algorithm>
#include <optional>
#include <set>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace statespec
{

namespace
{

std::string replace_all_copy(
    std::string value,
    std::string_view from,
    std::string_view to
);

std::string go_exported_api_codec_operations(
    std::string content,
    std::string_view helper_package,
    bool strip_shape_package
);

std::vector<std::pair<
    std::string,
    std::string>>
go_runtime_registration_modules(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::vector<std::pair<std::string, std::string>> modules;
    auto add = [&](bool used, std::string name, std::string label)
    {
        if (used)
        {
            modules.emplace_back(std::move(name), std::move(label));
        }
    };
    add(usage.uses_feature_flags, "feature_flags", "feature flag runtime registration");
    add(usage.uses_queues, "queues", "queue runtime registration");
    add(usage.uses_leases, "leases", "lease runtime registration");
    add(usage.uses_logs, "logs", "log runtime registration");
    add(usage.uses_metrics, "metrics", "metric runtime registration");
    add(usage.uses_logs && usage.uses_metrics, "observability",
        "observability runtime registration");
    add(usage.uses_workflows, "workflows", "workflow runtime registration");
    return modules;
}

std::string go_runtime_registration_module_file(
    const TemplatePackage& templates,
    std::string_view name
)
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import (\n";
    out << "\t\"context\"\n";
    out << ")\n\n";
    out << generate_go_runtime_registration_domain(templates, name);
    return out.str();
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
    std::ostringstream entity_schema_imports;
    std::ostringstream entity_collection_bootstrap;

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
    for (const auto& entity : system.entities)
    {
        const auto package_name = snake_identifier(entity.name);
        entity_schema_imports << "\t" << package_name << " \"statespec-generated/common/entities/"
                              << package_name << "\"\n";
    }
    if (!system.entities.empty())
    {
        entity_collection_bootstrap
            << "\tif err := app.Backend.EnsureCollections(ctx, []common.CollectionDescriptor{\n";
        for (const auto& entity : system.entities)
        {
            const auto package_name = snake_identifier(entity.name);
            entity_collection_bootstrap << "\t\t" << package_name << "."
                                        << pascal_identifier(entity.name)
                                        << "CollectionDescriptor(),\n";
        }
        entity_collection_bootstrap << "\t}); err != nil {\n";
        entity_collection_bootstrap << "\t\treturn err\n";
        entity_collection_bootstrap << "\t}\n";
    }
    worker_time_import << "\t\"time\"\n";
    if (usage.uses_workflows)
    {
        worker_run_once
            << "func (runtime *WorkerTierRuntime) RunOnce(ctx context.Context, workerContext "
               "descriptortypes.WorkerContext, invokers WorkflowStepInvokerMap, "
               "workflowExecutionID "
               "string) "
               "(*common.WorkflowExecutionRecord, error) {\n"
            << "\tif workerContext.Executes == nil {\n"
            << "\t\treturn nil, nil\n"
            << "\t}\n"
            << "\trunner := WorkflowRunner{\n"
            << "\t\tBackend:       runtime.Backend,\n"
            << "\t\tWorkflowStore: runtime.Workflows,\n"
            << "\t\tInvokers:      invokers,\n"
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
               "descriptortypes.WorkerContext, invokers WorkflowStepInvokerMap, "
               "workflowExecutionID "
               "string) "
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
        {"entity_schema_imports", entity_schema_imports.str()},
        {"entity_collection_bootstrap", entity_collection_bootstrap.str()},
    };
}

TemplateRenderer::Values go_worker_runtime_bootstrap_values(const IrSystem& system)
{
    auto values = go_runtime_bootstrap_values(system);
    auto& arguments = values["runtime_bootstrap_arguments"];
    auto& entity_collection_bootstrap = values["entity_collection_bootstrap"];
    std::size_t pos = 0;
    while ((pos = arguments.find("app.", pos)) != std::string::npos)
    {
        arguments.replace(pos, 4, "runtime.");
        pos += 8;
    }
    pos = 0;
    while ((pos = entity_collection_bootstrap.find("app.", pos)) != std::string::npos)
    {
        entity_collection_bootstrap.replace(pos, 4, "runtime.");
        pos += 8;
    }
    return values;
}

std::string generate_go_workflow_module_support()
{
    return R"(package workflows

import workflowcontext "statespec-generated/worker/backend/workflows/context"

type WorkflowStepHandlerContext = workflowcontext.WorkflowStepHandlerContext
type WorkflowStepResultKind = workflowcontext.WorkflowStepResultKind
type WorkflowStepResult = workflowcontext.WorkflowStepResult

const (
	WorkflowStepComplete = workflowcontext.WorkflowStepComplete
	WorkflowStepFail     = workflowcontext.WorkflowStepFail
	WorkflowStepCancel   = workflowcontext.WorkflowStepCancel
)

func Complete(nextStep *string) WorkflowStepResult {
	return workflowcontext.Complete(nextStep)
}

func Fail(reason string) WorkflowStepResult {
	return workflowcontext.Fail(reason)
}

func Cancel(reason string) WorkflowStepResult {
	return workflowcontext.Cancel(reason)
}
)";
}

std::string generate_go_workflow_context_module()
{
    return R"(package context

import common "statespec-generated/common/backend"

type WorkflowStepHandlerContext struct {
	WorkflowName    string
	WorkflowVersion int64
	StepName        string
	ExecutionID     *string
	Input           common.JSON
}

type WorkflowStepResultKind string

const (
	WorkflowStepComplete WorkflowStepResultKind = "complete"
	WorkflowStepFail     WorkflowStepResultKind = "fail"
	WorkflowStepCancel   WorkflowStepResultKind = "cancel"
)

type WorkflowStepResult struct {
	Kind     WorkflowStepResultKind
	NextStep *string
	Reason   string
}

func Complete(nextStep *string) WorkflowStepResult {
	return WorkflowStepResult{Kind: WorkflowStepComplete, NextStep: nextStep}
}

func Fail(reason string) WorkflowStepResult {
	return WorkflowStepResult{Kind: WorkflowStepFail, Reason: reason}
}

func Cancel(reason string) WorkflowStepResult {
	return WorkflowStepResult{Kind: WorkflowStepCancel, Reason: reason}
}
)";
}

std::string go_worker_registry_module(const IrWorker& worker)
{
    const auto pascal = pascal_identifier(worker.name);
    std::ostringstream out;
    out << "package registry\n\n";
    out << "import (\n";
    out << "\tdescriptortypes \"statespec-generated/common/backend/descriptortypes\"\n";
    out << "\tdescriptors \"statespec-generated/worker/backend/descriptors\"\n";
    out << ")\n\n";
    out << "func Find" << pascal
        << "WorkerTierDescriptor(workerName string) (descriptortypes.WorkerDescriptor, bool) {\n";
    out << "\tif workerName == " << go_string(worker.name) << " {\n";
    out << "\t\treturn descriptors." << pascal << "WorkerDescriptor(), true\n";
    out << "\t}\n";
    out << "\treturn descriptortypes.WorkerDescriptor{}, false\n";
    out << "}\n\n";
    out << "func Find" << pascal
        << "WorkerTierContext(workerName string) (descriptortypes.WorkerContext, bool) {\n";
    out << "\tif workerName == " << go_string(worker.name) << " {\n";
    out << "\t\treturn descriptors." << pascal << "WorkerContext(), true\n";
    out << "\t}\n";
    out << "\treturn descriptortypes.WorkerContext{}, false\n";
    out << "}\n";
    return out.str();
}

std::string go_worker_registry_facade(const IrSystem& system)
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import (\n";
    if (!system.workflows.empty())
    {
        out << "\t\"context\"\n\n";
        out << "\tcommon \"statespec-generated/common/backend\"\n";
    }
    out << "\tdescriptortypes \"statespec-generated/common/backend/descriptortypes\"\n";
    if (!system.workers.empty())
    {
        out << "\tregistry \"statespec-generated/worker/backend/registry\"\n";
    }
    for (const auto& workflow : system.workflows)
    {
        out << "\tworkflow" << snake_identifier(workflow.name)
            << " \"statespec-generated/worker/backend/workflows/" << snake_identifier(workflow.name)
            << "\"\n";
    }
    out << ")\n\n";
    out << "func FindWorkerTierDescriptor(workerName string) (descriptortypes.WorkerDescriptor, "
           "bool) {\n";
    for (const auto& worker : system.workers)
    {
        out << "\tif worker, ok := registry.Find" << pascal_identifier(worker.name)
            << "WorkerTierDescriptor(workerName); ok {\n";
        out << "\t\treturn worker, true\n";
        out << "\t}\n";
    }
    out << "\treturn descriptortypes.WorkerDescriptor{}, false\n";
    out << "}\n\n";
    out << "func FindWorkerTierContext(workerName string) (descriptortypes.WorkerContext, bool) "
           "{\n";
    for (const auto& worker : system.workers)
    {
        out << "\tif context, ok := registry.Find" << pascal_identifier(worker.name)
            << "WorkerTierContext(workerName); ok {\n";
        out << "\t\treturn context, true\n";
        out << "\t}\n";
    }
    out << "\treturn descriptortypes.WorkerContext{}, false\n";
    out << "}\n";
    if (!system.workflows.empty())
    {
        for (const auto& workflow : system.workflows)
        {
            const auto alias = "workflow" + snake_identifier(workflow.name);
            const auto pascal = pascal_identifier(workflow.name);
            out << "\nfunc Register" << pascal
                << "WorkflowStepInvokers(invokers WorkflowStepInvokerMap, handler " << alias << "."
                << pascal << "StepHandler) {\n";
            out << "\tlocalInvokers := map[string]" << alias << ".StepInvoker{}\n";
            out << "\t" << alias << ".RegisterWorkflowStepInvokers(localInvokers, handler)\n";
            out << "\tfor key, invoker := range localInvokers {\n";
            out << "\t\tlocalInvoker := invoker\n";
            out << "\t\tinvokers[key] = func(ctx context.Context, backend common.Backend, "
                   "tx common.Transaction, stepContext WorkflowStepHandlerContext) "
                   "(WorkflowStepResult, error) {\n";
            out << "\t\t\treturn localInvoker(ctx, backend, tx, stepContext)\n";
            out << "\t\t}\n";
            out << "\t}\n";
            out << "}\n";
        }
    }
    return out.str();
}

std::string go_workflow_handler_methods(const IrWorkflow& workflow)
{
    std::ostringstream out;
    for (const auto& step : workflow.steps)
    {
        out << "\tHandle" << pascal_identifier(step.name)
            << "(context.Context, common.Transaction, workflowcontext.WorkflowStepHandlerContext) "
               "(workflowcontext.WorkflowStepResult, error)\n";
    }
    return out.str();
}

std::string go_workflow_default_handler_methods(const IrWorkflow& workflow)
{
    std::ostringstream out;
    for (const auto& step : workflow.steps)
    {
        out << "func (Default" << pascal_identifier(workflow.name) << "StepHandler) Handle"
            << pascal_identifier(step.name)
            << "(context.Context, common.Transaction, workflowcontext.WorkflowStepHandlerContext) "
               "(workflowcontext.WorkflowStepResult, error) {\n";
        out << "\treason := \"generated workflow step handler " << workflow.name << "." << step.name
            << " is not implemented\"\n";
        out << "\treturn workflowcontext.Fail(reason), fmt.Errorf(reason)\n";
        out << "}\n\n";
    }
    return out.str();
}

std::string generate_go_workflow_handler_module(
    const TemplatePackage& templates,
    const IrWorkflow& workflow
)
{
    const auto handler_interface = pascal_identifier(workflow.name) + "V" +
                                   std::to_string(workflow.version.value_or(1)) + "StepHandler";
    const auto handler_alias = pascal_identifier(workflow.name) + "StepHandler";
    return templates.render(
        "worker/backend/workflows/handlers.go.tmpl",
        TemplateRenderer::Values{
            {"workflow_package", snake_identifier(workflow.name)},
            {"handler_interface", handler_interface},
            {"handler_alias", handler_alias},
            {"handler_methods", go_workflow_handler_methods(workflow)},
            {"default_handler_methods", go_workflow_default_handler_methods(workflow)},
        }
    );
}

std::string go_workflow_registry_invoker_functions(const IrWorkflow& workflow)
{
    std::ostringstream out;
    for (const auto& step : workflow.steps)
    {
        out << "func invoke" << pascal_identifier(workflow.name) << pascal_identifier(step.name)
            << "(handler " << pascal_identifier(workflow.name) << "StepHandler) StepInvoker {\n";
        out << "\treturn func(ctx context.Context, backend common.Backend, tx common.Transaction, "
               "stepContext "
               "workflowcontext.WorkflowStepHandlerContext) "
               "(workflowcontext.WorkflowStepResult, error) {\n";
        out << "\t_ = backend\n";
        out << "\treturn handler.Handle" << pascal_identifier(step.name)
            << "(ctx, tx, stepContext)\n";
        out << "\t}\n";
        out << "}\n\n";
    }
    return out.str();
}

std::string go_workflow_registry_invoker_entries(const IrWorkflow& workflow)
{
    std::ostringstream out;
    for (const auto& step : workflow.steps)
    {
        out << "\tinvokers["
            << go_string(
                   workflow.name + ":" + std::to_string(workflow.version.value_or(1)) + ":" +
                   step.name
               )
            << "] = invoke" << pascal_identifier(workflow.name) << pascal_identifier(step.name)
            << "(handler)\n";
    }
    return out.str();
}

std::string go_workflow_step_invokers_function(const IrWorkflow& workflow)
{
    std::ostringstream out;
    out << "func WorkflowStepInvokers(handler " << pascal_identifier(workflow.name)
        << "StepHandler) map[string]StepInvoker {\n";
    out << "\tinvokers := map[string]StepInvoker{}\n";
    out << "\tRegisterWorkflowStepInvokers(invokers, handler)\n";
    out << "\treturn invokers\n";
    out << "}\n";
    return out.str();
}

std::string generate_go_workflow_registry_module(
    const TemplatePackage& templates,
    const IrWorkflow& workflow
)
{
    return templates.render(
        "worker/backend/workflows/registry.go.tmpl",
        TemplateRenderer::Values{
            {"workflow_package", snake_identifier(workflow.name)},
            {"handler_alias", pascal_identifier(workflow.name) + "StepHandler"},
            {"step_invoker_functions", go_workflow_registry_invoker_functions(workflow)},
            {"invoker_entries", go_workflow_registry_invoker_entries(workflow)},
            {"workflow_step_invokers_function", go_workflow_step_invokers_function(workflow)},
        }
    );
}

TemplateRenderer::Values go_workflow_runner_values(const IrSystem& system)
{
    (void)system;
    return TemplateRenderer::Values{};
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

bool go_api_uses_shape(
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

const IrShape* go_find_shape(
    const IrSystem& system,
    std::string_view shape_name
)
{
    const auto it = std::find_if(
        system.shapes.begin(), system.shapes.end(),
        [&](const auto& shape) { return shape.name == shape_name; }
    );
    return it == system.shapes.end() ? nullptr : &*it;
}

std::string go_api_shape_import(const IrSystem& system)
{
    if (!go_api_uses_shapes(system))
    {
        return {};
    }
    return "\tshapes \"statespec-generated/api/backend/shapes\"\n";
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
    return "api/backend/entities/" + snake_identifier(std::string{domain_name}) + "/handlers.go";
}

std::string go_api_handler_domain_registry_path(std::string_view domain_name)
{
    return "api/backend/entities/" + snake_identifier(std::string{domain_name}) + "/registry.go";
}

std::string go_api_descriptor_function_name(const IrApi& api);
std::string go_api_route_descriptor_function_name(const IrApi& api);
bool go_api_server_serves(
    const IrApiServer& api_server,
    const std::string& api_name
);

std::string go_entity_api_catalog_path(std::string_view entity_name)
{
    return "api/backend/entities/" + snake_identifier(std::string{entity_name}) + "/catalog.go";
}

std::string go_api_handler_entity_imports(
    const IrSystem& system,
    const std::vector<IrApi>& apis
)
{
    std::set<std::string> entity_names;
    for (const auto& api : apis)
    {
        if (api.entity.has_value())
        {
            entity_names.insert(*api.entity);
            if (api.repository_operation == "create")
            {
                for (const auto& entity : system.entities)
                {
                    if (entity.name != *api.entity)
                    {
                        continue;
                    }
                    for (const auto& relation : entity.relations)
                    {
                        if (relation.kind == "parent")
                        {
                            entity_names.insert(relation.target);
                        }
                    }
                }
            }
        }
        for (const auto& entity : system.entities)
        {
            const auto pascal = pascal_identifier(entity.name);
            const auto is_create = api.name == "Create" + pascal;
            const auto is_entity_api =
                is_create || api.name == "Get" + pascal || api.name == "Delete" + pascal ||
                api.name == "Update" + pascal + "Status" || api.name == "List" + pascal;
            if (!is_entity_api)
            {
                continue;
            }
            entity_names.insert(entity.name);
            if (is_create)
            {
                for (const auto& relation : entity.relations)
                {
                    if (relation.kind == "parent")
                    {
                        entity_names.insert(relation.target);
                    }
                }
            }
        }
    }

    std::ostringstream out;
    for (const auto& entity_name : entity_names)
    {
        out << "\t" << snake_identifier(entity_name) << " \"statespec-generated/common/entities/"
            << snake_identifier(entity_name) << "\"\n";
    }
    return out.str();
}

std::string go_api_handler_lookup_registrations(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        out << "\t" << snake_identifier(domain.name) << ".RegisterHandlerInvokers(registrar)\n";
    }
    return out.str();
}

std::string go_api_handler_registry_domain_imports(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        out << "\t" << snake_identifier(domain.name)
            << " \"statespec-generated/api/backend/entities/" << snake_identifier(domain.name)
            << "\"\n";
    }
    return out.str();
}

bool is_entity_crud_api_go(const IrApi& api)
{
    return api.entity.has_value() && api.repository_operation.has_value();
}

std::vector<ApiHandlerDomain>
crud_api_handler_domains_go(const std::vector<ApiHandlerDomain>& domains)
{
    std::vector<ApiHandlerDomain> result;
    for (const auto& domain : domains)
    {
        for (const auto& api : domain.apis)
        {
            if (is_entity_crud_api_go(api))
            {
                auto target = std::find_if(
                    result.begin(), result.end(),
                    [&](const auto& candidate) { return candidate.name == *api.entity; }
                );
                if (target == result.end())
                {
                    result.push_back(ApiHandlerDomain{*api.entity, {api}});
                }
                else
                {
                    target->apis.push_back(api);
                }
            }
        }
    }
    return result;
}

std::string go_business_api_handler_lookup_entries(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        if (is_entity_crud_api_go(api))
        {
            continue;
        }
        out << "\thandlers[" << go_string(api.name)
            << "] = func(ctx context.Context, handler BusinessAPITierHandler, request "
               "descriptortypes.APIRequestContext) (descriptortypes.APIResponse, error) {\n";
        out << "\t\treturn handler.Handle" << pascal_identifier(api.name) << "(ctx, request)\n";
        out << "\t}\n";
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
    out << "package " << snake_identifier(domain.name) << "\n\n";
    out << "import (\n";
    out << "\t\"context\"\n";
    out << "\t\"fmt\"\n";
    out << "\t\"strings\"\n";
    out << "\t\"time\"\n\n";
    out << "\tcommon \"statespec-generated/common/backend\"\n";
    out << "\tcodecsupport \"statespec-generated/api/backend/codecsupport\"\n";
    out << go_api_handler_entity_imports(system, domain.apis);
    out << ")\n\n";
    out << "var _ = fmt.Errorf\n";
    out << "var _ = time.Now\n\n";
    out << "func extractAPIPathParameters(routeTemplate string, actualPath *string) "
           "map[string]string "
           "{\n";
    out << "\tparameters := map[string]string{}\n";
    out << "\tif actualPath == nil {\n";
    out << "\t\treturn parameters\n";
    out << "\t}\n";
    out << "\ttemplateParts := splitAPIPath(routeTemplate)\n";
    out << "\tactualParts := splitAPIPath(*actualPath)\n";
    out << "\tfor i := 0; i < len(templateParts) && i < len(actualParts); i++ {\n";
    out << "\t\tpart := templateParts[i]\n";
    out << "\t\tif strings.HasPrefix(part, \"{\") && strings.HasSuffix(part, \"}\") && "
           "len(part) > 2 {\n";
    out << "\t\t\tparameters[part[1:len(part)-1]] = actualParts[i]\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn parameters\n";
    out << "}\n\n";
    out << "func splitAPIPath(path string) []string {\n";
    out << "\tparts := []string{}\n";
    out << "\tfor _, part := range strings.Split(path, \"/\") {\n";
    out << "\t\tif part != \"\" {\n";
    out << "\t\t\tparts = append(parts, part)\n";
    out << "\t\t}\n";
    out << "\t}\n";
    out << "\treturn parts\n";
    out << "}\n\n";
    out << "func pathParameterJSON(parameters map[string]string, name string) common.JSON {\n";
    out << "\tvalue, ok := parameters[name]\n";
    out << "\tif !ok {\n";
    out << "\t\treturn common.JSONNull()\n";
    out << "\t}\n";
    out << "\treturn common.JSONString(value)\n";
    out << "}\n\n";
    out << "type " << go_api_handler_domain_type_name(domain.name) << " struct {\n";
    out << "\tBackend common.Backend\n";
    out << "}\n\n";
    out << go_exported_api_codec_operations(
        generate_api_operation_default_handler_methods_go_for_receiver(
            filtered, go_api_handler_domain_type_name(domain.name)
        ),
        "codecsupport", true
    );
    return out.str();
}

std::string go_api_handler_domain_registry_file(const ApiHandlerDomain& domain)
{
    std::ostringstream out;
    out << "package " << snake_identifier(domain.name) << "\n\n";
    out << "import (\n";
    out << "\t\"context\"\n\n";
    out << "\tcommon \"statespec-generated/common/backend\"\n";
    out << "\tdescriptortypes \"statespec-generated/common/backend/descriptortypes\"\n";
    out << ")\n\n";
    out << "func New" << go_api_handler_domain_type_name(domain.name) << "(backend common.Backend) "
        << go_api_handler_domain_type_name(domain.name) << " {\n";
    out << "\treturn " << go_api_handler_domain_type_name(domain.name) << "{Backend: backend}\n";
    out << "}\n\n";
    out << "type HandlerRegistrar interface {\n";
    out << "\tRegisterHandlerInvoker(string, func(context.Context, common.Backend, "
           "descriptortypes.APIRequestContext) (descriptortypes.APIResponse, error))\n";
    out << "}\n\n";
    out << "func RegisterHandlerInvokers(registrar HandlerRegistrar) {\n";
    for (const auto& api : domain.apis)
    {
        out << "\tregistrar.RegisterHandlerInvoker(" << go_string(api.name)
            << ", func(ctx context.Context, backend common.Backend, request "
               "descriptortypes.APIRequestContext) (descriptortypes.APIResponse, error) {\n";
        out << "\t\thandler := New" << go_api_handler_domain_type_name(domain.name)
            << "(backend)\n";
        out << "\t\treturn handler.Handle" << pascal_identifier(api.name) << "(ctx, request)\n";
        out << "\t})\n";
    }
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

std::optional<std::string> go_entity_api_shape_owner(
    const IrSystem& system,
    std::string_view shape_name
)
{
    for (const auto& api : system.apis)
    {
        if (!api.entity.has_value() || !api.repository_operation.has_value())
        {
            continue;
        }
        if ((api.input.has_value() && *api.input == shape_name) ||
            (api.output.has_value() && *api.output == shape_name))
        {
            return *api.entity;
        }
    }
    return std::nullopt;
}

std::vector<IrShape> go_entity_api_shapes(
    const IrSystem& system,
    std::string_view entity_name
)
{
    std::vector<IrShape> shapes;
    for (const auto& shape : system.shapes)
    {
        const auto owner = go_entity_api_shape_owner(system, shape.name);
        if (owner.has_value() && *owner == entity_name)
        {
            shapes.push_back(shape);
        }
    }
    return shapes;
}

IrSystem go_shapes_matching(
    const IrSystem& system,
    bool api_contract_shapes
)
{
    auto filtered = system;
    filtered.shapes.clear();
    for (const auto& shape : system.shapes)
    {
        if (go_api_uses_shape(system, shape.name) == api_contract_shapes)
        {
            filtered.shapes.push_back(shape);
        }
    }
    return filtered;
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

IrSystem with_codec_shapes_apis(
    const IrSystem& system,
    const std::vector<IrShape>& shapes
)
{
    auto filtered = system;
    filtered.apis.clear();
    std::set<std::string> shape_names;
    for (const auto& shape : shapes)
    {
        shape_names.insert(shape.name);
    }
    for (const auto& api : system.apis)
    {
        IrApi scoped = api;
        if (!scoped.input.has_value() || shape_names.count(*scoped.input) == 0)
        {
            scoped.input.reset();
        }
        if (!scoped.output.has_value() || shape_names.count(*scoped.output) == 0)
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

std::string go_api_codec_shape_path(std::string_view shape_name)
{
    return "api/backend/codecs/" + snake_identifier(std::string{shape_name}) + ".go";
}

std::string go_entity_api_codec_path(std::string_view entity_name)
{
    return "api/backend/entities/" + snake_identifier(std::string{entity_name}) + "/codecs.go";
}

std::string go_api_codec_support_path()
{
    return "api/backend/codecsupport/api_codecs.go";
}

bool go_api_has_non_entity_codec_shapes(const IrSystem& system)
{
    for (const auto& shape : api_codec_shapes(system))
    {
        if (!go_entity_api_shape_owner(system, shape.name).has_value())
        {
            return true;
        }
    }
    return false;
}

std::string go_api_codec_imports(const IrSystem& system)
{
    std::ostringstream out;
    if (go_api_has_non_entity_codec_shapes(system))
    {
        out << "\tcodecs \"statespec-generated/api/backend/codecs\"\n";
    }
    std::set<std::string> imported_entities;
    for (const auto& shape : api_codec_shapes(system))
    {
        const auto owner = go_entity_api_shape_owner(system, shape.name);
        if (owner.has_value() && imported_entities.insert(*owner).second)
        {
            out << "\t" << snake_identifier(*owner)
                << " \"statespec-generated/api/backend/entities/" << snake_identifier(*owner)
                << "\"\n";
        }
    }
    return out.str();
}

std::string go_api_codec_delegate_package(
    const IrSystem& system,
    std::string_view shape_name
)
{
    const auto owner = go_entity_api_shape_owner(system, shape_name);
    return owner.has_value() ? snake_identifier(*owner) : "codecs";
}

std::string go_api_codec_shape_file(
    const IrSystem& system,
    const IrShape& shape
)
{
    const auto filtered = with_codec_shape_apis(system, shape.name);
    std::ostringstream out;
    out << "package codecs\n\n";
    out << "import (\n";
    out << "\tcommon \"statespec-generated/common/backend\"\n";
    out << "\tcodecsupport \"statespec-generated/api/backend/codecsupport\"\n";
    out << go_api_shape_import(filtered);
    out << ")\n\n";
    out << go_exported_api_codec_operations(
        generate_api_codec_operations_go(filtered), "codecsupport", false
    );
    return out.str();
}

std::string go_exported_api_codec_operations(
    std::string content,
    std::string_view helper_package,
    bool strip_shape_package
)
{
    const auto prefix = std::string{helper_package} + ".";
    content = replace_all_copy(content, "requireMember", prefix + "RequireMember");
    content = replace_all_copy(content, "decodeString", prefix + "DecodeString");
    content = replace_all_copy(content, "decodeBool", prefix + "DecodeBool");
    content = replace_all_copy(content, "decodeInt64", prefix + "DecodeInt64");
    content = replace_all_copy(content, "decodeInt32", prefix + "DecodeInt32");
    content = replace_all_copy(content, "decodeFloat64", prefix + "DecodeFloat64");
    content = replace_all_copy(content, "decodeJSON", prefix + "DecodeJSON");
    content = replace_all_copy(content, "mustJSONFloat", prefix + "MustJSONFloat");
    if (strip_shape_package)
    {
        content = replace_all_copy(content, "shapes.", "");
    }
    return content;
}

std::string go_entity_api_codec_file(
    const IrSystem& system,
    const IrEntity& entity
)
{
    const auto shapes = go_entity_api_shapes(system, entity.name);
    const auto filtered = with_codec_shapes_apis(system, shapes);
    std::ostringstream out;
    out << "package " << snake_identifier(entity.name) << "\n\n";
    out << "import (\n";
    out << "\tcommon \"statespec-generated/common/backend\"\n";
    out << "\tentityconstants \"statespec-generated/common/entities/"
        << snake_identifier(entity.name) << "\"\n";
    out << "\tcodecsupport \"statespec-generated/api/backend/codecsupport\"\n";
    out << ")\n\n";
    out << go_exported_api_codec_operations(
        generate_api_codec_operations_go(filtered), "codecsupport", true
    );
    return out.str();
}

std::string go_api_codec_support_file()
{
    std::ostringstream out;
    out << "package codecsupport\n\n";
    out << "import (\n";
    out << "\t\"fmt\"\n\n";
    out << "\tcommon \"statespec-generated/common/backend\"\n";
    out << ")\n\n";
    out << generate_api_codec_helpers_go();
    out << "func RequireMember(value common.JSON, fieldName string) (common.JSON, error) {\n";
    out << "\treturn requireMember(value, fieldName)\n";
    out << "}\n\n";
    out << "func DecodeString(value common.JSON, fieldName string) (string, error) {\n";
    out << "\treturn decodeString(value, fieldName)\n";
    out << "}\n\n";
    out << "func DecodeBool(value common.JSON, fieldName string) (bool, error) {\n";
    out << "\treturn decodeBool(value, fieldName)\n";
    out << "}\n\n";
    out << "func DecodeInt64(value common.JSON, fieldName string) (int64, error) {\n";
    out << "\treturn decodeInt64(value, fieldName)\n";
    out << "}\n\n";
    out << "func DecodeInt32(value common.JSON, fieldName string) (int32, error) {\n";
    out << "\treturn decodeInt32(value, fieldName)\n";
    out << "}\n\n";
    out << "func DecodeFloat64(value common.JSON, fieldName string) (float64, error) {\n";
    out << "\treturn decodeFloat64(value, fieldName)\n";
    out << "}\n\n";
    out << "func DecodeJSON(value common.JSON, fieldName string) (common.JSON, error) {\n";
    out << "\treturn decodeJSON(value, fieldName)\n";
    out << "}\n\n";
    out << "func MustJSONFloat(value float64) common.JSON {\n";
    out << "\treturn mustJSONFloat(value)\n";
    out << "}\n\n";
    return out.str();
}

const IrShape* find_api_codec_shape(
    const IrSystem& system,
    std::string_view name
)
{
    const auto it = std::find_if(
        system.shapes.begin(), system.shapes.end(),
        [&](const auto& shape) { return shape.name == name; }
    );
    return it == system.shapes.end() ? nullptr : &*it;
}

std::string go_api_codec_delegates(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        if (api.input.has_value())
        {
            const auto* shape = find_api_codec_shape(system, *api.input);
            if (shape != nullptr)
            {
                out << "func Decode" << pascal_identifier(api.name)
                    << "Request(request common.APIRequestContext) (shapes."
                    << pascal_identifier(shape->name) << ", error) {\n";
                out << "\treturn " << go_api_codec_delegate_package(system, shape->name)
                    << ".Decode" << pascal_identifier(api.name) << "Request(request)\n";
                out << "}\n\n";
            }
        }
        if (api.output.has_value())
        {
            const auto* shape = find_api_codec_shape(system, *api.output);
            if (shape != nullptr)
            {
                out << "func Decode" << pascal_identifier(api.name)
                    << "Response(response common.APIResponse) (shapes."
                    << pascal_identifier(shape->name) << ", error) {\n";
                out << "\treturn " << go_api_codec_delegate_package(system, shape->name)
                    << ".Decode" << pascal_identifier(api.name) << "Response(response)\n";
                out << "}\n\n";
                out << "func Decode" << pascal_identifier(api.name)
                    << "ResponseBody(request common.APIRequestContext) (shapes."
                    << pascal_identifier(shape->name) << ", error) {\n";
                out << "\treturn " << go_api_codec_delegate_package(system, shape->name)
                    << ".Decode" << pascal_identifier(api.name) << "ResponseBody(request)\n";
                out << "}\n\n";
                out << "func Encode" << pascal_identifier(api.name) << "Response(response shapes."
                    << pascal_identifier(shape->name) << ") common.APIResponse {\n";
                out << "\treturn " << go_api_codec_delegate_package(system, shape->name)
                    << ".Encode" << pascal_identifier(api.name) << "Response(response)\n";
                out << "}\n\n";
                out << "func Encode" << pascal_identifier(api.name)
                    << "ResponseWithStatus(response shapes." << pascal_identifier(shape->name)
                    << ", statusCode int) common.APIResponse {\n";
                out << "\treturn " << go_api_codec_delegate_package(system, shape->name)
                    << ".Encode" << pascal_identifier(api.name)
                    << "ResponseWithStatus(response, statusCode)\n";
                out << "}\n\n";
            }
        }
    }
    return out.str();
}

std::string go_entity_gc_descriptor_file(const IrEntity& entity)
{
    std::ostringstream terminal_states;
    for (const auto& state : entity.states)
    {
        if (!state.garbage_collection.has_value())
        {
            continue;
        }
        terminal_states << "\t\t\t{State: "
                        << go_entity_state_constant_name(entity.name, state.name)
                        << ", After: " << pascal_identifier(entity.name) << "State"
                        << pascal_identifier(state.name) << "GcAfter"
                        << ", Mode: " << pascal_identifier(entity.name) << "State"
                        << pascal_identifier(state.name) << "GcMode},\n";
    }

    const auto descriptor_function = pascal_identifier(entity.name) + "EntityGCDescriptor";
    std::ostringstream out;
    out << "package " << snake_identifier(entity.name) << "\n\n";
    out << "import entitygc \"statespec-generated/common/backend/runtime/entitygc\"\n\n";
    out << "func " << descriptor_function << "() entitygc.EntityGCDescriptor {\n";
    out << "\treturn entitygc.EntityGCDescriptor{\n";
    out << "\t\tEntity: " << go_entity_name_constant_name(entity.name) << ",\n";
    out << "\t\tCollection: " << go_entity_name_constant_name(entity.name) << ",\n";
    out << "\t\tStatusField: " << go_entity_field_constant_name(entity.name, "status") << ",\n";
    out << "\t\tTerminalStates: []entitygc.EntityGCTerminalStateDescriptor{\n";
    out << terminal_states.str();
    out << "\t\t},\n";
    out << "\t}\n";
    out << "}\n";
    return out.str();
}

std::string go_api_entity_gc_catalog_file(const IrSystem& system)
{
    std::ostringstream imports;
    std::ostringstream descriptor_calls;
    for (const auto& entity : system.entities)
    {
        const auto entity_uses_gc = std::any_of(
            entity.states.begin(), entity.states.end(),
            [](const IrState& state) { return state.garbage_collection.has_value(); }
        );
        if (!entity_uses_gc)
        {
            continue;
        }
        imports << "\t" << snake_identifier(entity.name)
                << " \"statespec-generated/common/entities/" << snake_identifier(entity.name)
                << "\"\n";
        descriptor_calls << "\t\t" << snake_identifier(entity.name) << "."
                         << pascal_identifier(entity.name) << "EntityGCDescriptor(),\n";
    }

    std::ostringstream out;
    out << "package backend\n\n";
    out << "import (\n";
    out << "\truntime \"statespec-generated/common/backend/runtime\"\n";
    out << imports.str();
    out << ")\n\n";
    out << "func APIEntityGCDescriptors() []runtime.EntityGCDescriptor {\n";
    out << "\treturn []runtime.EntityGCDescriptor{\n";
    out << descriptor_calls.str();
    out << "\t}\n";
    out << "}\n";
    return out.str();
}

std::string go_worker_entity_gc_catalog_file(const IrSystem& system)
{
    std::ostringstream imports;
    std::ostringstream descriptor_calls;
    for (const auto& entity : system.entities)
    {
        const auto entity_uses_gc = std::any_of(
            entity.states.begin(), entity.states.end(),
            [](const IrState& state) { return state.garbage_collection.has_value(); }
        );
        if (!entity_uses_gc)
        {
            continue;
        }
        imports << "\t" << snake_identifier(entity.name)
                << " \"statespec-generated/common/entities/" << snake_identifier(entity.name)
                << "\"\n";
        descriptor_calls << "\t\t" << snake_identifier(entity.name) << "."
                         << pascal_identifier(entity.name) << "EntityGCDescriptor(),\n";
    }

    std::ostringstream out;
    out << "package backend\n\n";
    out << "import (\n";
    out << "\truntime \"statespec-generated/common/backend/runtime\"\n";
    out << imports.str();
    out << ")\n\n";
    out << "func WorkerEntityGCDescriptors() []runtime.EntityGCDescriptor {\n";
    out << "\treturn []runtime.EntityGCDescriptor{\n";
    out << descriptor_calls.str();
    out << "\t}\n";
    out << "}\n";
    return out.str();
}

TemplateRenderer::Values go_api_main_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    if (!usage.uses_entity_gc)
    {
        return TemplateRenderer::Values{
            {"api_main_entity_gc_import", ""},
            {"api_main_entity_gc_registration", ""},
        };
    }
    return TemplateRenderer::Values{
        {"api_main_entity_gc_import", "\truntime \"statespec-generated/common/backend/runtime\"\n"},
        {"api_main_entity_gc_registration",
         "\tif err := runtime.RegisterEntityGCWorkers(func(worker "
         "func(context.Context, string) error) error {\n"
         "\t\treturn process.AddEntityGCWorker(api.EntityGCWorkerFunc(worker))\n"
         "\t}, backend, api.APIEntityGCDescriptors()); err != nil {\n"
         "\t\tfmt.Fprintf(os.Stderr, \"statespec generated API process failed: %v\\n\", err)\n"
         "\t\tos.Exit(1)\n"
         "\t}\n\n"},
    };
}

TemplateRenderer::Values go_worker_main_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    std::ostringstream workflow_imports;
    std::ostringstream workflow_invoker_composition;
    for (const auto& workflow : system.workflows)
    {
        const auto workflow_package = snake_identifier(workflow.name);
        const auto workflow_alias = "workflow" + workflow_package;
        const auto workflow_var = workflow_package + "Handler";
        workflow_imports << "\t" << workflow_alias << " \"statespec-generated/worker/backend/"
                         << "workflows/" << workflow_package << "\"\n";
        workflow_invoker_composition << "\t" << workflow_var << " := " << workflow_alias
                                     << ".Default" << pascal_identifier(workflow.name)
                                     << "StepHandler{}\n";
        workflow_invoker_composition << "\tworker.Register" << pascal_identifier(workflow.name)
                                     << "WorkflowStepInvokers(invokers, " << workflow_var << ")\n";
    }
    if (!usage.uses_entity_gc)
    {
        return TemplateRenderer::Values{
            {"worker_main_entity_gc_import", ""},
            {"worker_main_entity_gc_registration", ""},
            {"worker_main_workflow_imports", workflow_imports.str()},
            {"worker_main_workflow_invoker_composition", workflow_invoker_composition.str()},
        };
    }
    return TemplateRenderer::Values{
        {"worker_main_entity_gc_import",
         "\truntimegc \"statespec-generated/common/backend/runtime\"\n"},
        {"worker_main_entity_gc_registration",
         "\tif err := runtimegc.RegisterEntityGCWorkers(func(gcWorker "
         "func(context.Context, string) error) error {\n"
         "\t\treturn runtime.AddEntityGCWorker(worker.WorkerEntityGCWorkerFunc(gcWorker))\n"
         "\t}, backend, worker.WorkerEntityGCDescriptors()); err != nil {\n"
         "\t\tfmt.Fprintf(os.Stderr, \"statespec generated Worker process failed: %v\\n\", err)\n"
         "\t\tos.Exit(1)\n"
         "\t}\n\n"},
        {"worker_main_workflow_imports", workflow_imports.str()},
        {"worker_main_workflow_invoker_composition", workflow_invoker_composition.str()},
    };
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

std::string go_descriptor_types_file()
{
    return generate_go_descriptor_types_file();
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

std::string go_entity_centered_facade_file(
    const TemplatePackage& templates,
    const IrEntity& entity,
    std::string_view area
)
{
    std::ostringstream out;
    out << "package " << snake_identifier(entity.name) << "\n\n";
    if (area == "model")
    {
        out << "import backend \"statespec-generated/common/backend\"\n\n";
        out << "func " << pascal_identifier(entity.name)
            << "FieldDescriptors() []backend.FieldDescriptor {\n";
        out << "\treturn []backend.FieldDescriptor{\n";
        for (const auto& field : entity.fields)
        {
            auto field_descriptor = go_entity_field_descriptor_expr(entity.name, field);
            field_descriptor = replace_all_copy(field_descriptor, "FieldType", "backend.FieldType");
            out << "\t\t" << field_descriptor << ",\n";
        }
        out << "\t}\n";
        out << "}\n\n";
        out << "func " << pascal_identifier(entity.name)
            << "IndexDescriptors() []backend.IndexDescriptor {\n";
        out << "\treturn []backend.IndexDescriptor{\n";
        for (const auto& index : entity.indexes)
        {
            out << "\t\t{\n";
            out << "\t\t\tName: " << go_entity_index_constant_name(entity.name, index.name)
                << ",\n";
            out << "\t\t\tFields: []string{";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << go_entity_field_constant_name(entity.name, index.fields[i]);
            }
            out << "},\n";
            out << "\t\t\tUnique: " << (index.unique ? "true" : "false") << ",\n";
            out << "\t\t},\n";
        }
        out << "\t}\n";
        out << "}\n\n";
        out << "// Field, index, relationship, and state constants are rooted in "
               "constants.go.\n";
    }
    else if (area == "schema")
    {
        out << "import backend \"statespec-generated/common/backend\"\n\n";
        out << "func stringPtr(value string) *string {\n";
        out << "\treturn &value\n";
        out << "}\n\n";
        out << "const (\n";
        out << "\t" << pascal_identifier(entity.name) << "SchemaVersion uint64 = 1\n";
        if (entity.ownership.has_value())
        {
            out << "\t" << pascal_identifier(entity.name)
                << "OwnershipAuthority = " << go_string(entity.ownership->authority) << "\n";
            out << "\t" << pascal_identifier(entity.name)
                << "OwnershipSystemOfRecord = " << go_string(entity.ownership->system_of_record)
                << "\n";
            out << "\t" << pascal_identifier(entity.name)
                << "OwnershipLifecycle = " << go_string(entity.ownership->lifecycle) << "\n";
        }
        for (const auto& state : entity.states)
        {
            out << "\t" << pascal_identifier(entity.name) << "State"
                << pascal_identifier(state.name)
                << "Terminal = " << (state.terminal ? "true" : "false") << "\n";
            if (state.garbage_collection.has_value())
            {
                out << "\t" << pascal_identifier(entity.name) << "State"
                    << pascal_identifier(state.name)
                    << "GcAfter = " << go_string(state.garbage_collection->after) << "\n";
                out << "\t" << pascal_identifier(entity.name) << "State"
                    << pascal_identifier(state.name)
                    << "GcMode = " << go_string(state.garbage_collection->mode) << "\n";
            }
        }
        out << ")\n\n";
        out << "func " << pascal_identifier(entity.name)
            << "CollectionDescriptor() backend.CollectionDescriptor {\n";
        out << "\treturn backend.CollectionDescriptor{\n";
        out << "\t\tName: " << go_entity_name_constant_name(entity.name) << ",\n";
        out << "\t\tFields: " << pascal_identifier(entity.name) << "FieldDescriptors(),\n";
        out << "\t\tKeyFields: []string{";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "},\n";
        out << "\t\tIndexes: " << pascal_identifier(entity.name) << "IndexDescriptors(),\n";
        out << "\t\tSchemaVersion: " << pascal_identifier(entity.name) << "SchemaVersion,\n";
        out << "\t}\n";
        out << "}\n\n";
        out << "func " << pascal_identifier(entity.name)
            << "CollectionDescriptors() []backend.CollectionDescriptor {\n";
        out << "\treturn []backend.CollectionDescriptor{" << pascal_identifier(entity.name)
            << "CollectionDescriptor()}\n";
        out << "}\n\n";
        out << "func " << pascal_identifier(entity.name)
            << "EntityDescriptor() backend.EntityDescriptor {\n";
        out << "\treturn backend.EntityDescriptor{\n";
        out << "\t\tName: " << go_entity_name_constant_name(entity.name) << ",\n";
        out << "\t\tKeyFields: []string{";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "},\n";
        if (entity.ownership.has_value())
        {
            out << "\t\tOwnership: &backend.EntityOwnershipDescriptor{Authority: "
                << pascal_identifier(entity.name)
                << "OwnershipAuthority, SystemOfRecord: " << pascal_identifier(entity.name)
                << "OwnershipSystemOfRecord, Lifecycle: " << pascal_identifier(entity.name)
                << "OwnershipLifecycle},\n";
        }
        else
        {
            out << "\t\tOwnership: nil,\n";
        }
        out << "\t\tRelations: []backend.EntityRelationDescriptor{\n";
        for (const auto& relation : entity.relations)
        {
            const auto relation_constant_prefix =
                pascal_identifier(entity.name) + "Relation" + pascal_identifier(relation.name);
            out << "\t\t\t{\n";
            out << "\t\t\t\tKind: " << relation_constant_prefix << "Kind,\n";
            out << "\t\t\t\tName: " << relation_constant_prefix << "Name,\n";
            out << "\t\t\t\tTarget: " << relation_constant_prefix << "Target,\n";
            out << "\t\t\t\tOptional: " << (relation.optional ? "true" : "false") << ",\n";
            if (relation.relation_kind.has_value())
            {
                out << "\t\t\t\tRelationKind: stringPtr(" << relation_constant_prefix
                    << "RelationKind),\n";
            }
            else
            {
                out << "\t\t\t\tRelationKind: nil,\n";
            }
            if (relation.on_parent_delete.has_value())
            {
                out << "\t\t\t\tOnParentDelete: stringPtr(" << relation_constant_prefix
                    << "OnParentDelete),\n";
            }
            else
            {
                out << "\t\t\t\tOnParentDelete: nil,\n";
            }
            out << "\t\t\t\tParentMustBeIn: []string{";
            for (std::size_t i = 0; i < relation.parent_must_be_in.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << go_string(relation.parent_must_be_in[i]);
            }
            out << "},\n";
            out << "\t\t\t\tUniqueWithinParent: []string{";
            for (std::size_t i = 0; i < relation.unique_within_parent.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << go_entity_field_constant_name(entity.name, relation.unique_within_parent[i]);
            }
            out << "},\n";
            out << "\t\t\t},\n";
        }
        out << "\t\t},\n";
        out << "\t\tChildren: []backend.EntityChildDescriptor{\n";
        for (const auto& child : entity.children)
        {
            out << "\t\t\t{Name: " << go_string(child.name)
                << ", TargetEntity: " << go_string(child.target_entity)
                << ", Relation: " << go_string(child.relation) << "},\n";
        }
        out << "\t\t},\n";
        out << "\t\tInvariants: []backend.EntityInvariantDescriptor{\n";
        for (const auto& invariant : entity.invariants)
        {
            out << "\t\t\t{Name: " << go_string(invariant.name)
                << ", Expression: " << go_string(invariant.expression) << "},\n";
        }
        out << "\t\t},\n";
        out << "\t\tStates: []backend.EntityStateDescriptor{\n";
        for (const auto& state : entity.states)
        {
            out << "\t\t\t{\n";
            out << "\t\t\t\tName: " << go_entity_state_constant_name(entity.name, state.name)
                << ",\n";
            out << "\t\t\t\tTerminal: " << pascal_identifier(entity.name) << "State"
                << pascal_identifier(state.name) << "Terminal,\n";
            if (state.garbage_collection.has_value())
            {
                out << "\t\t\t\tGarbageCollection: &backend.GarbageCollectionPolicy{After: "
                    << pascal_identifier(entity.name) << "State" << pascal_identifier(state.name)
                    << "GcAfter, Mode: " << pascal_identifier(entity.name) << "State"
                    << pascal_identifier(state.name) << "GcMode},\n";
            }
            else
            {
                out << "\t\t\t\tGarbageCollection: nil,\n";
            }
            out << "\t\t\t},\n";
        }
        out << "\t\t},\n";
        if (entity.initial_state.has_value())
        {
            out << "\t\tInitialState: stringPtr("
                << go_entity_state_constant_name(entity.name, *entity.initial_state) << "),\n";
        }
        else
        {
            out << "\t\tInitialState: nil,\n";
        }
        out << "\t\tTerminalStates: []string{";
        for (std::size_t i = 0; i < entity.terminal_states.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_entity_state_constant_name(entity.name, entity.terminal_states[i]);
        }
        out << "},\n";
        out << "\t}\n";
        out << "}\n\n";
        out << "func " << pascal_identifier(entity.name)
            << "EntityDescriptors() []backend.EntityDescriptor {\n";
        out << "\treturn []backend.EntityDescriptor{" << pascal_identifier(entity.name)
            << "EntityDescriptor()}\n";
        out << "}\n\n";
        out << "// Collection schema and compatibility metadata are rooted with the entity "
               "schema.\n";
    }
    else if (area == "persistence")
    {
        const auto type_name = pascal_identifier(entity.name);
        out << "import (\n";
        out << "\t\"context\"\n\n";
        out << "\tbackend \"statespec-generated/common/backend\"\n";
        out << ")\n\n";
        out << "func " << type_name << "Lookup(keyValues []backend.EntityKeyValue) "
            << "backend.EntityLookup {\n";
        out << "\treturn backend.EntityLookup{Entity: " << go_entity_name_constant_name(entity.name)
            << ", KeyFields: []string{";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << go_entity_field_constant_name(entity.name, entity.key_fields[i]);
        }
        out << "}, KeyValues: keyValues}\n";
        out << "}\n\n";
        out << "type " << type_name << "Repository interface {\n";
        out << "\tRegisterDescriptor(context.Context, backend.Backend) error\n";
        out << "\tCreateTx(context.Context, backend.Transaction, backend.JSON) "
               "(*backend.VersionedRecord, error)\n";
        out << "\tGetTx(context.Context, backend.Transaction, []backend.EntityKeyValue) "
               "(*backend.VersionedRecord, error)\n";
        out << "\tListByIndexTx(context.Context, backend.Transaction, string, "
               "[]backend.IndexValue) ([]backend.VersionedRecord, error)\n";
        for (const auto& index : entity.indexes)
        {
            out << "\t" << go_entity_index_repository_method_name(index.name)
                << "(context.Context, backend.Transaction, []backend.IndexValue) "
                   "([]backend.VersionedRecord, error)\n";
        }
        out << "\tUpdateTx(context.Context, backend.Transaction, []backend.EntityKeyValue, "
               "backend.JSON, backend.Version) (*backend.VersionedRecord, error)\n";
        out << "}\n\n";
        out << "type Default" << type_name << "Repository struct{}\n\n";
        out << "func (Default" << type_name
            << "Repository) RegisterDescriptor(ctx context.Context, backendStore "
               "backend.Backend) error {\n";
        out << "\treturn backendStore.EnsureCollection(ctx, " << type_name
            << "CollectionDescriptor())\n";
        out << "}\n\n";
        out << "func (Default" << type_name
            << "Repository) CreateTx(ctx context.Context, tx backend.Transaction, document "
               "backend.JSON) (*backend.VersionedRecord, error) {\n";
        out << "\treturn backend.DefaultEntityRepository{}.CreateEntityTx(ctx, tx, "
               "backend.EntityCreateRequest{\n";
        out << "\t\tLookup: backend.EntityLookupFromDocument(" << type_name
            << "EntityDescriptor(), document),\n";
        out << "\t\tDocument: document,\n";
        out << "\t})\n";
        out << "}\n\n";
        out << "func (Default" << type_name
            << "Repository) GetTx(ctx context.Context, tx backend.Transaction, keyValues "
               "[]backend.EntityKeyValue) (*backend.VersionedRecord, error) {\n";
        out << "\treturn backend.DefaultEntityRepository{}.GetEntityTx(ctx, tx, "
               "backend.EntityGetRequest{Lookup: "
            << type_name << "Lookup(keyValues)})\n";
        out << "}\n\n";
        out << "func (Default" << type_name
            << "Repository) ListByIndexTx(ctx context.Context, tx backend.Transaction, indexName "
               "string, values []backend.IndexValue) ([]backend.VersionedRecord, error) {\n";
        out << "\treturn backend.DefaultEntityRepository{}.ListEntitiesByIndexTx(ctx, tx, "
               "backend.EntityListByIndexRequest{Entity: "
            << go_entity_name_constant_name(entity.name)
            << ", IndexName: indexName, Values: values})\n";
        out << "}\n\n";
        for (const auto& index : entity.indexes)
        {
            out << "func (repository Default" << type_name << "Repository) "
                << go_entity_index_repository_method_name(index.name)
                << "(ctx context.Context, tx backend.Transaction, values []backend.IndexValue) "
                   "([]backend.VersionedRecord, error) {\n";
            out << "\treturn repository.ListByIndexTx(ctx, tx, "
                << go_entity_index_constant_name(entity.name, index.name) << ", values)\n";
            out << "}\n\n";
        }
        out << "func (Default" << type_name
            << "Repository) UpdateTx(ctx context.Context, tx backend.Transaction, keyValues "
               "[]backend.EntityKeyValue, document backend.JSON, expectedVersion "
               "backend.Version) (*backend.VersionedRecord, error) {\n";
        out << "\treturn backend.DefaultEntityRepository{}.UpsertEntityTx(ctx, tx, "
               "backend.EntityUpsertRequest{\n";
        out << "\t\tLookup: " << type_name << "Lookup(keyValues),\n";
        out << "\t\tDocument: document,\n";
        out << "\t\tExpectedVersion: &expectedVersion,\n";
        out << "\t})\n";
        out << "}\n\n";
        out << "var _ " << type_name << "Repository = Default" << type_name << "Repository{}\n";
    }
    else
    {
        out << "// Entity-centered " << area
            << " facade. Implementation moves here in the next staged split.\n";
    }
    const auto content = out.str();
    if (area == "model")
    {
        return templates.render(
            "common/entities/model.go.tmpl", TemplateRenderer::Values{{"model_content", content}}
        );
    }
    if (area == "schema")
    {
        return templates.render(
            "common/entities/schema.go.tmpl", TemplateRenderer::Values{{"schema_content", content}}
        );
    }
    if (area == "persistence")
    {
        return templates.render(
            "common/entities/persistence.go.tmpl",
            TemplateRenderer::Values{{"persistence_content", content}}
        );
    }
    return content;
}

std::string go_entity_constants_file(
    const TemplatePackage& templates,
    const IrEntity& entity
)
{
    std::ostringstream out;
    out << "package " << snake_identifier(entity.name) << "\n\n";
    out << "const (\n";
    out << "\t" << go_entity_name_constant_name(entity.name) << " = " << go_string(entity.name)
        << "\n";
    out << "\t" << go_entity_plural_name_constant_name(entity.name) << " = "
        << go_string(go_entity_plural_api_field_name(entity.name)) << "\n";
    out << "\t" << pascal_identifier(entity.name) << "CollectionName = " << go_string(entity.name)
        << "\n";
    out << "\t" << pascal_identifier(entity.name)
        << "KeyHelperName = " << go_string(pascal_identifier(entity.name) + "Lookup") << "\n";
    for (const auto& field : entity.fields)
    {
        out << "\t" << go_entity_field_constant_name(entity.name, field.name) << " = "
            << go_string(field.name) << "\n";
        out << "\t" << go_entity_field_type_name_constant_name(entity.name, field.name) << " = "
            << go_string(field.type) << "\n";
    }
    for (const auto& index : entity.indexes)
    {
        out << "\t" << go_entity_index_constant_name(entity.name, index.name) << " = "
            << go_string(index.name) << "\n";
        out << "\t" << pascal_identifier(entity.name) << "Index" << pascal_identifier(index.name)
            << "HelperName = " << go_string(go_entity_index_repository_method_name(index.name))
            << "\n";
    }
    for (const auto& state : entity.states)
    {
        out << "\t" << go_entity_state_constant_name(entity.name, state.name) << " = "
            << go_string(state.name) << "\n";
    }
    for (const auto& relation : entity.relations)
    {
        const auto relation_constant_prefix =
            pascal_identifier(entity.name) + "Relation" + pascal_identifier(relation.name);
        out << "\t" << relation_constant_prefix << "Name = " << go_string(relation.name) << "\n";
        out << "\t" << relation_constant_prefix << "Target = " << go_string(relation.target)
            << "\n";
        out << "\t" << relation_constant_prefix << "Kind = " << go_string(relation.kind) << "\n";
        if (relation.relation_kind.has_value())
        {
            out << "\t" << relation_constant_prefix
                << "RelationKind = " << go_string(*relation.relation_kind) << "\n";
        }
        if (relation.on_parent_delete.has_value())
        {
            out << "\t" << relation_constant_prefix
                << "OnParentDelete = " << go_string(*relation.on_parent_delete) << "\n";
        }
    }
    out << ")\n";
    return templates.render(
        "common/entities/constants.go.tmpl",
        TemplateRenderer::Values{{"constants_content", out.str()}}
    );
}

std::string go_event_helpers_file(const IrSystem& system)
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "type EventEnvelope struct {\n";
    out << "\tName string\n";
    out << "\tFields map[string]JSON\n";
    out << "}\n\n";
    for (const auto& event : system.events)
    {
        out << "func New" << pascal_identifier(event.name) << "Event(";
        for (std::size_t i = 0; i < event.fields.size(); ++i)
        {
            const auto& field = event.fields[i];
            if (i > 0)
            {
                out << ", ";
            }
            out << lower_camel_identifier(field.name) << " JSON";
        }
        out << ") EventEnvelope {\n";
        out << "\treturn EventEnvelope{\n";
        out << "\t\tName: " << go_string(event.name) << ",\n";
        out << "\t\tFields: map[string]JSON{\n";
        for (const auto& field : event.fields)
        {
            out << "\t\t\t" << go_string(field.name) << ": " << lower_camel_identifier(field.name)
                << ",\n";
        }
        out << "\t\t},\n";
        out << "\t}\n";
        out << "}\n\n";
    }
    return out.str();
}

std::string go_external_systems_file(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import (\n";
    out << "\t\"context\"\n";
    out << "\t\"strings\"\n";
    out << ")\n\n";
    out << generate_go_external_system_descriptors(
        system, templates.load("generated/external_system_call_adapters.go.tmpl")
    );
    return out.str();
}

TemplateRenderer::Values go_shape_descriptor_values(const IrSystem& system)
{
    std::ostringstream content;
    content << "func ShapeDescriptors() []ShapeDescriptor {\n";
    content << "\tvar descriptors []ShapeDescriptor\n";
    for (const auto& shape : system.shapes)
    {
        content << "\tdescriptors = append(descriptors, " << pascal_identifier(shape.name)
                << "ShapeDescriptors()...)\n";
    }
    content << "\treturn descriptors\n";
    content << "}\n\n";
    return TemplateRenderer::Values{{"shape_descriptor_content", content.str()}};
}

std::string go_shape_descriptor_file(const IrShape& shape)
{
    IrSystem one_shape_system;
    one_shape_system.shapes.push_back(shape);
    auto content = generate_go_shape_descriptors(one_shape_system);
    const auto type_name = pascal_identifier(shape.name);
    content = replace_all_copy(content, "ShapeDescriptors()", type_name + "ShapeDescriptors()");
    return "package backend\n\n" + content;
}

std::string go_shape_types_file()
{
    return generate_go_shape_types_file();
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

std::string go_shape_type_file(
    const IrShape& shape,
    std::string_view package_name = "shapes"
)
{
    std::ostringstream out;
    out << "package " << package_name << "\n\n";
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

std::string go_api_shape_descriptor_content(
    const IrShape& shape,
    std::string_view function_name
)
{
    IrSystem one_shape_system;
    one_shape_system.shapes.push_back(shape);
    auto content = generate_go_shape_descriptors(one_shape_system);
    content = replace_all_copy(content, "ShapeDescriptors()", std::string{function_name});
    content = replace_all_copy(content, "[]ShapeDescriptor", "[]common.ShapeDescriptor");
    content = replace_all_copy(content, "[]FieldDescriptor", "[]common.FieldDescriptor");
    content = replace_all_copy(content, "FieldType", "common.FieldType");
    return content;
}

const IrField* go_find_entity_field(
    const IrEntity& entity,
    const std::string& field_name
)
{
    const auto it = std::find_if(
        entity.fields.begin(), entity.fields.end(),
        [&](const IrField& field) { return field.name == field_name; }
    );
    return it == entity.fields.end() ? nullptr : &*it;
}

std::string go_entity_shape_field_descriptor_expr(
    const IrEntity& entity,
    const IrShape& shape,
    const IrField& field
)
{
    auto expression = go_shape_field_descriptor_expr(shape.name, field);
    expression = replace_all_copy(expression, "FieldType", "common.FieldType");
    if (go_find_entity_field(entity, field.name) == nullptr)
    {
        if (field.name == go_entity_plural_api_field_name(entity.name))
        {
            const auto field_type = replace_all_copy(
                go_field_type_enum_expr(field.type), "FieldType", "common.FieldType"
            );
            return "{Name: entityconstants." + go_entity_plural_name_constant_name(entity.name) +
                   ", Type: " + field_type +
                   ", TypeName: " + go_shape_field_type_name_constant_name(shape.name, field.name) +
                   ", Required: " + (go_field_required(field.type) ? "true" : "false") + "}";
        }
        return expression;
    }
    expression = replace_all_copy(
        expression, go_shape_field_constant_name(shape.name, field.name),
        "entityconstants." + go_entity_field_constant_name(entity.name, field.name)
    );
    expression = replace_all_copy(
        expression, go_shape_field_type_name_constant_name(shape.name, field.name),
        "entityconstants." + go_entity_field_type_name_constant_name(entity.name, field.name)
    );
    return expression;
}

std::string go_entity_api_shape_descriptor_content(
    const IrEntity& entity,
    const IrShape& shape,
    std::string_view function_name
)
{
    std::ostringstream out;
    out << "const (\n";
    out << "\t" << go_shape_name_constant_name(shape.name) << " = " << go_string(shape.name)
        << "\n";
    for (const auto& field : shape.fields)
    {
        if (go_find_entity_field(entity, field.name) != nullptr)
        {
            continue;
        }
        if (field.name == go_entity_plural_api_field_name(entity.name))
        {
            out << "\t" << go_shape_field_type_name_constant_name(shape.name, field.name) << " = "
                << go_string(field.type) << "\n";
            continue;
        }
        out << "\t" << go_shape_field_constant_name(shape.name, field.name) << " = "
            << go_string(field.name) << "\n";
        out << "\t" << go_shape_field_type_name_constant_name(shape.name, field.name) << " = "
            << go_string(field.type) << "\n";
    }
    out << ")\n\n";
    out << "func " << function_name << " []common.ShapeDescriptor {\n";
    out << "\treturn []common.ShapeDescriptor{\n";
    out << "\t\t{\n";
    out << "\t\t\tName: " << go_shape_name_constant_name(shape.name) << ",\n";
    out << "\t\t\tFields: []common.FieldDescriptor{\n";
    for (const auto& field : shape.fields)
    {
        out << "\t\t\t\t" << go_entity_shape_field_descriptor_expr(entity, shape, field) << ",\n";
    }
    out << "\t\t\t},\n";
    out << "\t\t},\n";
    out << "\t}\n";
    out << "}\n\n";
    return out.str();
}

bool go_entity_api_shapes_use_entity_constants(
    const IrEntity& entity,
    const std::vector<IrShape>& shapes
)
{
    return std::any_of(
        shapes.begin(), shapes.end(),
        [&](const IrShape& shape)
        {
            return std::any_of(
                shape.fields.begin(), shape.fields.end(),
                [&](const IrField& field)
                {
                    return go_find_entity_field(entity, field.name) != nullptr ||
                           field.name == go_entity_plural_api_field_name(entity.name);
                }
            );
        }
    );
}

bool go_api_path_uses_entity_constants(
    const IrSystem& system,
    const IrApi& api
);

std::string go_api_optional_path_expr(
    const IrSystem& system,
    const IrApi& api,
    std::string_view string_ptr
);

std::string go_api_shape_type_file(const IrShape& shape)
{
    std::ostringstream out;
    out << go_shape_type_file(shape);
    out << "\n";
    if (!go_shape_uses_json(shape))
    {
        out = std::ostringstream{};
        out << "package shapes\n\n";
        out << "import common \"statespec-generated/common/backend\"\n\n";
        out << "type " << pascal_identifier(shape.name) << " struct {\n";
        for (const auto& field : shape.fields)
        {
            out << "\t" << pascal_identifier(field.name) << " " << go_shape_field_type(field.type)
                << " `json:\"" << field.name << "\"`\n";
        }
        out << "}\n\n";
    }
    else
    {
        auto content = out.str();
        content = replace_all_copy(
            content, "import common \"statespec-generated/common/backend\"",
            "import common \"statespec-generated/common/backend\""
        );
        out = std::ostringstream{};
        out << content << "\n";
    }
    out << go_api_shape_descriptor_content(
        shape, pascal_identifier(shape.name) + "ShapeDescriptors()"
    );
    return out.str();
}

std::string go_entity_api_shapes_file(
    const TemplatePackage& templates,
    const IrEntity& entity,
    const std::vector<IrShape>& shapes,
    std::string_view package_name
)
{
    const auto uses_entity_constants = go_entity_api_shapes_use_entity_constants(entity, shapes);
    std::ostringstream imports;
    if (uses_entity_constants)
    {
        imports << "import (\n";
        imports << "\tcommon \"statespec-generated/common/backend\"\n";
        imports << "\tentityconstants \"statespec-generated/common/entities/"
                << snake_identifier(entity.name) << "\"\n";
        imports << ")\n\n";
    }
    else
    {
        imports << "import common \"statespec-generated/common/backend\"\n\n";
    }
    std::ostringstream shape_type_declarations;
    std::ostringstream shape_descriptor_functions;
    for (const auto& shape : shapes)
    {
        shape_type_declarations << "type " << pascal_identifier(shape.name) << " struct {\n";
        for (const auto& field : shape.fields)
        {
            shape_type_declarations << "\t" << pascal_identifier(field.name) << " "
                                    << go_shape_field_type(field.type);
            if (field.name != go_entity_plural_api_field_name(entity.name))
            {
                shape_type_declarations << " `json:\"" << field.name << "\"`";
            }
            shape_type_declarations << "\n";
        }
        shape_type_declarations << "}\n\n";
        shape_descriptor_functions << go_entity_api_shape_descriptor_content(
            entity, shape, pascal_identifier(shape.name) + "ShapeDescriptors()"
        );
    }
    return templates.render(
        "api/backend/entities/shapes.go.tmpl",
        TemplateRenderer::Values{
            {"package_name", std::string{package_name}},
            {"imports", imports.str()},
            {"shape_type_declarations", shape_type_declarations.str()},
            {"shape_descriptor_functions", shape_descriptor_functions.str()},
        }
    );
}

std::string go_api_name_constant_name(const std::string& api_name)
{
    return pascal_identifier(api_name) + "APIName";
}

std::string go_api_route_name_constant_name(
    const std::string& api_server_name,
    const std::string& api_name
)
{
    return pascal_identifier(api_server_name) + pascal_identifier(api_name) + "RouteName";
}

std::string go_api_shape_name_constant_name(const std::string& shape_name)
{
    return pascal_identifier(shape_name) + "APIShapeName";
}

std::string go_api_response_envelope_constant_name(const std::string& entity_name)
{
    return pascal_identifier(entity_name) + "ListResponseEnvelopeName";
}

std::string go_api_server_name_constant_name(const std::string& api_server_name)
{
    return pascal_identifier(api_server_name) + "ServerName";
}

std::string go_api_server_concurrency_constant_name(const std::string& api_server_name)
{
    return pascal_identifier(api_server_name) + "ServerConcurrency";
}

std::string go_api_server_constants_file(const IrApiServer& api_server)
{
    std::ostringstream out;
    out << "package " << snake_identifier(api_server.name) << "\n\n";
    out << "const (\n";
    out << "\t" << go_api_server_name_constant_name(api_server.name) << " = "
        << go_string(api_server.name) << "\n";
    out << "\t" << go_api_server_concurrency_constant_name(api_server.name) << " = "
        << api_server.concurrency.value_or(1) << "\n";
    out << ")\n";
    return out.str();
}

std::string go_api_server_catalog_file(
    const TemplatePackage& templates,
    const IrSystem& system,
    const IrApiServer& api_server
)
{
    const auto entity_domains = crud_api_handler_domains_go(api_handler_domains(system));
    std::set<std::string> entity_api_names;
    std::vector<ApiHandlerDomain> served_domains;
    for (const auto& domain : entity_domains)
    {
        ApiHandlerDomain served_domain{domain.name, {}};
        for (const auto& api : domain.apis)
        {
            entity_api_names.insert(api.name);
            if (std::find(api_server.serves.begin(), api_server.serves.end(), api.name) !=
                api_server.serves.end())
            {
                served_domain.apis.push_back(api);
            }
        }
        if (!served_domain.apis.empty())
        {
            served_domains.push_back(std::move(served_domain));
        }
    }
    std::ostringstream entity_imports;
    std::ostringstream entity_append_calls;
    std::ostringstream manual_served_api_entries;
    for (const auto& domain : served_domains)
    {
        entity_imports << "\tserver" << snake_identifier(domain.name)
                       << " \"statespec-generated/api/backend/servers/"
                       << snake_identifier(api_server.name) << "/entities/"
                       << snake_identifier(domain.name) << "\"\n";
        entity_append_calls << "\tserver" << snake_identifier(domain.name)
                            << ".AppendAPIServerNames(&serves)\n";
    }
    for (const auto& served_api : api_server.serves)
    {
        if (entity_api_names.contains(served_api))
        {
            continue;
        }
        manual_served_api_entries << "\tserves = append(serves, " << go_string(served_api) << ")\n";
    }
    return templates.render(
        "api/backend/servers/catalog.go.tmpl",
        TemplateRenderer::Values{
            {"api_server_package", snake_identifier(api_server.name)},
            {"server_entity_imports", entity_imports.str()},
            {"server_entity_append_calls", entity_append_calls.str()},
            {"manual_served_api_entries", manual_served_api_entries.str()},
            {"api_server_name_constant", go_api_server_name_constant_name(api_server.name)},
            {"api_server_concurrency_constant",
             go_api_server_concurrency_constant_name(api_server.name)},
            {"composite_open", "{{"},
            {"composite_close", "}}"},
        }
    );
}

std::string go_api_server_entity_catalog_file(
    const TemplatePackage& templates,
    const IrApiServer& api_server,
    const ApiHandlerDomain& domain
)
{
    std::ostringstream cases;
    bool first = true;
    for (const auto& api : domain.apis)
    {
        if (std::find(api_server.serves.begin(), api_server.serves.end(), api.name) ==
            api_server.serves.end())
        {
            continue;
        }
        if (!first)
        {
            cases << ", ";
        }
        first = false;
        cases << snake_identifier(domain.name) << "." << go_api_name_constant_name(api.name);
    }
    return templates.render(
        "api/backend/servers/entity_catalog.go.tmpl",
        TemplateRenderer::Values{
            {"entity_package", snake_identifier(domain.name)},
            {"served_api_cases", cases.str()},
        }
    );
}

std::string go_entity_api_constants_file(
    const IrSystem& system,
    const IrEntity& entity
)
{
    const auto shapes = go_entity_api_shapes(system, entity.name);
    std::vector<IrApi> apis;
    for (const auto& domain : crud_api_handler_domains_go(api_handler_domains(system)))
    {
        if (domain.name == entity.name)
        {
            apis = domain.apis;
            break;
        }
    }

    std::ostringstream out;
    out << "package " << snake_identifier(entity.name) << "\n\n";
    out << "import entityconstants \"statespec-generated/common/entities/"
        << snake_identifier(entity.name) << "\"\n\n";
    out << "const (\n";
    for (const auto& api : apis)
    {
        out << "\t" << go_api_name_constant_name(api.name) << " = " << go_string(api.name) << "\n";
    }
    for (const auto& api_server : system.api_servers)
    {
        for (const auto& api : apis)
        {
            if (!go_api_server_serves(api_server, api.name))
            {
                continue;
            }
            out << "\t" << go_api_route_name_constant_name(api_server.name, api.name) << " = "
                << go_string(api_server.name + "." + api.name) << "\n";
        }
    }
    for (const auto& shape : shapes)
    {
        out << "\t" << go_api_shape_name_constant_name(shape.name) << " = " << go_string(shape.name)
            << "\n";
    }
    out << "\t" << go_api_response_envelope_constant_name(entity.name) << " = entityconstants."
        << go_entity_plural_name_constant_name(entity.name) << "\n";
    out << ")\n";
    return out.str();
}

std::string go_entity_api_catalog_file(
    const IrSystem& system,
    const IrEntity& entity
)
{
    const auto shapes = go_entity_api_shapes(system, entity.name);
    std::vector<IrApi> apis;
    for (const auto& domain : crud_api_handler_domains_go(api_handler_domains(system)))
    {
        if (domain.name == entity.name)
        {
            apis = domain.apis;
            break;
        }
    }

    std::ostringstream out;
    out << "package " << snake_identifier(entity.name) << "\n\n";
    out << "import (\n";
    out << "\tcommon \"statespec-generated/common/backend\"\n";
    out << "\tdescriptortypes \"statespec-generated/common/backend/descriptortypes\"\n";
    out << ")\n\n";
    out << "func EntityShapeDescriptors() []common.ShapeDescriptor {\n";
    out << "\tresult := []common.ShapeDescriptor{}\n";
    for (const auto& shape : shapes)
    {
        out << "\tresult = append(result, " << pascal_identifier(shape.name)
            << "ShapeDescriptors()...)\n";
    }
    out << "\treturn result\n";
    out << "}\n\n";
    out << "func NewHandlerRegistry(backend common.Backend) "
        << go_api_handler_domain_type_name(entity.name) << " {\n";
    out << "\treturn New" << go_api_handler_domain_type_name(entity.name) << "(backend)\n";
    out << "}\n\n";
    out << "func EntityAPIDescriptors() []descriptortypes.ApiDescriptor {\n";
    out << "\tresult := []descriptortypes.ApiDescriptor{}\n";
    for (const auto& api : apis)
    {
        out << "\tresult = append(result, " << go_api_descriptor_function_name(api) << "()...)\n";
    }
    out << "\treturn result\n";
    out << "}\n\n";
    out << "func EntityAPINames() []string {\n";
    out << "\treturn []string{\n";
    for (const auto& api : apis)
    {
        out << "\t\t" << go_api_name_constant_name(api.name) << ",\n";
    }
    out << "\t}\n";
    out << "}\n\n";
    out << "func EntityAPIRouteDescriptors() []descriptortypes.ApiRouteDescriptor {\n";
    out << "\tresult := []descriptortypes.ApiRouteDescriptor{}\n";
    for (const auto& api : apis)
    {
        out << "\tresult = append(result, " << go_api_route_descriptor_function_name(api)
            << "()...)\n";
    }
    out << "\treturn result\n";
    out << "}\n\n";
    out << "func HandlerEntrypoints() []string {\n";
    out << "\treturn []string{\n";
    for (const auto& api : apis)
    {
        out << "\t\t\"Handle" << pascal_identifier(api.name) << "\",\n";
    }
    out << "\t}\n";
    out << "}\n";
    return out.str();
}

std::string go_api_shape_alias_file(const IrSystem& system)
{
    std::ostringstream imports;
    std::ostringstream aliases;
    for (const auto& entity : system.entities)
    {
        const auto shapes = go_entity_api_shapes(system, entity.name);
        if (shapes.empty())
        {
            continue;
        }
        const auto package_alias = snake_identifier(entity.name);
        imports << "\t" << package_alias << " \"statespec-generated/api/backend/entities/"
                << snake_identifier(entity.name) << "\"\n";
        for (const auto& shape : shapes)
        {
            aliases << "type " << pascal_identifier(shape.name) << " = " << package_alias << "."
                    << pascal_identifier(shape.name) << "\n";
        }
    }
    if (aliases.str().empty())
    {
        return {};
    }
    return "package shapes\n\nimport (\n" + imports.str() + ")\n\n" + aliases.str();
}

std::string go_api_shape_descriptor_catalog_file(const IrSystem& system)
{
    std::ostringstream imports;
    std::ostringstream aggregation;
    std::set<std::string> imported_entities;
    for (const auto& shape : go_shapes_matching(system, true).shapes)
    {
        const auto owner = go_entity_api_shape_owner(system, shape.name);
        if (owner.has_value())
        {
            const auto package_alias = snake_identifier(*owner);
            if (imported_entities.insert(package_alias).second)
            {
                imports << "\t" << package_alias << " \"statespec-generated/api/backend/entities/"
                        << package_alias << "\"\n";
            }
            continue;
        }
        else
        {
            aggregation << "\tdescriptors = append(descriptors, " << pascal_identifier(shape.name)
                        << "ShapeDescriptors()...)\n";
        }
    }
    for (const auto& package_alias : imported_entities)
    {
        aggregation << "\tdescriptors = append(descriptors, " << package_alias
                    << ".EntityShapeDescriptors()...)\n";
    }
    std::ostringstream out;
    out << "package shapes\n\n";
    out << "import (\n";
    out << "\tcommon \"statespec-generated/common/backend\"\n";
    out << imports.str();
    out << ")\n\n";
    out << "func ShapeDescriptors() []common.ShapeDescriptor {\n";
    out << "\tvar descriptors []common.ShapeDescriptor\n";
    out << aggregation.str();
    out << "\treturn descriptors\n";
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
        if (go_api_uses_shape(system, shape.name))
        {
            continue;
        }
        add_go_raw_common_file(
            result, options, "backend/shapes/" + snake_identifier(shape.name) + ".go",
            go_shape_type_file(shape)
        );
    }
}

void add_go_raw_api_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    std::string_view relative_output_path,
    std::string content
);

void add_go_api_shape_type_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system
)
{
    for (const auto& shape : system.shapes)
    {
        if (!go_api_uses_shape(system, shape.name))
        {
            continue;
        }
        if (go_entity_api_shape_owner(system, shape.name).has_value())
        {
            continue;
        }
        add_go_raw_api_file(
            result, options, "api/backend/shapes/" + snake_identifier(shape.name) + ".go",
            go_api_shape_type_file(shape)
        );
    }
    const auto aliases = go_api_shape_alias_file(system);
    if (!aliases.empty())
    {
        add_go_raw_api_file(result, options, "api/backend/shapes/entities.go", aliases);
    }
    add_go_raw_api_file(
        result, options, "api/backend/shapes/catalog.go",
        go_api_shape_descriptor_catalog_file(system)
    );
    for (const auto& entity : system.entities)
    {
        const auto shapes = go_entity_api_shapes(system, entity.name);
        if (shapes.empty())
        {
            continue;
        }
        add_go_raw_api_file(
            result, options,
            "api/backend/entities/" + snake_identifier(entity.name) + "/constants.go",
            go_entity_api_constants_file(system, entity)
        );
        add_go_raw_api_file(
            result, options, "api/backend/entities/" + snake_identifier(entity.name) + "/shapes.go",
            go_entity_api_shapes_file(templates, entity, shapes, snake_identifier(entity.name))
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
    const auto common_shapes = go_shapes_matching(system, false);
    if (common_shapes.shapes.empty())
    {
        return;
    }
    add_generated_template_file(
        result, options.output_dir, templates, generated_template_path("shape_descriptors.go.tmpl"),
        common_artifact_path("backend/shape_descriptors.go"), diagnostics,
        GeneratedArtifactTier::Common, go_shape_descriptor_values(common_shapes)
    );
    for (const auto& shape : common_shapes.shapes)
    {
        add_go_raw_common_file(
            result, options, "backend/" + snake_identifier(shape.name) + "_shape_descriptors.go",
            go_shape_descriptor_file(shape)
        );
    }
}

void add_go_entity_descriptor_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const TemplatePackage& templates,
    const IrSystem& system,
    DiagnosticBag&
)
{
    auto entity_uses_gc = [](const IrEntity& entity)
    {
        return std::any_of(
            entity.states.begin(), entity.states.end(),
            [](const IrState& state) { return state.garbage_collection.has_value(); }
        );
    };

    for (const auto& entity : system.entities)
    {
        const auto entity_dir = "entities/" + snake_identifier(entity.name) + "/";
        add_go_raw_common_file(
            result, options, entity_dir + "constants.go",
            go_entity_constants_file(templates, entity)
        );
        add_go_raw_common_file(
            result, options, entity_dir + "model.go",
            go_entity_centered_facade_file(templates, entity, "model")
        );
        add_go_raw_common_file(
            result, options, entity_dir + "persistence.go",
            go_entity_centered_facade_file(templates, entity, "persistence")
        );
        add_go_raw_common_file(
            result, options, entity_dir + "schema.go",
            go_entity_centered_facade_file(templates, entity, "schema")
        );
        if (entity_uses_gc(entity))
        {
            add_go_raw_common_file(
                result, options, entity_dir + "gc.go", go_entity_gc_descriptor_file(entity)
            );
        }
    }
}

std::string go_workflow_descriptor_support_file()
{
    std::ostringstream out;
    out << "package workflows\n\n";
    out << "import \"time\"\n\n";
    out << "type WorkflowStepDefinition struct {\n";
    out << "\tName string\n";
    out << "\tExpectedExecutionTime time.Duration\n";
    out << "\tMaxRetries uint32\n";
    out << "}\n\n";
    out << "type WorkflowDefinition struct {\n";
    out << "\tWorkflowName string\n";
    out << "\tWorkflowVersion int64\n";
    out << "\tStartStep string\n";
    out << "\tExpectedExecutionTime time.Duration\n";
    out << "\tSingleton bool\n";
    out << "\tSteps []WorkflowStepDefinition\n";
    out << "}\n";
    return out.str();
}

void add_go_workflow_descriptor_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    if (system.workflows.empty())
    {
        return;
    }
    add_go_raw_common_file(
        result, options, "backend/workflows/workflows.go", go_workflow_descriptor_support_file()
    );
    for (const auto& workflow : system.workflows)
    {
        add_go_raw_common_file(
            result, options, "backend/workflows/" + snake_identifier(workflow.name) + ".go",
            generate_go_workflow_descriptor(workflow)
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
    const auto usage = runtime_domain_usage(system);
    add_go_descriptor_module_artifact(
        result, options, templates, "backend/descriptors/core.go", "descriptors",
        "core descriptors", diagnostics
    );
    add_go_raw_common_file(
        result, options, "backend/values_enums_descriptors.go",
        "package backend\n\n" + generate_go_value_enum_descriptors(system)
    );
    add_go_raw_common_file(
        result, options, "backend/events.go",
        go_event_helpers_file(system) + "\n" + generate_go_event_descriptors(system)
    );
    add_go_raw_common_file(
        result, options, "backend/external_systems.go", go_external_systems_file(system, templates)
    );
    add_go_raw_common_file(
        result, options, "backend/policy_descriptors.go",
        "package backend\n\n" + generate_go_policy_descriptors(system)
    );
    const auto common_shapes = go_shapes_matching(system, false);
    if (!common_shapes.shapes.empty())
    {
        add_go_descriptor_module_artifact(
            result, options, templates, "backend/descriptors/shapes.go", "descriptors",
            "shape descriptors", diagnostics
        );
    }
    add_go_shape_descriptor_artifact(result, options, templates, system, diagnostics);
    add_go_descriptor_module_artifact(
        result, options, templates, "backend/descriptors/runtime.go", "descriptors",
        "runtime descriptors", diagnostics
    );
    if (usage.uses_observability)
    {
        add_go_raw_common_file(
            result, options, "backend/observability_definitions.go",
            "package backend\n\n" + generate_go_observability_descriptors(system)
        );
    }
    if (usage.uses_feature_flags || usage.uses_queues || usage.uses_leases || usage.uses_workflows)
    {
        std::ostringstream imports;
        if (usage.uses_queues || usage.uses_leases || usage.uses_workflows)
        {
            imports << "import (\n";
            if (usage.uses_queues || usage.uses_leases)
            {
                imports << "\t\"time\"\n";
            }
            if (usage.uses_workflows)
            {
                imports << "\tworkflows \"statespec-generated/common/backend/workflows\"\n";
            }
            imports << ")\n\n";
        }
        add_go_raw_common_file(
            result, options, "backend/runtime_definitions.go",
            "package backend\n\n" + imports.str() + generate_go_feature_flag_descriptors(system) +
                generate_go_runtime_descriptors(system)
        );
    }
    for (const auto& [name, label] : go_runtime_registration_modules(system))
    {
        add_go_raw_common_file(
            result, options, "backend/runtime_registration_" + name + ".go",
            go_runtime_registration_module_file(templates, name)
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

void add_go_raw_worker_file(
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
            GeneratedArtifactTier::Worker,
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

std::string go_api_shape_package_for(
    const IrSystem& system,
    std::string_view shape_name
)
{
    const auto owner = go_entity_api_shape_owner(system, shape_name);
    if (owner.has_value())
    {
        return snake_identifier(*owner);
    }
    return "shapes";
}

std::string go_api_optional_shape_expr(
    const IrSystem& system,
    const std::optional<std::string>& value,
    std::string_view string_ptr,
    const IrEntity* local_entity = nullptr
)
{
    if (!value.has_value())
    {
        return "nil";
    }
    if (go_find_shape(system, *value) == nullptr)
    {
        return std::string{string_ptr} + "(" + go_string(*value) + ")";
    }
    const auto owner = go_entity_api_shape_owner(system, *value);
    if (owner.has_value() && local_entity != nullptr && *owner == local_entity->name)
    {
        return std::string{string_ptr} + "(" + go_shape_name_constant_name(*value) + ")";
    }
    return std::string{string_ptr} + "(" + go_api_shape_package_for(system, *value) + "." +
           go_shape_name_constant_name(*value) + ")";
}

const IrEntity* go_api_entity(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api.entity.has_value())
    {
        return nullptr;
    }
    const auto it = std::find_if(
        system.entities.begin(), system.entities.end(),
        [&](const IrEntity& entity) { return entity.name == *api.entity; }
    );
    return it == system.entities.end() ? nullptr : &*it;
}

bool go_entity_has_field(
    const IrEntity& entity,
    std::string_view field_name
)
{
    return std::any_of(
        entity.fields.begin(), entity.fields.end(),
        [&](const IrField& field) { return field.name == field_name; }
    );
}

bool go_api_path_uses_entity_constants(
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = go_api_entity(system, api);
    if (entity == nullptr || !api.path.has_value())
    {
        return false;
    }
    for (std::size_t pos = 0; (pos = api.path->find('{', pos)) != std::string::npos;)
    {
        const auto end = api.path->find('}', pos + 1);
        if (end == std::string::npos)
        {
            return false;
        }
        if (go_entity_has_field(
                *entity, std::string_view{*api.path}.substr(pos + 1, end - pos - 1)
            ))
        {
            return true;
        }
        pos = end + 1;
    }
    return false;
}

std::string go_api_optional_path_expr(
    const IrSystem& system,
    const IrApi& api,
    std::string_view string_ptr
)
{
    if (!api.path.has_value())
    {
        return "nil";
    }
    const auto* entity = go_api_entity(system, api);
    if (entity == nullptr || !go_api_path_uses_entity_constants(system, api))
    {
        return std::string{string_ptr} + "(" + go_string(*api.path) + ")";
    }

    std::vector<std::string> parts;
    std::size_t cursor = 0;
    for (std::size_t pos = 0; (pos = api.path->find('{', cursor)) != std::string::npos;)
    {
        const auto end = api.path->find('}', pos + 1);
        if (end == std::string::npos)
        {
            return std::string{string_ptr} + "(" + go_string(*api.path) + ")";
        }
        const auto field_name = api.path->substr(pos + 1, end - pos - 1);
        parts.push_back(go_string(api.path->substr(cursor, pos - cursor + 1)));
        if (go_entity_has_field(*entity, field_name))
        {
            parts.push_back(
                snake_identifier(entity->name) + "constants." +
                go_entity_field_constant_name(entity->name, field_name)
            );
        }
        else
        {
            parts.push_back(go_string(field_name));
        }
        parts.push_back(go_string("}"));
        cursor = end + 1;
    }
    if (cursor < api.path->size())
    {
        parts.push_back(go_string(api.path->substr(cursor)));
    }
    std::ostringstream expr;
    expr << string_ptr << "(";
    for (std::size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
        {
            expr << " + ";
        }
        expr << parts[i];
    }
    expr << ")";
    return expr.str();
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
    const auto* local_entity = go_api_entity(system, api);
    out << "package "
        << (local_entity == nullptr ? std::string{"descriptors"}
                                    : snake_identifier(local_entity->name))
        << "\n\n";
    out << "import (\n";
    out << "\tdescriptortypes \"statespec-generated/common/backend/descriptortypes\"\n";
    if (local_entity != nullptr && go_api_path_uses_entity_constants(system, api))
    {
        out << "\t" << snake_identifier(local_entity->name)
            << "constants \"statespec-generated/common/entities/"
            << snake_identifier(local_entity->name) << "\"\n";
    }
    std::set<std::string> imports;
    auto add_shape_import = [&](const std::optional<std::string>& shape_name)
    {
        if (!shape_name.has_value() || go_find_shape(system, *shape_name) == nullptr)
        {
            return;
        }
        const auto owner = go_entity_api_shape_owner(system, *shape_name);
        if (owner.has_value())
        {
            if (local_entity != nullptr && *owner == local_entity->name)
            {
                return;
            }
            const auto package_alias = snake_identifier(*owner);
            imports.insert(
                "\t" + package_alias + " \"statespec-generated/api/backend/entities/" +
                package_alias + "\""
            );
        }
        else
        {
            imports.insert("\tshapes \"statespec-generated/api/backend/shapes\"");
        }
    };
    add_shape_import(api.input);
    add_shape_import(api.output);
    for (const auto& import : imports)
    {
        out << import << "\n";
    }
    out << ")\n\n";
    out << "func " << string_ptr << "(value string) *string { return &value }\n\n";
    out << "func " << go_api_descriptor_function_name(api)
        << "() []descriptortypes.ApiDescriptor {\n";
    out << "\treturn []descriptortypes.ApiDescriptor{\n";
    out << "\t\t{\n";
    out << "\t\t\tName: " << go_string(api.name) << ",\n";
    out << "\t\t\tMethod: " << optional_string(api.method) << ",\n";
    out << "\t\t\tPath: " << go_api_optional_path_expr(system, api, string_ptr) << ",\n";
    out << "\t\t\tInput: "
        << go_api_optional_shape_expr(system, api.input, string_ptr, local_entity) << ",\n";
    out << "\t\t\tOutput: "
        << go_api_optional_shape_expr(system, api.output, string_ptr, local_entity) << ",\n";
    out << "\t\t\tError: " << optional_string(api.error) << ",\n";
    out << "\t\t\tStartsWorkflow: " << optional_string(api.starts_workflow) << ",\n";
    out << "\t\t\tEnqueues: " << optional_string(api.enqueues) << ",\n";
    out << "\t\t},\n";
    out << "\t}\n";
    out << "}\n\n";
    out << "func " << go_api_route_descriptor_function_name(api)
        << "() []descriptortypes.ApiRouteDescriptor {\n";
    out << "\treturn []descriptortypes.ApiRouteDescriptor{\n";
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
        out << "\t\t\tPath: " << go_api_optional_path_expr(system, api, string_ptr) << ",\n";
        out << "\t\t\tInput: "
            << go_api_optional_shape_expr(system, api.input, string_ptr, local_entity) << ",\n";
        out << "\t\t\tOutput: "
            << go_api_optional_shape_expr(system, api.output, string_ptr, local_entity) << ",\n";
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
        std::string path = "api/backend/descriptors/" + snake_identifier(api.name) + ".go";
        if (const auto* entity = go_api_entity(system, api); entity != nullptr)
        {
            path = "api/backend/entities/" + snake_identifier(entity->name) + "/" +
                   snake_identifier(api.name) + "_descriptor.go";
        }
        add_go_raw_api_file(result, options, path, go_api_descriptor_module(system, api));
    }
}

std::string go_api_descriptor_catalog_file(const IrSystem& system)
{
    const auto entity_domains = crud_api_handler_domains_go(api_handler_domains(system));
    std::set<std::string> entity_api_names;
    std::ostringstream out;
    out << "package descriptors\n\n";
    out << "import (\n";
    out << "\tdescriptortypes \"statespec-generated/common/backend/descriptortypes\"\n";
    for (const auto& domain : entity_domains)
    {
        out << "\t" << snake_identifier(domain.name)
            << " \"statespec-generated/api/backend/entities/" << snake_identifier(domain.name)
            << "\"\n";
        for (const auto& api : domain.apis)
        {
            entity_api_names.insert(api.name);
        }
    }
    out << ")\n\n";
    out << "func ApiDescriptors() []descriptortypes.ApiDescriptor {\n";
    out << "\tresult := []descriptortypes.ApiDescriptor{}\n";
    for (const auto& domain : entity_domains)
    {
        out << "\tresult = append(result, " << snake_identifier(domain.name)
            << ".EntityAPIDescriptors()...)\n";
    }
    for (const auto& api : system.apis)
    {
        if (entity_api_names.contains(api.name))
        {
            continue;
        }
        out << "\tresult = append(result, " << go_api_descriptor_function_name(api) << "()...)\n";
    }
    out << "\treturn result\n";
    out << "}\n\n";
    out << "func ApiRouteDescriptors() []descriptortypes.ApiRouteDescriptor {\n";
    out << "\tresult := []descriptortypes.ApiRouteDescriptor{}\n";
    for (const auto& domain : entity_domains)
    {
        out << "\tresult = append(result, " << snake_identifier(domain.name)
            << ".EntityAPIRouteDescriptors()...)\n";
    }
    for (const auto& api : system.apis)
    {
        if (entity_api_names.contains(api.name))
        {
            continue;
        }
        out << "\tresult = append(result, " << go_api_route_descriptor_function_name(api)
            << "()...)\n";
    }
    out << "\treturn result\n";
    out << "}\n";
    return out.str();
}

std::string go_api_descriptors_file(const IrSystem& system)
{
    std::ostringstream out;
    out << "package backend\n\n";
    out << "import (\n";
    out << "\tdescriptortypes \"statespec-generated/common/backend/descriptortypes\"\n";
    out << "\tdescriptors \"statespec-generated/api/backend/descriptors\"\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "\t" << lower_camel_identifier(api_server.name) << "server "
            << "\"statespec-generated/api/backend/servers/" << snake_identifier(api_server.name)
            << "\"\n";
    }
    out << ")\n\n";
    out << "type APITierDescriptor = descriptortypes.ApiDescriptor\n";
    out << "type APITierServerDescriptor = descriptortypes.ApiServerDescriptor\n\n";
    out << "func APITierDescriptors() []descriptortypes.ApiDescriptor {\n";
    out << "\treturn descriptors.ApiDescriptors()\n";
    out << "}\n\n";
    out << "func APITierServerDescriptors() []descriptortypes.ApiServerDescriptor {\n";
    out << "\tresult := []descriptortypes.ApiServerDescriptor{}\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "\tresult = append(result, " << lower_camel_identifier(api_server.name)
            << "server.ApiServerDescriptors()...)\n";
    }
    out << "\treturn result\n";
    out << "}\n";
    return out.str();
}

std::string worker_descriptor_string_ptr_expr(const std::optional<std::string>& value)
{
    return value.has_value() ? "workerDescriptorStringPtr(" + go_string(*value) + ")" : "nil";
}

std::string go_worker_descriptor_module_file(const IrWorker& worker)
{
    const auto pascal = pascal_identifier(worker.name);
    std::ostringstream out;
    out << "package descriptors\n\n";
    out << "import descriptortypes \"statespec-generated/common/backend/descriptortypes\"\n\n";
    out << "func " << pascal << "WorkerDescriptor() descriptortypes.WorkerDescriptor {\n";
    out << "\treturn descriptortypes.WorkerDescriptor{\n";
    out << "\t\tName: " << go_string(worker.name) << ",\n";
    out << "\t\tSingleton: " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "\t\tLease: " << worker_descriptor_string_ptr_expr(worker.lease) << ",\n";
    out << "\t\tPolls: " << worker_descriptor_string_ptr_expr(worker.polls) << ",\n";
    out << "\t\tExecutes: " << worker_descriptor_string_ptr_expr(worker.executes) << ",\n";
    out << "\t\tConcurrency: " << worker.concurrency.value_or(1) << ",\n";
    out << "\t}\n";
    out << "}\n\n";
    out << "func " << pascal << "WorkerContext() descriptortypes.WorkerContext {\n";
    out << "\treturn descriptortypes.WorkerContext{\n";
    out << "\t\tWorkerName: " << go_string(worker.name) << ",\n";
    out << "\t\tSingleton: " << (worker.singleton.value_or(false) ? "true" : "false") << ",\n";
    out << "\t\tLease: " << worker_descriptor_string_ptr_expr(worker.lease) << ",\n";
    out << "\t\tPolls: " << worker_descriptor_string_ptr_expr(worker.polls) << ",\n";
    out << "\t\tExecutes: " << worker_descriptor_string_ptr_expr(worker.executes) << ",\n";
    out << "\t\tConcurrency: " << worker.concurrency.value_or(1) << ",\n";
    out << "\t}\n";
    out << "}\n";
    return out.str();
}

std::string go_worker_descriptor_catalog_file(const IrSystem& system)
{
    std::ostringstream out;
    out << "package descriptors\n\n";
    out << "import descriptortypes \"statespec-generated/common/backend/descriptortypes\"\n\n";
    out << "func workerDescriptorStringPtr(value string) *string { return &value }\n\n";
    out << "func WorkerDescriptors() []descriptortypes.WorkerDescriptor {\n";
    out << "\tdescriptors := []descriptortypes.WorkerDescriptor{}\n";
    for (const auto& worker : system.workers)
    {
        out << "\tdescriptors = append(descriptors, " << pascal_identifier(worker.name)
            << "WorkerDescriptor())\n";
    }
    out << "\treturn descriptors\n";
    out << "}\n\n";
    out << "func WorkerContexts() []descriptortypes.WorkerContext {\n";
    out << "\tcontexts := []descriptortypes.WorkerContext{}\n";
    for (const auto& worker : system.workers)
    {
        out << "\tcontexts = append(contexts, " << pascal_identifier(worker.name)
            << "WorkerContext())\n";
    }
    out << "\treturn contexts\n";
    out << "}\n";
    return out.str();
}

} // namespace

#include "generator_go_artifact_tier_builders.inc"

} // namespace statespec
