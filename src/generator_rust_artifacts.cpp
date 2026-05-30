#include "generator_rust_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_entity_index_helpers.hpp"
#include "generator_rust_artifact_manifest.hpp"
#include "generator_rust_descriptor_areas.hpp"
#include "generator_rust_descriptor_support.hpp"
#include "generator_rust_descriptors.hpp"
#include "generator_support.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

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

TemplateRenderer::Values rust_api_main_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    if (!usage.uses_entity_gc)
    {
        return TemplateRenderer::Values{
            {"api_main_entity_gc_import", ""},
            {"api_main_entity_gc_backend_clone", ""},
            {"api_main_entity_gc_registration", ""},
        };
    }
    return TemplateRenderer::Values{
        {"api_main_entity_gc_import", "use crate::api_entity_gc_catalog::entity_gc_descriptors as "
                                      "api_entity_gc_descriptors;\n"
                                      "use "
                                      "crate::runtime_entity_gc_registration::register_entity_gc_"
                                      "workers;\n"},
        {"api_main_entity_gc_backend_clone", "    let gc_backend = backend.clone();\n"},
        {"api_main_entity_gc_registration", "    register_entity_gc_workers(\n"
                                            "        |task| process.add_entity_gc_worker(task),\n"
                                            "        std::sync::Arc::new(gc_backend),\n"
                                            "        api_entity_gc_descriptors(),\n"
                                            "    )?;\n"},
    };
}

TemplateRenderer::Values rust_worker_main_values(const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    if (!usage.uses_entity_gc)
    {
        return TemplateRenderer::Values{
            {"worker_main_entity_gc_import", ""},
            {"worker_main_entity_gc_backend_clone", ""},
            {"worker_main_entity_gc_registration", ""},
        };
    }
    return TemplateRenderer::Values{
        {"worker_main_entity_gc_import",
         "use crate::runtime_entity_gc_registration::register_entity_gc_workers;\n"
         "use crate::worker_entity_gc_catalog::entity_gc_descriptors as "
         "worker_entity_gc_descriptors;\n"},
        {"worker_main_entity_gc_backend_clone", "    let gc_backend = backend.clone();\n"},
        {"worker_main_entity_gc_registration",
         "    register_entity_gc_workers(\n"
         "        |task| process.runtime.add_entity_gc_worker(task),\n"
         "        std::sync::Arc::new(gc_backend),\n"
         "        worker_entity_gc_descriptors(),\n"
         "    )?;\n"},
    };
}

std::vector<std::pair<
    std::string,
    std::string>>
rust_runtime_registration_modules(const IrSystem& system)
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

TemplateRenderer::Values rust_runtime_registration_module_values(
    const TemplatePackage& templates,
    std::string_view name,
    std::string_view label
)
{
    return TemplateRenderer::Values{
        {"descriptor_module_name", std::string{label}},
        {"descriptor_module_content",
         "use super::*;\n\n" + generate_rust_runtime_registration_domain(templates, name)}
    };
}

bool rust_api_uses_shape(
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

const IrShape* rust_find_shape(
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

IrSystem rust_shapes_matching(
    const IrSystem& system,
    bool api_contract_shapes
)
{
    auto filtered = system;
    filtered.shapes.clear();
    for (const auto& shape : system.shapes)
    {
        if (rust_api_uses_shape(system, shape.name) == api_contract_shapes)
        {
            filtered.shapes.push_back(shape);
        }
    }
    return filtered;
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

std::string rust_workflow_module_name(const IrWorkflow& workflow)
{
    return "workflow_" + snake_identifier(workflow.name);
}

std::string rust_worker_registry_module(const IrWorker& worker)
{
    std::ostringstream out;
    out << "use crate::descriptor_types::{WorkerContext, WorkerDescriptor};\n";
    out << "use crate::worker_contexts;\n";
    out << "use crate::worker_descriptors;\n\n";
    out << "pub fn find_worker_descriptor(worker_name: &str) -> Option<WorkerDescriptor> {\n";
    out << "    if worker_name == " << rust_string(worker.name) << " {\n";
    out << "        return worker_descriptors::worker_descriptors()\n";
    out << "            .into_iter()\n";
    out << "            .find(|worker| worker.name == worker_name);\n";
    out << "    }\n";
    out << "    None\n";
    out << "}\n\n";
    out << "pub fn find_worker_context(worker_name: &str) -> Option<WorkerContext> {\n";
    out << "    if worker_name == " << rust_string(worker.name) << " {\n";
    out << "        return worker_contexts::worker_contexts()\n";
    out << "            .into_iter()\n";
    out << "            .find(|context| context.worker_name == worker_name);\n";
    out << "    }\n";
    out << "    None\n";
    out << "}\n";
    return out.str();
}

std::string rust_worker_registry_facade(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& worker : system.workers)
    {
        out << "#[path = \"registry/" << snake_identifier(worker.name) << ".rs\"]\n";
        out << "mod registry_" << snake_identifier(worker.name) << ";\n";
    }
    out << "\npub use crate::descriptor_types::{WorkerContext, WorkerDescriptor};\n\n";
    out << "pub fn find_worker_descriptor(worker_name: &str) -> Option<WorkerDescriptor> {\n";
    for (const auto& worker : system.workers)
    {
        out << "    if let Some(worker) = registry_" << snake_identifier(worker.name)
            << "::find_worker_descriptor(worker_name) {\n";
        out << "        return Some(worker);\n";
        out << "    }\n";
    }
    out << "    None\n";
    out << "}\n\n";
    out << "pub fn find_worker_context(worker_name: &str) -> Option<WorkerContext> {\n";
    for (const auto& worker : system.workers)
    {
        out << "    if let Some(context) = registry_" << snake_identifier(worker.name)
            << "::find_worker_context(worker_name) {\n";
        out << "        return Some(context);\n";
        out << "    }\n";
    }
    out << "    None\n";
    out << "}\n";
    return out.str();
}

std::string generate_rust_workflow_worker_module(const IrWorkflow& workflow)
{
    std::ostringstream out;
    out << "use crate::backend::{BackendError, BackendResult};\n";
    out << "use crate::workflow::WorkflowExecutionRecord;\n";
    out << "use crate::workflow_step_handlers::{WorkflowStepHandler, "
           "WorkflowStepHandlerContext};\n\n";
    out << "pub fn dispatch_step(\n";
    out << "    handler: &impl WorkflowStepHandler,\n";
    out << "    context: &WorkflowStepHandlerContext,\n";
    out << "    record: &WorkflowExecutionRecord,\n";
    out << ") -> Option<BackendResult<()>> {\n";
    out << "    if record.workflow_name != " << rust_string(workflow.name) << " {\n";
    out << "        return None;\n";
    out << "    }\n";
    out << "    match record.current_step.as_str() {\n";
    for (const auto& step : workflow.steps)
    {
        out << "        " << rust_string(step.name) << " => Some(handler.handle_"
            << snake_identifier(workflow.name + "_" + step.name) << "(context)),\n";
    }
    out << "        _ => Some(Err(BackendError::Unsupported {\n";
    out << "            message: \"unknown generated workflow step handler\".to_string(),\n";
    out << "        })),\n";
    out << "    }\n";
    out << "}\n\n";
    out << "pub fn next_step(record: &WorkflowExecutionRecord) -> Option<String> {\n";
    out << "    if record.workflow_name != " << rust_string(workflow.name) << " {\n";
    out << "        return None;\n";
    out << "    }\n";
    out << "    match record.current_step.as_str() {\n";
    for (const auto& step : workflow.steps)
    {
        for (const auto& statement : step.statements)
        {
            if (statement.kind != "transition_to" || !statement.target.has_value())
            {
                continue;
            }
            out << "        " << rust_string(step.name) << " => Some("
                << rust_string(*statement.target) << ".to_string()),\n";
        }
    }
    out << "        _ => None,\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

TemplateRenderer::Values rust_workflow_runner_values(const IrSystem& system)
{
    std::ostringstream declarations;
    std::ostringstream dispatch_cases;
    std::ostringstream next_cases;
    for (const auto& workflow : system.workflows)
    {
        const auto module_name = rust_workflow_module_name(workflow);
        declarations << "#[path = \"workflows/" << snake_identifier(workflow.name) << ".rs\"]\n";
        declarations << "mod " << module_name << ";\n";
        dispatch_cases << "        if !matched {\n";
        dispatch_cases << "            if let Some(result) = " << module_name
                       << "::dispatch_step(self.handler, &context, &record) {\n";
        dispatch_cases << "                matched = true;\n";
        dispatch_cases << "                handler_result = result;\n";
        dispatch_cases << "            }\n";
        dispatch_cases << "        }\n";
        next_cases << "                if next_step.is_none() {\n";
        next_cases << "                    next_step = " << module_name
                   << "::next_step(&record);\n";
        next_cases << "                }\n";
    }
    return TemplateRenderer::Values{
        {"workflow_step_module_declarations", declarations.str()},
        {"workflow_step_module_dispatch_cases", dispatch_cases.str()},
        {"workflow_step_module_next_cases", next_cases.str()},
    };
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

std::string rust_api_handler_domain_module_name(std::string_view domain_name)
{
    return "entity_" + snake_identifier(std::string{domain_name});
}

std::string rust_api_handler_domain_type_name(std::string_view domain_name)
{
    return "Default" + pascal_identifier(std::string{domain_name}) + "ApiHandler";
}

std::string rust_api_handler_domain_path(std::string_view domain_name)
{
    return "api/entities/" + snake_identifier(std::string{domain_name}) + "/handlers.rs";
}

std::string rust_api_handler_domain_registry_path(std::string_view domain_name)
{
    return "api/entities/" + snake_identifier(std::string{domain_name}) + "/registry.rs";
}

std::string rust_api_descriptor_module_name(const IrApi& api);
std::string rust_api_descriptor_function_name(const IrApi& api);
std::string rust_api_route_descriptor_function_name(const IrApi& api);

std::string rust_entity_api_catalog_path(std::string_view entity_name)
{
    return "api/entities/" + snake_identifier(std::string{entity_name}) + "/catalog.rs";
}

std::string rust_api_handler_registry_domain_modules(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        out << "#[path = \"entities/" << snake_identifier(domain.name) << "/catalog.rs\"]\n";
        out << "mod " << rust_api_handler_domain_module_name(domain.name) << ";\n";
    }
    return out.str();
}

std::string rust_api_handler_lookup_registrations(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        const auto module_name = rust_api_handler_domain_module_name(domain.name);
        out << "    " << module_name << "::registry::register_handler_invokers::<B>(handlers);\n";
    }
    return out.str();
}

bool is_entity_crud_api_rs(const IrApi& api)
{
    return api.entity.has_value() && api.repository_operation.has_value();
}

std::vector<ApiHandlerDomain>
crud_api_handler_domains_rs(const std::vector<ApiHandlerDomain>& domains)
{
    std::vector<ApiHandlerDomain> result;
    for (const auto& domain : domains)
    {
        for (const auto& api : domain.apis)
        {
            if (is_entity_crud_api_rs(api))
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

std::string rust_business_api_handler_lookup_entries(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& api : system.apis)
    {
        if (is_entity_crud_api_rs(api))
        {
            continue;
        }
        out << "    handlers.insert(" << rust_string(api.name) << ", |handler, context| "
            << "handler.handle_" << snake_identifier(api.name) << "(context));\n";
    }
    return out.str();
}

std::string rust_api_handler_domain_registry_file(const ApiHandlerDomain& domain)
{
    std::ostringstream out;
    out << "use super::*;\n\n";
    out << "#[path = \"handlers.rs\"]\n";
    out << "mod handlers;\n\n";
    for (const auto& api : domain.apis)
    {
        out << "pub(super) fn handle_" << snake_identifier(api.name)
            << "<B: Backend>(backend: &B, context: &ApiRequestContext) -> "
               "BackendResult<ApiResponse> {\n";
        out << "    handlers::" << rust_api_handler_domain_type_name(domain.name)
            << " { backend }.handle_" << snake_identifier(api.name) << "(context)\n";
        out << "}\n\n";
    }
    out << "pub(crate) fn register_handler_invokers<B: Backend>(\n";
    out << "    handlers: &mut std::collections::HashMap<&'static str, "
           "crate::api_dispatcher::ApiHandlerInvoker<B>>,\n";
    out << ") {\n";
    for (const auto& api : domain.apis)
    {
        out << "    handlers.insert(" << rust_string(api.name) << ", handle_"
            << snake_identifier(api.name) << "::<B>);\n";
    }
    out << "}\n";
    return out.str();
}

std::string rust_api_handler_domain_file(
    const IrSystem& system,
    const ApiHandlerDomain& domain
)
{
    const auto filtered = with_domain_apis(system, domain.apis);
    std::ostringstream out;
    out << "use super::*;\n\n";
    out << "use crate::backend::{BackendError, IndexValue};\n\n";
    out << "#[allow(dead_code)]\n";
    out << "pub(super) struct " << rust_api_handler_domain_type_name(domain.name)
        << "<'a, B: Backend> {\n";
    out << "    pub(super) backend: &'a B,\n";
    out << "}\n\n";
    out << "impl<'a, B: Backend> " << rust_api_handler_domain_type_name(domain.name)
        << "<'a, B> {\n";
    out << generate_api_operation_default_handler_domain_methods_rs(filtered);
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

std::optional<std::string> rust_entity_api_shape_owner(
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

std::vector<IrShape> rust_entity_api_shapes(
    const IrSystem& system,
    std::string_view entity_name
)
{
    std::vector<IrShape> shapes;
    for (const auto& shape : system.shapes)
    {
        const auto owner = rust_entity_api_shape_owner(system, shape.name);
        if (owner.has_value() && *owner == entity_name)
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

std::string rust_api_codec_shape_module_name(std::string_view shape_name)
{
    return "api_codecs_" + snake_identifier(std::string{shape_name});
}

std::string rust_api_codec_shape_path(std::string_view shape_name)
{
    return "api/codecs/" + snake_identifier(std::string{shape_name}) + ".rs";
}

std::string rust_entity_api_codec_path(std::string_view entity_name)
{
    return "api/entities/" + snake_identifier(std::string{entity_name}) + "/codecs.rs";
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

std::string rust_api_codec_modules(
    const IrSystem& system,
    const std::vector<IrShape>& shapes
)
{
    std::ostringstream out;
    std::set<std::string> included_entities;
    for (const auto& shape : shapes)
    {
        const auto owner = rust_entity_api_shape_owner(system, shape.name);
        if (owner.has_value())
        {
            if (included_entities.insert(*owner).second)
            {
                const auto module_name = "api_codecs_entity_" + snake_identifier(*owner);
                out << "#[path = \"entities/" << snake_identifier(*owner) << "/codecs.rs\"]\n";
                out << "mod " << module_name << ";\n";
                out << "pub use " << module_name << "::*;\n";
            }
            continue;
        }
        out << "#[path = \"codecs/" << snake_identifier(shape.name) << ".rs\"]\n";
        out << "mod " << rust_api_codec_shape_module_name(shape.name) << ";\n";
        out << "pub use " << rust_api_codec_shape_module_name(shape.name) << "::*;\n";
    }
    return out.str();
}

std::string rust_api_codec_shape_file(
    const IrSystem& system,
    const IrShape& shape
)
{
    const auto filtered = with_codec_shape_apis(system, shape.name);
    std::ostringstream out;
    out << "use super::*;\n\n";
    out << generate_api_codec_operations_rs(filtered);
    return out.str();
}

std::string rust_entity_api_codec_file(
    const IrSystem& system,
    const IrEntity& entity
)
{
    const auto shapes = rust_entity_api_shapes(system, entity.name);
    const auto filtered = with_codec_shapes_apis(system, shapes);
    std::ostringstream out;
    out << "use crate::api_shapes::entity_" << snake_identifier(entity.name)
        << "_shapes as shapes;\n";
    out << "use crate::api_codecs::*;\n";
    out << "use crate::backend::{BackendError, BackendResult};\n";
    out << "use crate::descriptor_types::{ApiRequestContext, ApiResponse};\n";
    out << "use crate::entity_" << snake_identifier(entity.name)
        << "::constants as entity_constants;\n";
    out << "use crate::json::Json;\n";
    out << "use std::collections::BTreeMap;\n\n";
    out << generate_api_codec_operations_rs(filtered);
    return out.str();
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

std::string rust_entity_gc_descriptor_file(const IrEntity& entity)
{
    std::ostringstream terminal_states;
    for (const auto& state : entity.states)
    {
        if (!state.garbage_collection.has_value())
        {
            continue;
        }
        terminal_states << "            EntityGcTerminalStateDescriptor {\n"
                        << "                state: constants::"
                        << rust_entity_state_constant_name(entity.name, state.name)
                        << ".to_string(),\n"
                        << "                after: schema::"
                        << upper_snake_identifier(
                               entity.name + "_state_" + state.name + "_gc_after"
                           )
                        << ".to_string(),\n"
                        << "                mode: schema::"
                        << upper_snake_identifier(entity.name + "_state_" + state.name + "_gc_mode")
                        << ".to_string(),\n"
                        << "            },\n";
    }

    const auto descriptor_function = snake_identifier(entity.name) + "_entity_gc_descriptor";
    std::ostringstream out;
    out << "use crate::runtime_entity_gc_types::{EntityGcDescriptor, "
           "EntityGcTerminalStateDescriptor};\n\n";
    out << "use super::{constants, schema};\n\n";
    out << "pub fn " << descriptor_function
        << "() -> crate::runtime_entity_gc_types::EntityGcDescriptor {\n";
    out << "    EntityGcDescriptor {\n";
    out << "        entity: constants::" << rust_entity_name_constant_name(entity.name)
        << ".to_string(),\n";
    out << "        collection: constants::" << rust_entity_name_constant_name(entity.name)
        << ".to_string(),\n";
    out << "        status_field: constants::"
        << rust_entity_field_constant_name(entity.name, "status") << ".to_string(),\n";
    out << "        terminal_states: vec![\n";
    out << terminal_states.str();
    out << "        ],\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string rust_api_entity_gc_catalog_file(const IrSystem& system)
{
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
        descriptor_calls << "        crate::entity_" << snake_identifier(entity.name)
                         << "::gc::" << snake_identifier(entity.name)
                         << "_entity_gc_descriptor(),\n";
    }

    std::ostringstream out;
    out << "pub fn entity_gc_descriptors() -> "
           "Vec<crate::runtime_entity_gc_types::EntityGcDescriptor> {\n";
    out << "    vec![\n";
    out << descriptor_calls.str();
    out << "    ]\n";
    out << "}\n";
    return out.str();
}

std::string rust_worker_entity_gc_catalog_file(const IrSystem& system)
{
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
        descriptor_calls << "        crate::entity_" << snake_identifier(entity.name)
                         << "::gc::" << snake_identifier(entity.name)
                         << "_entity_gc_descriptor(),\n";
    }

    std::ostringstream out;
    out << "pub fn entity_gc_descriptors() -> "
           "Vec<crate::runtime_entity_gc_types::EntityGcDescriptor> {\n";
    out << "    vec![\n";
    out << descriptor_calls.str();
    out << "    ]\n";
    out << "}\n";
    return out.str();
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

std::string rust_entity_centered_facade_file(
    const IrEntity& entity,
    std::string_view area
)
{
    std::ostringstream out;
    out << "#![allow(dead_code)]\n\n";
    if (area == "model")
    {
        out << "use crate::backend::{FieldDescriptor, FieldType, IndexDescriptor};\n\n";
        out << "use super::constants::*;\n\n";
        out << "pub fn " << snake_identifier(entity.name)
            << "_field_descriptors() -> Vec<FieldDescriptor> {\n";
        out << "    vec![\n";
        for (const auto& field : entity.fields)
        {
            out << "        " << rust_entity_field_descriptor_expr(entity.name, field) << ",\n";
        }
        out << "    ]\n";
        out << "}\n\n";
        out << "pub fn " << snake_identifier(entity.name)
            << "_index_descriptors() -> Vec<IndexDescriptor> {\n";
        out << "    vec![\n";
        for (const auto& index : entity.indexes)
        {
            out << "        IndexDescriptor {\n";
            out << "            name: " << rust_entity_index_constant_name(entity.name, index.name)
                << ".to_string(),\n";
            out << "            fields: vec![";
            for (std::size_t i = 0; i < index.fields.size(); ++i)
            {
                if (i > 0)
                {
                    out << ", ";
                }
                out << rust_entity_field_constant_name(entity.name, index.fields[i])
                    << ".to_string()";
            }
            out << "],\n";
            out << "            unique: " << (index.unique ? "true" : "false") << ",\n";
            out << "        },\n";
        }
        out << "    ]\n";
        out << "}\n\n";
        out << "// Entity descriptor catalogs are composed by common/descriptors.rs.\n";
    }
    else if (area == "schema")
    {
        out << "use crate::backend::CollectionDescriptor;\n";
        out << "use super::constants::*;\n";
        out << "use super::model::*;\n\n";
        out << "pub const " << upper_snake_identifier(entity.name + "_schema_version")
            << ": u64 = 1;\n";
        if (entity.ownership.has_value())
        {
            out << "pub const " << upper_snake_identifier(entity.name + "_ownership_authority")
                << ": &str = " << rust_string(entity.ownership->authority) << ";\n";
            out << "pub const "
                << upper_snake_identifier(entity.name + "_ownership_system_of_record")
                << ": &str = " << rust_string(entity.ownership->system_of_record) << ";\n";
            out << "pub const " << upper_snake_identifier(entity.name + "_ownership_lifecycle")
                << ": &str = " << rust_string(entity.ownership->lifecycle) << ";\n";
        }
        for (const auto& state : entity.states)
        {
            out << "pub const "
                << upper_snake_identifier(entity.name + "_state_" + state.name + "_terminal")
                << ": bool = " << (state.terminal ? "true" : "false") << ";\n";
            if (state.garbage_collection.has_value())
            {
                out << "pub const "
                    << upper_snake_identifier(entity.name + "_state_" + state.name + "_gc_after")
                    << ": &str = " << rust_string(state.garbage_collection->after) << ";\n";
                out << "pub const "
                    << upper_snake_identifier(entity.name + "_state_" + state.name + "_gc_mode")
                    << ": &str = " << rust_string(state.garbage_collection->mode) << ";\n";
            }
        }
        out << "\n";
        out << "pub fn " << snake_identifier(entity.name)
            << "_collection_descriptor() -> CollectionDescriptor {\n";
        out << "    CollectionDescriptor {\n";
        out << "        name: " << rust_entity_name_constant_name(entity.name) << ".to_string(),\n";
        out << "        fields: " << snake_identifier(entity.name) << "_field_descriptors(),\n";
        out << "        key_fields: vec![";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_entity_field_constant_name(entity.name, entity.key_fields[i])
                << ".to_string()";
        }
        out << "],\n";
        out << "        indexes: " << snake_identifier(entity.name) << "_index_descriptors(),\n";
        out << "        schema_version: " << upper_snake_identifier(entity.name + "_schema_version")
            << ",\n";
        out << "    }\n";
        out << "}\n\n";
        out << "// Collection schema and compatibility metadata are rooted with the entity "
               "schema.\n";
        out << "// common/descriptors.rs composes entity descriptor catalogs.\n";
    }
    else if (area == "persistence")
    {
        const auto type_name = pascal_identifier(entity.name);
        const auto function_prefix = snake_identifier(entity.name);
        out << "use crate::backend::{Backend, BackendResult, VersionedRecord};\n";
        out << "use crate::descriptors::{\n";
        out << "    entity_lookup_from_document, DefaultEntityRepository, EntityCreateRequest,\n";
        out << "    EntityDescriptor, EntityGetRequest, EntityKeyValue, "
               "EntityListByIndexRequest,\n";
        out << "    EntityLookup, EntityRepository, EntityUpsertRequest,\n";
        out << "};\n";
        out << "use crate::json::Json;\n";
        out << "use super::constants::*;\n";
        out << "use super::schema::*;\n\n";
        out << "pub fn " << function_prefix
            << "_lookup(key_values: Vec<EntityKeyValue>) -> EntityLookup {\n";
        out << "    EntityLookup {\n";
        out << "        entity: " << rust_entity_name_constant_name(entity.name)
            << ".to_string(),\n";
        out << "        key_fields: vec![";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_entity_field_constant_name(entity.name, entity.key_fields[i])
                << ".to_string()";
        }
        out << "],\n";
        out << "        key_values,\n";
        out << "    }\n";
        out << "}\n\n";
        out << "pub fn " << function_prefix << "_entity_descriptor() -> EntityDescriptor {\n";
        out << "    EntityDescriptor {\n";
        out << "        name: " << rust_entity_name_constant_name(entity.name) << ".to_string(),\n";
        out << "        key_fields: vec![";
        for (std::size_t i = 0; i < entity.key_fields.size(); ++i)
        {
            if (i > 0)
            {
                out << ", ";
            }
            out << rust_entity_field_constant_name(entity.name, entity.key_fields[i])
                << ".to_string()";
        }
        out << "],\n";
        out << "        ownership: None,\n";
        out << "        relations: vec![],\n";
        out << "        children: vec![],\n";
        out << "        invariants: vec![],\n";
        out << "        states: vec![],\n";
        out << "        initial_state: None,\n";
        out << "        terminal_states: vec![],\n";
        out << "    }\n";
        out << "}\n\n";
        out << "pub trait " << type_name << "Repository<B: Backend> {\n";
        out << "    fn register_descriptor(&self, backend: &B) -> BackendResult<()>;\n";
        out << "    fn create_tx(&self, tx: &mut B::Tx, document: Json) -> "
               "BackendResult<Option<VersionedRecord>>;\n";
        out << "    fn get_tx(&self, tx: &mut B::Tx, key_values: Vec<EntityKeyValue>) -> "
               "BackendResult<Option<VersionedRecord>>;\n";
        out << "    fn list_by_index_tx(\n";
        out << "        &self,\n";
        out << "        tx: &mut B::Tx,\n";
        out << "        index_name: String,\n";
        out << "        values: Vec<crate::backend::IndexValue>,\n";
        out << "    ) -> BackendResult<Vec<VersionedRecord>>;\n";
        for (const auto& index : entity.indexes)
        {
            out << "    fn " << rust_entity_index_repository_method_name(index.name) << "(\n";
            out << "        &self,\n";
            out << "        tx: &mut B::Tx,\n";
            out << "        values: Vec<crate::backend::IndexValue>,\n";
            out << "    ) -> BackendResult<Vec<VersionedRecord>>;\n";
        }
        out << "    fn update_tx(\n";
        out << "        &self,\n";
        out << "        tx: &mut B::Tx,\n";
        out << "        key_values: Vec<EntityKeyValue>,\n";
        out << "        document: Json,\n";
        out << "        expected_version: u64,\n";
        out << "    ) -> BackendResult<Option<VersionedRecord>>;\n";
        out << "}\n\n";
        out << "#[derive(Debug, Clone, Copy, Default)]\n";
        out << "pub struct Default" << type_name << "Repository;\n\n";
        out << "impl<B: Backend> " << type_name << "Repository<B> for Default" << type_name
            << "Repository {\n";
        out << "    fn register_descriptor(&self, backend: &B) -> BackendResult<()> {\n";
        out << "        backend.ensure_collection(&" << function_prefix
            << "_collection_descriptor())\n";
        out << "    }\n\n";
        out << "    fn create_tx(&self, tx: &mut B::Tx, document: Json) -> "
               "BackendResult<Option<VersionedRecord>> {\n";
        out << "        <DefaultEntityRepository as EntityRepository<B>>::create_entity_tx(\n";
        out << "            &DefaultEntityRepository,\n";
        out << "            tx,\n";
        out << "            &EntityCreateRequest {\n";
        out << "                lookup: entity_lookup_from_document(&" << function_prefix
            << "_entity_descriptor(), &document),\n";
        out << "                document,\n";
        out << "            },\n";
        out << "        )\n";
        out << "    }\n\n";
        out << "    fn get_tx(&self, tx: &mut B::Tx, key_values: Vec<EntityKeyValue>) -> "
               "BackendResult<Option<VersionedRecord>> {\n";
        out << "        <DefaultEntityRepository as EntityRepository<B>>::get_entity_tx(\n";
        out << "            &DefaultEntityRepository,\n";
        out << "            tx,\n";
        out << "            &EntityGetRequest { lookup: " << function_prefix
            << "_lookup(key_values) },\n";
        out << "        )\n";
        out << "    }\n\n";
        out << "    fn list_by_index_tx(\n";
        out << "        &self,\n";
        out << "        tx: &mut B::Tx,\n";
        out << "        index_name: String,\n";
        out << "        values: Vec<crate::backend::IndexValue>,\n";
        out << "    ) -> BackendResult<Vec<VersionedRecord>> {\n";
        out << "        <DefaultEntityRepository as "
               "EntityRepository<B>>::list_entities_by_index_tx(\n";
        out << "            &DefaultEntityRepository,\n";
        out << "            tx,\n";
        out << "            &EntityListByIndexRequest {\n";
        out << "                entity: " << rust_entity_name_constant_name(entity.name)
            << ".to_string(),\n";
        out << "                index_name,\n";
        out << "                values,\n";
        out << "            },\n";
        out << "        )\n";
        out << "    }\n";
        for (const auto& index : entity.indexes)
        {
            out << "\n";
            out << "    fn " << rust_entity_index_repository_method_name(index.name) << "(\n";
            out << "        &self,\n";
            out << "        tx: &mut B::Tx,\n";
            out << "        values: Vec<crate::backend::IndexValue>,\n";
            out << "    ) -> BackendResult<Vec<VersionedRecord>> {\n";
            out << "        <Default" << type_name << "Repository as " << type_name
                << "Repository<B>>::list_by_index_tx(\n";
            out << "            self,\n";
            out << "            tx,\n";
            out << "            " << rust_entity_index_constant_name(entity.name, index.name)
                << ".to_string(),\n";
            out << "            values,\n";
            out << "        )\n";
            out << "    }\n";
        }
        out << "\n";
        out << "    fn update_tx(\n";
        out << "        &self,\n";
        out << "        tx: &mut B::Tx,\n";
        out << "        key_values: Vec<EntityKeyValue>,\n";
        out << "        document: Json,\n";
        out << "        expected_version: u64,\n";
        out << "    ) -> BackendResult<Option<VersionedRecord>> {\n";
        out << "        <DefaultEntityRepository as EntityRepository<B>>::upsert_entity_tx(\n";
        out << "            &DefaultEntityRepository,\n";
        out << "            tx,\n";
        out << "            &EntityUpsertRequest {\n";
        out << "                lookup: " << function_prefix << "_lookup(key_values),\n";
        out << "                document,\n";
        out << "                expected_version: Some(expected_version),\n";
        out << "            },\n";
        out << "        )\n";
        out << "    }\n";
        out << "}\n";
    }
    else
    {
        out << "// Entity-centered " << area
            << " facade. Implementation moves here in the next staged split.\n";
    }
    return out.str();
}

std::string rust_entity_constants_file(const IrEntity& entity)
{
    std::ostringstream out;
    out << "#![allow(dead_code)]\n\n";
    out << "pub const " << rust_entity_name_constant_name(entity.name)
        << ": &str = " << rust_string(entity.name) << ";\n";
    out << "pub const " << rust_entity_plural_name_constant_name(entity.name)
        << ": &str = " << rust_string(rust_entity_plural_api_field_name(entity.name)) << ";\n";
    out << "pub const " << upper_snake_identifier(entity.name + "_collection_name")
        << ": &str = " << rust_string(entity.name) << ";\n";
    out << "pub const " << upper_snake_identifier(entity.name + "_key_helper_name")
        << ": &str = " << rust_string(snake_identifier(entity.name) + "_lookup") << ";\n";
    for (const auto& field : entity.fields)
    {
        out << "pub const " << rust_entity_field_constant_name(entity.name, field.name)
            << ": &str = " << rust_string(field.name) << ";\n";
        out << "pub const " << rust_entity_field_type_name_constant_name(entity.name, field.name)
            << ": &str = " << rust_string(field.type) << ";\n";
    }
    for (const auto& index : entity.indexes)
    {
        out << "pub const " << rust_entity_index_constant_name(entity.name, index.name)
            << ": &str = " << rust_string(index.name) << ";\n";
        out << "pub const "
            << upper_snake_identifier(entity.name + "_index_" + index.name + "_helper_name")
            << ": &str = " << rust_string(rust_entity_index_repository_method_name(index.name))
            << ";\n";
    }
    for (const auto& state : entity.states)
    {
        out << "pub const " << rust_entity_state_constant_name(entity.name, state.name)
            << ": &str = " << rust_string(state.name) << ";\n";
    }
    for (const auto& relation : entity.relations)
    {
        const auto relation_constant_prefix =
            upper_snake_identifier(entity.name + "_relation_" + relation.name);
        out << "pub const " << relation_constant_prefix
            << "_NAME: &str = " << rust_string(relation.name) << ";\n";
        out << "pub const " << relation_constant_prefix
            << "_TARGET: &str = " << rust_string(relation.target) << ";\n";
        out << "pub const " << relation_constant_prefix
            << "_KIND: &str = " << rust_string(relation.kind) << ";\n";
        if (relation.relation_kind.has_value())
        {
            out << "pub const " << relation_constant_prefix
                << "_RELATION_KIND: &str = " << rust_string(*relation.relation_kind) << ";\n";
        }
        if (relation.on_parent_delete.has_value())
        {
            out << "pub const " << relation_constant_prefix
                << "_ON_PARENT_DELETE: &str = " << rust_string(*relation.on_parent_delete) << ";\n";
        }
    }
    return out.str();
}

std::string rust_event_descriptor_module(const IrSystem& system)
{
    std::ostringstream out;
    out << "use std::collections::BTreeMap;\n\n";
    out << "use crate::json::Json;\n\n";
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct EventEnvelope {\n";
    out << "    pub name: String,\n";
    out << "    pub fields: BTreeMap<String, Json>,\n";
    out << "}\n\n";
    for (const auto& event : system.events)
    {
        out << "pub fn make_" << snake_identifier(event.name) << "_event(\n";
        for (const auto& field : event.fields)
        {
            out << "    " << field.name << ": Json,\n";
        }
        out << ") -> EventEnvelope {\n";
        out << "    let mut fields = BTreeMap::new();\n";
        for (const auto& field : event.fields)
        {
            out << "    fields.insert(" << rust_string(field.name) << ".to_string(), " << field.name
                << ");\n";
        }
        out << "    EventEnvelope { name: " << rust_string(event.name)
            << ".to_string(), fields }\n";
        out << "}\n\n";
    }
    return out.str();
}

std::string rust_external_system_descriptor_module(
    const IrSystem& system,
    const TemplatePackage& templates
)
{
    std::ostringstream out;
    out << "use super::*;\n";
    out << "use crate::backend::{Backend, BackendError, BackendResult};\n";
    out << "use crate::external_system::{\n";
    out << "    ExternalSystemMetadataKeyValue, ExternalSystemMetadataLookup,\n";
    out << "    ExternalSystemMetadataResolution, ExternalSystemMetadataResolver,\n";
    out << "};\n\n";
    out << generate_rust_external_system_descriptors(
        system, templates.load("generated/external_system_call_adapters.rs.tmpl")
    );
    return out.str();
}

TemplateRenderer::Values rust_shape_descriptor_module_values(const IrSystem& system)
{
    std::ostringstream content;
    for (const auto& shape : system.shapes)
    {
        content << "#[path = \"shapes/" << snake_identifier(shape.name) << ".rs\"]\n";
        content << "mod shape_" << snake_identifier(shape.name) << ";\n";
        content << "pub use shape_" << snake_identifier(shape.name) << "::*;\n";
    }
    if (!system.shapes.empty())
    {
        content << "\n";
    }
    content << "pub fn shape_descriptors() -> Vec<ShapeDescriptor> {\n";
    content << "    let mut descriptors = Vec::new();\n";
    for (const auto& shape : system.shapes)
    {
        content << "    descriptors.extend(shape_" << snake_identifier(shape.name)
                << "::" << snake_identifier(shape.name) << "_shape_descriptors());\n";
    }
    content << "    descriptors\n";
    content << "}\n\n";
    return TemplateRenderer::Values{
        {"descriptor_module_name", "shape descriptors"},
        {"descriptor_module_content", "use super::*;\n\n" + content.str()},
    };
}

TemplateRenderer::Values rust_shape_descriptor_module_values(const IrShape& shape)
{
    IrSystem one_shape_system;
    one_shape_system.shapes.push_back(shape);
    auto content = generate_rust_shape_descriptors(one_shape_system);
    const auto prefix = snake_identifier(shape.name);
    content = replace_all_copy(content, "shape_descriptors()", prefix + "_shape_descriptors()");
    return TemplateRenderer::Values{
        {"descriptor_module_name", "shape descriptor " + shape.name},
        {"descriptor_module_content", "use super::*;\n\n" + content},
    };
}

std::string rust_shape_types_file()
{
    return generate_rust_shape_types_file();
}

std::string rust_descriptor_types_file()
{
    return generate_rust_descriptor_types_file();
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

std::string rust_shape_type_declaration(const IrShape& shape);

std::string rust_shape_type_file(const IrShape& shape)
{
    std::ostringstream out;
    out << "#[allow(unused_imports)]\n";
    out << "use crate::json::Json;\n\n";
    out << rust_shape_type_declaration(shape);
    return out.str();
}

std::string rust_api_shape_type_file(const IrShape& shape)
{
    IrSystem one_shape_system;
    one_shape_system.shapes.push_back(shape);
    auto descriptors = generate_rust_shape_descriptors(one_shape_system);
    descriptors = replace_all_copy(
        descriptors, "shape_descriptors()", snake_identifier(shape.name) + "_shape_descriptors()"
    );

    std::ostringstream out;
    out << "#[allow(unused_imports)]\n";
    out << "use crate::json::Json;\n\n";
    out << "use crate::backend::{FieldDescriptor, FieldType};\n";
    out << "use crate::shape_types::ShapeDescriptor;\n\n";
    out << rust_shape_type_declaration(shape) << "\n";
    out << descriptors;
    return out.str();
}

const IrField* rust_find_entity_field(
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

std::string rust_entity_shape_field_descriptor_expr(
    const IrEntity& entity,
    const IrShape& shape,
    const IrField& field
)
{
    auto expression = rust_shape_field_descriptor_expr(shape.name, field);
    if (rust_find_entity_field(entity, field.name) == nullptr)
    {
        if (field.name == rust_entity_plural_api_field_name(entity.name))
        {
            return "FieldDescriptor { name: entity_constants::" +
                   rust_entity_plural_name_constant_name(entity.name) +
                   ".to_string(), field_type: " + rust_field_type_enum_expr(field.type) +
                   ", type_name: " +
                   rust_shape_field_type_name_constant_name(shape.name, field.name) +
                   ".to_string(), required: " +
                   (rust_field_required(field.type) ? "true" : "false") + " }";
        }
        return expression;
    }
    expression = replace_all_copy(
        expression, rust_shape_field_constant_name(shape.name, field.name),
        "entity_constants::" + rust_entity_field_constant_name(entity.name, field.name)
    );
    expression = replace_all_copy(
        expression, rust_shape_field_type_name_constant_name(shape.name, field.name),
        "entity_constants::" + rust_entity_field_type_name_constant_name(entity.name, field.name)
    );
    return expression;
}

std::string rust_entity_api_shape_descriptors(
    const IrEntity& entity,
    const IrShape& shape,
    std::string_view function_name
)
{
    std::ostringstream out;
    out << "pub const " << rust_shape_name_constant_name(shape.name)
        << ": &str = " << rust_string(shape.name) << ";\n";
    for (const auto& field : shape.fields)
    {
        if (rust_find_entity_field(entity, field.name) != nullptr)
        {
            continue;
        }
        if (field.name == rust_entity_plural_api_field_name(entity.name))
        {
            out << "pub const " << rust_shape_field_type_name_constant_name(shape.name, field.name)
                << ": &str = " << rust_string(field.type) << ";\n";
            continue;
        }
        out << "pub const " << rust_shape_field_constant_name(shape.name, field.name)
            << ": &str = " << rust_string(field.name) << ";\n";
        out << "pub const " << rust_shape_field_type_name_constant_name(shape.name, field.name)
            << ": &str = " << rust_string(field.type) << ";\n";
    }
    out << "\n";
    out << "pub fn " << function_name << " -> Vec<ShapeDescriptor> {\n";
    out << "    vec![\n";
    out << "        ShapeDescriptor {\n";
    out << "            name: " << rust_shape_name_constant_name(shape.name) << ".to_string(),\n";
    out << "            fields: vec![\n";
    for (const auto& field : shape.fields)
    {
        out << "                " << rust_entity_shape_field_descriptor_expr(entity, shape, field)
            << ",\n";
    }
    out << "            ],\n";
    out << "        },\n";
    out << "    ]\n";
    out << "}\n\n";
    return out.str();
}

std::string rust_shape_type_declaration(const IrShape& shape)
{
    std::ostringstream out;
    out << "#[derive(Debug, Clone)]\n";
    out << "pub struct " << pascal_identifier(shape.name) << " {\n";
    for (const auto& field : shape.fields)
    {
        out << "    pub " << field.name << ": " << rust_shape_type(field.type) << ",\n";
    }
    out << "}\n";
    return out.str();
}

std::string rust_shapes_module(
    const IrSystem& system,
    bool api_shapes = false
)
{
    std::ostringstream out;
    for (const auto& shape : system.shapes)
    {
        if (api_shapes && rust_entity_api_shape_owner(system, shape.name).has_value())
        {
            continue;
        }
        const auto module_name = "shape_" + snake_identifier(shape.name);
        out << "#[path = \"shapes/" << snake_identifier(shape.name) << ".rs\"]\n";
        out << (api_shapes ? "pub mod " : "mod ") << module_name << ";\n";
        out << "pub use " << module_name << "::*;\n";
    }
    if (api_shapes)
    {
        for (const auto& entity : system.entities)
        {
            if (rust_entity_api_shapes(system, entity.name).empty())
            {
                continue;
            }
            const auto module_name = "entity_" + snake_identifier(entity.name) + "_shapes";
            out << "#[path = \"entities/" << snake_identifier(entity.name) << "/shapes.rs\"]\n";
            out << "pub mod " << module_name << ";\n";
            out << "pub use " << module_name << "::*;\n";
            out << "#[path = \"entities/" << snake_identifier(entity.name) << "/catalog.rs\"]\n";
            out << "mod entity_" << snake_identifier(entity.name) << "_catalog;\n";
        }
        out << "\n";
        out << "use crate::shape_types::ShapeDescriptor;\n\n";
        out << "pub fn shape_descriptors() -> Vec<ShapeDescriptor> {\n";
        out << "    let mut descriptors = Vec::new();\n";
        for (const auto& shape : system.shapes)
        {
            const auto owner = rust_entity_api_shape_owner(system, shape.name);
            if (owner.has_value())
            {
                continue;
            }
            out << "    descriptors.extend(shape_" << snake_identifier(shape.name)
                << "::" << snake_identifier(shape.name) << "_shape_descriptors());\n";
        }
        std::set<std::string> added_entities;
        for (const auto& shape : system.shapes)
        {
            const auto owner = rust_entity_api_shape_owner(system, shape.name);
            if (!owner.has_value() || !added_entities.insert(*owner).second)
            {
                continue;
            }
            out << "    descriptors.extend(entity_" << snake_identifier(*owner)
                << "_catalog::shape_descriptors());\n";
        }
        out << "    descriptors\n";
        out << "}\n";
    }
    return out.str();
}

std::string rust_entity_api_shapes_file(
    const IrEntity& entity,
    const std::vector<IrShape>& shapes
)
{
    std::ostringstream out;
    out << "#[allow(unused_imports)]\n";
    out << "use crate::json::Json;\n\n";
    out << "use crate::backend::{FieldDescriptor, FieldType};\n";
    out << "use crate::entity_" << snake_identifier(entity.name)
        << "::constants as entity_constants;\n";
    out << "use crate::shape_types::ShapeDescriptor;\n\n";
    for (const auto& shape : shapes)
    {
        out << rust_shape_type_declaration(shape) << "\n";
        out << rust_entity_api_shape_descriptors(
            entity, shape, snake_identifier(shape.name) + "_shape_descriptors()"
        );
    }
    return out.str();
}

std::string rust_api_name_constant_name(const std::string& api_name)
{
    return upper_snake_identifier(api_name + "_api_name");
}

std::string rust_api_route_name_constant_name(
    const std::string& api_server_name,
    const std::string& api_name
)
{
    return upper_snake_identifier(api_server_name + "_" + api_name + "_route_name");
}

std::string rust_api_response_envelope_constant_name(const std::string& entity_name)
{
    return upper_snake_identifier(entity_name + "_list_response_envelope_name");
}

std::string rust_api_server_name_constant_name(const std::string& api_server_name)
{
    return upper_snake_identifier(api_server_name + "_server_name");
}

std::string rust_api_server_concurrency_constant_name(const std::string& api_server_name)
{
    return upper_snake_identifier(api_server_name + "_server_concurrency");
}

std::string rust_api_server_constants_file(const IrApiServer& api_server)
{
    std::ostringstream out;
    out << "pub const " << rust_api_server_name_constant_name(api_server.name)
        << ": &str = " << rust_string(api_server.name) << ";\n";
    out << "pub const " << rust_api_server_concurrency_constant_name(api_server.name)
        << ": i32 = " << api_server.concurrency.value_or(1) << ";\n";
    return out.str();
}

std::string rust_api_server_catalog_file(
    const IrSystem& system,
    const IrApiServer& api_server
)
{
    const auto entity_domains = crud_api_handler_domains_rs(api_handler_domains(system));
    std::set<std::string> entity_api_names;
    std::ostringstream out;
    out << "#[path = \"constants.rs\"]\n";
    out << "mod constants;\n";
    for (const auto& domain : entity_domains)
    {
        const auto served = std::any_of(
            domain.apis.begin(), domain.apis.end(),
            [&](const IrApi& api)
            {
                return std::find(api_server.serves.begin(), api_server.serves.end(), api.name) !=
                       api_server.serves.end();
            }
        );
        if (served)
        {
            out << "#[path = \"entities/" << snake_identifier(domain.name) << ".rs\"]\n";
            out << "mod entity_" << snake_identifier(domain.name) << ";\n";
        }
        for (const auto& api : domain.apis)
        {
            entity_api_names.insert(api.name);
        }
    }
    out << "\nuse crate::descriptor_types::ApiServerDescriptor;\n\n";
    out << "pub fn api_server_descriptors() -> Vec<ApiServerDescriptor> {\n";
    out << "    let mut serves = Vec::<String>::new();\n";
    for (const auto& domain : entity_domains)
    {
        const auto served = std::any_of(
            domain.apis.begin(), domain.apis.end(),
            [&](const IrApi& api)
            {
                return std::find(api_server.serves.begin(), api_server.serves.end(), api.name) !=
                       api_server.serves.end();
            }
        );
        if (served)
        {
            out << "    entity_" << snake_identifier(domain.name)
                << "::append_api_server_names(&mut serves);\n";
        }
    }
    for (const auto& served_api : api_server.serves)
    {
        if (entity_api_names.contains(served_api))
        {
            continue;
        }
        out << "    serves.push(" << rust_string(served_api) << ".to_string());\n";
    }
    out << "    vec![ApiServerDescriptor {\n";
    out << "        name: constants::" << rust_api_server_name_constant_name(api_server.name)
        << ".to_string(),\n";
    out << "        serves,\n";
    out << "        concurrency: constants::"
        << rust_api_server_concurrency_constant_name(api_server.name) << ",\n";
    out << "    }]\n";
    out << "}\n";
    return out.str();
}

std::string rust_api_server_entity_catalog_file(
    const IrApiServer& api_server,
    const ApiHandlerDomain& domain
)
{
    std::ostringstream out;
    out << "#[path = \"../../../entities/" << snake_identifier(domain.name) << "/catalog.rs\"]\n";
    out << "mod entity_catalog;\n\n";
    out << "pub fn append_api_server_names(serves: &mut Vec<String>) {\n";
    out << "    for api_name in entity_catalog::api_names() {\n";
    out << "        match *api_name {\n";
    for (const auto& api : domain.apis)
    {
        if (std::find(api_server.serves.begin(), api_server.serves.end(), api.name) ==
            api_server.serves.end())
        {
            continue;
        }
        out << "            entity_catalog::constants::" << rust_api_name_constant_name(api.name)
            << " => serves.push(api_name.to_string()),\n";
    }
    out << "            _ => {}\n";
    out << "        }\n";
    out << "    }\n";
    out << "}\n";
    return out.str();
}

std::string rust_entity_api_constants_file(
    const IrSystem& system,
    const IrEntity& entity
)
{
    const auto shapes = rust_entity_api_shapes(system, entity.name);
    std::vector<IrApi> apis;
    for (const auto& domain : crud_api_handler_domains_rs(api_handler_domains(system)))
    {
        if (domain.name == entity.name)
        {
            apis = domain.apis;
            break;
        }
    }

    std::ostringstream out;
    out << "use crate::entity_" << snake_identifier(entity.name)
        << "::constants as entity_constants;\n\n";
    for (const auto& api : apis)
    {
        out << "pub const " << rust_api_name_constant_name(api.name)
            << ": &str = " << rust_string(api.name) << ";\n";
    }
    if (!apis.empty())
    {
        out << "\n";
    }
    for (const auto& api_server : system.api_servers)
    {
        for (const auto& api : apis)
        {
            if (std::find(api_server.serves.begin(), api_server.serves.end(), api.name) ==
                api_server.serves.end())
            {
                continue;
            }
            out << "pub const " << rust_api_route_name_constant_name(api_server.name, api.name)
                << ": &str = " << rust_string(api_server.name + "." + api.name) << ";\n";
        }
    }
    if (!system.api_servers.empty() && !apis.empty())
    {
        out << "\n";
    }
    for (const auto& shape : shapes)
    {
        out << "pub const " << rust_shape_name_constant_name(shape.name)
            << ": &str = " << rust_string(shape.name) << ";\n";
    }
    if (!shapes.empty())
    {
        out << "\n";
    }
    out << "pub const " << rust_api_response_envelope_constant_name(entity.name)
        << ": &str = entity_constants::" << rust_entity_plural_name_constant_name(entity.name)
        << ";\n";
    return out.str();
}

std::string rust_entity_api_catalog_file(
    const IrSystem& system,
    const IrEntity& entity
)
{
    const auto shapes = rust_entity_api_shapes(system, entity.name);
    std::vector<IrApi> apis;
    for (const auto& domain : crud_api_handler_domains_rs(api_handler_domains(system)))
    {
        if (domain.name == entity.name)
        {
            apis = domain.apis;
            break;
        }
    }

    std::ostringstream out;
    out << "use crate::descriptor_types::{ApiDescriptor, ApiRouteDescriptor};\n";
    out << "use crate::shape_types::ShapeDescriptor;\n";
    out << "use crate::api_shapes::entity_" << snake_identifier(entity.name)
        << "_shapes as shapes;\n";
    out << "use shapes::*;\n";
    out << "use crate::api_codecs::*;\n";
    out << "use crate::api_handler_registry::*;\n";
    out << "use crate::api_handlers::{ApiRequestContext, ApiResponse};\n";
    out << "use crate::backend::{Backend, BackendResult};\n";
    out << "use crate::entity_repository::EntityKeyValue;\n";
    out << "use crate::json::Json;\n";
    out << "#[path = \"constants.rs\"]\n";
    out << "pub(crate) mod constants;\n";
    out << "#[path = \"codecs.rs\"]\n";
    out << "mod codecs;\n";
    out << "#[path = \"handlers.rs\"]\n";
    out << "mod handlers;\n";
    out << "#[path = \"registry.rs\"]\n";
    out << "pub(crate) mod registry;\n";
    for (const auto& api : apis)
    {
        out << "#[path = \"descriptors/" << snake_identifier(api.name) << ".rs\"]\n";
        out << "mod " << rust_api_descriptor_module_name(api) << ";\n";
    }
    out << "\n";
    out << "pub fn shape_descriptors() -> Vec<ShapeDescriptor> {\n";
    out << "    let mut descriptors = Vec::new();\n";
    for (const auto& shape : shapes)
    {
        out << "    descriptors.extend(shapes::" << snake_identifier(shape.name)
            << "_shape_descriptors());\n";
    }
    out << "    descriptors\n";
    out << "}\n\n";
    out << "pub fn api_descriptors() -> Vec<ApiDescriptor> {\n";
    out << "    let mut descriptors = Vec::new();\n";
    for (const auto& api : apis)
    {
        out << "    descriptors.extend(" << rust_api_descriptor_module_name(api)
            << "::" << rust_api_descriptor_function_name(api) << "());\n";
    }
    out << "    descriptors\n";
    out << "}\n\n";
    out << "pub fn api_names() -> &'static [&'static str] {\n";
    out << "    &[\n";
    for (const auto& api : apis)
    {
        out << "        constants::" << rust_api_name_constant_name(api.name) << ",\n";
    }
    out << "    ]\n";
    out << "}\n\n";
    out << "pub fn api_route_descriptors() -> Vec<ApiRouteDescriptor> {\n";
    out << "    let mut descriptors = Vec::new();\n";
    for (const auto& api : apis)
    {
        out << "    descriptors.extend(" << rust_api_descriptor_module_name(api)
            << "::" << rust_api_route_descriptor_function_name(api) << "());\n";
    }
    out << "    descriptors\n";
    out << "}\n\n";
    out << "pub fn handler_entrypoints() -> &'static [&'static str] {\n";
    out << "    &[\n";
    for (const auto& api : apis)
    {
        out << "        \"handle_" << snake_identifier(api.name) << "\",\n";
    }
    out << "    ]\n";
    out << "}\n";
    for (const auto& api : apis)
    {
        out << "\n";
        out << "pub(crate) fn handle_" << snake_identifier(api.name)
            << "<B: Backend>(backend: &B, context: &ApiRequestContext) -> "
               "BackendResult<ApiResponse> {\n";
        out << "    registry::handle_" << snake_identifier(api.name) << "(backend, context)\n";
        out << "}\n";
    }
    return out.str();
}

void add_rust_shape_type_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    const auto shared_shapes = rust_shapes_matching(system, false);
    if (shared_shapes.shapes.empty())
    {
        return;
    }
    add_rust_raw_common_file(result, options, "shapes.rs", rust_shapes_module(shared_shapes));
    for (const auto& shape : shared_shapes.shapes)
    {
        add_rust_raw_common_file(
            result, options, "shapes/" + snake_identifier(shape.name) + ".rs",
            rust_shape_type_file(shape)
        );
    }
}

void add_rust_raw_api_file(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    std::string_view relative_output_path,
    std::string content
);

void add_rust_api_shape_type_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    const auto api_shapes = rust_shapes_matching(system, true);
    add_rust_raw_api_file(result, options, "api/shapes.rs", rust_shapes_module(api_shapes, true));
    for (const auto& shape : api_shapes.shapes)
    {
        if (rust_entity_api_shape_owner(system, shape.name).has_value())
        {
            continue;
        }
        add_rust_raw_api_file(
            result, options, "api/shapes/" + snake_identifier(shape.name) + ".rs",
            rust_api_shape_type_file(shape)
        );
    }
    for (const auto& entity : system.entities)
    {
        const auto shapes = rust_entity_api_shapes(system, entity.name);
        if (shapes.empty())
        {
            continue;
        }
        add_rust_raw_api_file(
            result, options, "api/entities/" + snake_identifier(entity.name) + "/constants.rs",
            rust_entity_api_constants_file(system, entity)
        );
        add_rust_raw_api_file(
            result, options, "api/entities/" + snake_identifier(entity.name) + "/shapes.rs",
            rust_entity_api_shapes_file(entity, shapes)
        );
    }
}

std::string rust_api_shape_import(const IrSystem& system)
{
    if (rust_shapes_matching(system, true).shapes.empty())
    {
        return {};
    }
    return "#[allow(unused_imports)]\nuse crate::api_shapes as shapes;\n";
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
    const auto common_shapes = rust_shapes_matching(system, false);
    add_rust_raw_common_file(result, options, "descriptors/types.rs", rust_descriptor_types_file());
    add_rust_descriptor_module_artifact(
        result, options, templates, "descriptors/core.rs", "core descriptors", diagnostics,
        TemplateRenderer::Values{
            {"descriptor_module_name", "core descriptors"},
            {"descriptor_module_content",
             "use super::*;\n\n" + generate_rust_value_enum_descriptors(system)}
        }
    );
    add_rust_raw_common_file(
        result, options, "descriptors/events.rs",
        "use super::*;\n\n" + generate_rust_event_descriptors(system) +
            rust_event_descriptor_module(system)
    );
    add_rust_raw_common_file(
        result, options, "descriptors/external_systems.rs",
        rust_external_system_descriptor_module(system, templates)
    );
    add_rust_descriptor_module_artifact(
        result, options, templates, "descriptors/policies.rs", "policy descriptors", diagnostics,
        TemplateRenderer::Values{
            {"descriptor_module_name", "policy descriptors"},
            {"descriptor_module_content",
             "use super::*;\n\n" + generate_rust_policy_descriptors(system)}
        }
    );
    add_rust_descriptor_module_artifact(
        result, options, templates, "descriptors/observability.rs", "observability descriptors",
        diagnostics,
        TemplateRenderer::Values{
            {"descriptor_module_name", "observability descriptors"},
            {"descriptor_module_content",
             "use super::*;\n\n" + generate_rust_observability_descriptors(system)}
        }
    );
    if (!common_shapes.shapes.empty())
    {
        add_rust_descriptor_module_artifact(
            result, options, templates, "descriptors/shapes.rs", "shape descriptors", diagnostics,
            rust_shape_descriptor_module_values(common_shapes)
        );
        for (const auto& shape : common_shapes.shapes)
        {
            add_generated_template_file(
                result, options.output_dir, templates,
                generated_template_path("descriptor_module.rs.tmpl"),
                common_artifact_path("descriptors/shapes/" + snake_identifier(shape.name) + ".rs"),
                diagnostics, GeneratedArtifactTier::Common,
                rust_shape_descriptor_module_values(shape)
            );
        }
    }
    add_rust_descriptor_module_artifact(
        result, options, templates, "descriptors/runtime.rs", "runtime descriptors", diagnostics,
        TemplateRenderer::Values{
            {"descriptor_module_name", "runtime descriptors"},
            {"descriptor_module_content", "use super::*;\n\n" +
                                              generate_rust_feature_flag_descriptors(system) +
                                              generate_rust_runtime_descriptors(system)}
        }
    );
    for (const auto& [name, label] : rust_runtime_registration_modules(system))
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            generated_template_path("descriptor_module.rs.tmpl"),
            common_artifact_path("descriptors/runtime/" + name + ".rs"), diagnostics,
            GeneratedArtifactTier::Common,
            rust_runtime_registration_module_values(templates, name, label)
        );
    }
    for (const auto& entity : system.entities)
    {
        const auto entity_uses_gc = std::any_of(
            entity.states.begin(), entity.states.end(),
            [](const IrState& state) { return state.garbage_collection.has_value(); }
        );
        const auto entity_dir = "entities/" + snake_identifier(entity.name) + "/";
        std::string entity_mods =
            "pub mod constants;\npub mod model;\npub mod persistence;\npub mod schema;\n";
        if (entity_uses_gc)
        {
            entity_mods += "pub mod gc;\n";
        }
        add_rust_raw_common_file(result, options, entity_dir + "mod.rs", entity_mods);
        add_rust_raw_common_file(
            result, options, entity_dir + "constants.rs", rust_entity_constants_file(entity)
        );
        add_rust_raw_common_file(
            result, options, entity_dir + "model.rs",
            rust_entity_centered_facade_file(entity, "model")
        );
        add_rust_raw_common_file(
            result, options, entity_dir + "persistence.rs",
            rust_entity_centered_facade_file(entity, "persistence")
        );
        add_rust_raw_common_file(
            result, options, entity_dir + "schema.rs",
            rust_entity_centered_facade_file(entity, "schema")
        );
        if (entity_uses_gc)
        {
            add_rust_raw_common_file(
                result, options, entity_dir + "gc.rs", rust_entity_gc_descriptor_file(entity)
            );
        }
    }
    add_rust_raw_common_file(
        result, options, "entity_repository.rs",
        "use std::collections::BTreeMap;\n\n"
        "use crate::backend::{Backend, BackendError, BackendResult, ConflictKind, Transaction, "
        "VersionedRecord};\n"
        "use crate::descriptors::EntityDescriptor;\n"
        "use crate::json::Json;\n\n" +
            templates.load("generated/entity_repository.rs.tmpl")
    );
    add_rust_raw_common_file(
        result, options, "runtime_registration.rs",
        "use crate::backend::{Backend, BackendResult, Transaction};\n"
        "use crate::descriptors::*;\n"
        "use crate::feature_flag::FeatureFlagStore;\n"
        "use crate::lease::LeaseStore;\n"
        "use crate::log::LogSink;\n"
        "use crate::metric::MetricSink;\n"
        "use crate::queue::{QueueStore, RegisterQueueDefinitionRequest};\n"
        "use crate::workflow::{RegisterWorkflowDefinitionRequest, WorkflowStore};\n\n" +
            generate_rust_runtime_registration(system, templates)
    );
}

void add_rust_workflow_descriptor_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    for (const auto& workflow : system.workflows)
    {
        add_rust_raw_common_file(
            result, options, "workflows/" + snake_identifier(workflow.name) + ".rs",
            generate_rust_workflow_descriptor(workflow)
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

void add_rust_raw_api_file(
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

void add_rust_raw_worker_file(
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

std::string rust_worker_descriptor_catalog_file(const IrSystem& system)
{
    std::ostringstream out;
    for (const auto& worker : system.workers)
    {
        out << "#[path = \"" << snake_identifier(worker.name) << ".rs\"]\n";
        out << "mod descriptor_worker_" << snake_identifier(worker.name) << ";\n";
    }
    if (!system.workers.empty())
    {
        out << "\n";
    }
    out << "use crate::descriptor_types::{WorkerContext, WorkerDescriptor};\n\n";
    out << "pub fn worker_descriptors() -> Vec<WorkerDescriptor> {\n";
    out << "    let mut descriptors = Vec::new();\n";
    for (const auto& worker : system.workers)
    {
        out << "    descriptors.push(descriptor_worker_" << snake_identifier(worker.name)
            << "::worker_descriptor());\n";
    }
    out << "    descriptors\n";
    out << "}\n\n";
    out << "pub fn worker_contexts() -> Vec<WorkerContext> {\n";
    out << "    let mut contexts = Vec::new();\n";
    for (const auto& worker : system.workers)
    {
        out << "    contexts.push(descriptor_worker_" << snake_identifier(worker.name)
            << "::worker_context());\n";
    }
    out << "    contexts\n";
    out << "}\n";
    return out.str();
}

std::string rust_api_descriptor_module_name(const IrApi& api)
{
    return "api_descriptor_" + snake_identifier(api.name);
}

std::string rust_api_descriptor_function_name(const IrApi& api)
{
    return snake_identifier(api.name) + "_api_descriptors";
}

std::string rust_api_route_descriptor_function_name(const IrApi& api)
{
    return snake_identifier(api.name) + "_api_route_descriptors";
}

std::string rust_api_shape_module_for(
    const IrSystem& system,
    std::string_view shape_name
)
{
    const auto owner = rust_entity_api_shape_owner(system, shape_name);
    if (owner.has_value())
    {
        return "crate::api_shapes::entity_" + snake_identifier(*owner) + "_shapes";
    }
    return "crate::api_shapes::shape_" + snake_identifier(std::string{shape_name});
}

std::string rust_api_optional_shape_expr(
    const IrSystem& system,
    const std::optional<std::string>& value
)
{
    if (!value.has_value())
    {
        return "None";
    }
    if (rust_find_shape(system, *value) == nullptr)
    {
        return rust_optional_string_expr(value);
    }
    return "Some(" + rust_api_shape_module_for(system, *value) +
           "::" + rust_shape_name_constant_name(*value) + ".to_string())";
}

const IrEntity* rust_api_entity(
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

bool rust_entity_has_field(
    const IrEntity& entity,
    std::string_view field_name
)
{
    return std::any_of(
        entity.fields.begin(), entity.fields.end(),
        [&](const IrField& field) { return field.name == field_name; }
    );
}

bool rust_api_path_uses_entity_constants(
    const IrSystem& system,
    const IrApi& api
)
{
    const auto* entity = rust_api_entity(system, api);
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
        if (rust_entity_has_field(
                *entity, std::string_view{*api.path}.substr(pos + 1, end - pos - 1)
            ))
        {
            return true;
        }
        pos = end + 1;
    }
    return false;
}

std::string rust_api_optional_path_expr(
    const IrSystem& system,
    const IrApi& api
)
{
    if (!api.path.has_value())
    {
        return "None";
    }
    const auto* entity = rust_api_entity(system, api);
    if (entity == nullptr || !rust_api_path_uses_entity_constants(system, api))
    {
        return rust_optional_string_expr(api.path);
    }

    std::vector<std::string> parts;
    std::size_t cursor = 0;
    for (std::size_t pos = 0; (pos = api.path->find('{', cursor)) != std::string::npos;)
    {
        const auto end = api.path->find('}', pos + 1);
        if (end == std::string::npos)
        {
            return rust_optional_string_expr(api.path);
        }
        const auto field_name = api.path->substr(pos + 1, end - pos - 1);
        parts.push_back(rust_string(api.path->substr(cursor, pos - cursor + 1)));
        if (rust_entity_has_field(*entity, field_name))
        {
            parts.push_back(
                "entity_constants::" + rust_entity_field_constant_name(entity->name, field_name)
            );
        }
        else
        {
            parts.push_back(rust_string(field_name));
        }
        parts.push_back(rust_string("}"));
        cursor = end + 1;
    }
    if (cursor < api.path->size())
    {
        parts.push_back(rust_string(api.path->substr(cursor)));
    }
    std::ostringstream expr;
    expr << "Some([";
    for (std::size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
        {
            expr << ", ";
        }
        expr << parts[i];
    }
    expr << "].concat())";
    return expr.str();
}

bool rust_api_server_serves(
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

std::string rust_api_descriptor_module(
    const IrSystem& system,
    const IrApi& api
)
{
    std::ostringstream out;
    out << "use crate::descriptor_types::{ApiDescriptor, ApiRouteDescriptor};\n\n";
    if (const auto* entity = rust_api_entity(system, api);
        entity != nullptr && rust_api_path_uses_entity_constants(system, api))
    {
        out << "use crate::entity_" << snake_identifier(entity->name)
            << "::constants as entity_constants;\n\n";
    }
    out << "pub fn " << rust_api_descriptor_function_name(api) << "() -> Vec<ApiDescriptor> {\n";
    out << "    vec![\n";
    out << "        ApiDescriptor {\n";
    out << "            name: " << rust_string(api.name) << ".to_string(),\n";
    out << "            method: " << rust_optional_string_expr(api.method) << ",\n";
    out << "            path: " << rust_api_optional_path_expr(system, api) << ",\n";
    out << "            input: " << rust_api_optional_shape_expr(system, api.input) << ",\n";
    out << "            output: " << rust_api_optional_shape_expr(system, api.output) << ",\n";
    out << "            error: " << rust_optional_string_expr(api.error) << ",\n";
    out << "            starts_workflow: " << rust_optional_string_expr(api.starts_workflow)
        << ",\n";
    out << "            enqueues: " << rust_optional_string_expr(api.enqueues) << ",\n";
    out << "        },\n";
    out << "    ]\n";
    out << "}\n\n";
    out << "pub fn " << rust_api_route_descriptor_function_name(api)
        << "() -> Vec<ApiRouteDescriptor> {\n";
    out << "    vec![\n";
    for (const auto& api_server : system.api_servers)
    {
        if (!rust_api_server_serves(api_server, api.name))
        {
            continue;
        }
        out << "        ApiRouteDescriptor {\n";
        out << "            name: " << rust_string(api_server.name + "." + api.name)
            << ".to_string(),\n";
        out << "            server_name: " << rust_string(api_server.name) << ".to_string(),\n";
        out << "            api_name: " << rust_string(api.name) << ".to_string(),\n";
        out << "            method: " << rust_optional_string_expr(api.method) << ",\n";
        out << "            path: " << rust_api_optional_path_expr(system, api) << ",\n";
        out << "            input: " << rust_api_optional_shape_expr(system, api.input) << ",\n";
        out << "            output: " << rust_api_optional_shape_expr(system, api.output) << ",\n";
        out << "            error: " << rust_optional_string_expr(api.error) << ",\n";
        out << "        },\n";
    }
    out << "    ]\n";
    out << "}\n";
    return out.str();
}

void add_rust_api_descriptor_artifacts(
    GenerationResult& result,
    const BindingGeneratorOptions& options,
    const IrSystem& system
)
{
    for (const auto& api : system.apis)
    {
        std::string path = "api/descriptors/" + snake_identifier(api.name) + ".rs";
        if (const auto* entity = rust_api_entity(system, api); entity != nullptr)
        {
            path = "api/entities/" + snake_identifier(entity->name) + "/descriptors/" +
                   snake_identifier(api.name) + ".rs";
        }
        add_rust_raw_api_file(result, options, path, rust_api_descriptor_module(system, api));
    }
}

std::string rust_api_descriptor_catalog_file(const IrSystem& system)
{
    std::ostringstream modules;
    std::ostringstream api_aggregation;
    std::ostringstream route_aggregation;
    const auto entity_domains = crud_api_handler_domains_rs(api_handler_domains(system));
    std::set<std::string> entity_api_names;
    for (const auto& domain : entity_domains)
    {
        modules << "#[path = \"../entities/" << snake_identifier(domain.name) << "/catalog.rs\"]\n";
        modules << "mod entity_" << snake_identifier(domain.name) << "_catalog;\n";
        api_aggregation << "    descriptors.extend(entity_" << snake_identifier(domain.name)
                        << "_catalog::api_descriptors());\n";
        route_aggregation << "    descriptors.extend(entity_" << snake_identifier(domain.name)
                          << "_catalog::api_route_descriptors());\n";
        for (const auto& api : domain.apis)
        {
            entity_api_names.insert(api.name);
        }
    }
    for (const auto& api : system.apis)
    {
        if (entity_api_names.contains(api.name))
        {
            continue;
        }
        const auto module = rust_api_descriptor_module_name(api);
        modules << "#[path = \"" << snake_identifier(api.name) << ".rs\"]\n";
        modules << "mod " << module << ";\n";
        api_aggregation << "    descriptors.extend(" << module
                        << "::" << rust_api_descriptor_function_name(api) << "());\n";
        route_aggregation << "    descriptors.extend(" << module
                          << "::" << rust_api_route_descriptor_function_name(api) << "());\n";
    }
    std::ostringstream out;
    out << modules.str() << "\n";
    out << "use crate::api_shapes;\n";
    out << "use crate::descriptor_types::{ApiDescriptor, ApiRouteDescriptor};\n\n";
    out << "pub fn shape_descriptors() -> Vec<crate::shape_types::ShapeDescriptor> {\n";
    out << "    api_shapes::shape_descriptors()\n";
    out << "}\n\n";
    out << "pub fn api_descriptors() -> Vec<ApiDescriptor> {\n";
    out << "    let mut descriptors = Vec::new();\n";
    out << api_aggregation.str();
    out << "    descriptors\n";
    out << "}\n\n";
    out << "pub fn api_route_descriptors() -> Vec<ApiRouteDescriptor> {\n";
    out << "    let mut descriptors = Vec::new();\n";
    out << route_aggregation.str();
    out << "    descriptors\n";
    out << "}\n";
    return out.str();
}

std::string rust_api_descriptors_file(const IrSystem& system)
{
    std::ostringstream out;
    out << "#[path = \"descriptors/catalog.rs\"]\n";
    out << "mod catalog;\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "#[path = \"servers/" << snake_identifier(api_server.name) << "/catalog.rs\"]\n";
        out << "mod server_" << snake_identifier(api_server.name) << "_catalog;\n";
    }
    out << "\n";
    out << "pub use crate::descriptor_types::{ApiDescriptor, ApiRouteDescriptor, "
           "ApiServerDescriptor};\n\n";
    out << "pub fn api_descriptors() -> Vec<ApiDescriptor> {\n";
    out << "    catalog::api_descriptors()\n";
    out << "}\n\n";
    out << "pub fn api_server_descriptors() -> Vec<ApiServerDescriptor> {\n";
    out << "    let mut descriptors = Vec::new();\n";
    for (const auto& api_server : system.api_servers)
    {
        out << "    descriptors.extend(server_" << snake_identifier(api_server.name)
            << "_catalog::api_server_descriptors());\n";
    }
    out << "    descriptors\n";
    out << "}\n\n";
    out << "pub fn api_route_descriptors() -> Vec<ApiRouteDescriptor> {\n";
    out << "    catalog::api_route_descriptors()\n";
    out << "}\n";
    return out.str();
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
        rust_makefile_values(options.tier, system), common_artifact_path(GeneratedArtifactMakefile)
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
    if (system.apis.empty() && system.api_servers.empty())
    {
        return;
    }

    const auto include_api_composition = !system.api_servers.empty();

    add_rust_api_shape_type_artifacts(result, options, system);
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
                rust_api_server_entity_catalog_file(api_server, domain)
            );
        }
        add_rust_raw_api_file(
            result, options, "api/servers/" + snake_identifier(api_server.name) + "/catalog.rs",
            rust_api_server_catalog_file(system, api_server)
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
                result, options, "worker/workflows/" + snake_identifier(workflow.name) + ".rs",
                generate_rust_workflow_worker_module(workflow)
            );
        }
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

} // namespace statespec
