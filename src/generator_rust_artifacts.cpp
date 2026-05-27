#include "generator_rust_artifacts.hpp"

#include "generator_artifact_paths.hpp"
#include "generator_entity_index_helpers.hpp"
#include "generator_rust_descriptor_areas.hpp"
#include "generator_rust_descriptor_support.hpp"
#include "generator_rust_descriptors.hpp"
#include "generator_support.hpp"
#include "identifier_case.hpp"
#include "statespec/runtime_usage.hpp"

#include <algorithm>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace statespec
{

namespace
{

TemplateRenderer::Values rust_makefile_values(BindingGenerationTier tier, const IrSystem& system)
{
    const auto usage = runtime_domain_usage(system);
    const auto include_api =
        tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api;
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
        help_target_additions
            << "\t@printf '%s\\n' '  check-api     build-api     package-api'\n";
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
        (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Worker) &&
        (!system.workers.empty() || usage.uses_workflows);
    const auto include_worker_composition = include_worker && !system.workers.empty();
    const auto include_worker_execution =
        include_worker && (include_worker_composition || usage.uses_workflows);
    std::ostringstream runtime_modules;
    std::ostringstream api_modules;
    std::ostringstream worker_modules;
    if (!rust_shapes_matching(system, false).shapes.empty())
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
        runtime_modules << "#[path = \"common/runtime/entity_gc_descriptors.rs\"]\n";
        runtime_modules << "pub mod runtime_entity_gc_descriptors;\n";
        runtime_modules << "#[path = \"common/runtime/entity_gc_repository.rs\"]\n";
        runtime_modules << "pub mod runtime_entity_gc_repository;\n";
        runtime_modules << "#[path = \"common/runtime/entity_gc_workers.rs\"]\n";
        runtime_modules << "pub mod runtime_entity_gc_workers;\n";
    }
    for (const auto& entity : system.entities)
    {
        runtime_modules << "#[path = \"common/entities/" << snake_identifier(entity.name)
                        << "/mod.rs\"]\n";
        runtime_modules << "pub mod entity_" << snake_identifier(entity.name) << ";\n";
    }
    if (tier == BindingGenerationTier::All || tier == BindingGenerationTier::Api)
    {
        if (!rust_shapes_matching(system, true).shapes.empty())
        {
            api_modules << "#[path = \"api/shapes.rs\"]\n";
            api_modules << "pub mod api_shapes;\n";
        }
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
    return "api_handler_registry_" + snake_identifier(std::string{domain_name});
}

std::string rust_api_handler_domain_type_name(std::string_view domain_name)
{
    return "Default" + pascal_identifier(std::string{domain_name}) + "ApiHandler";
}

std::string rust_api_handler_domain_path(std::string_view domain_name)
{
    return "api/handlers/" + snake_identifier(std::string{domain_name}) + ".rs";
}

std::string rust_api_handler_registry_domain_modules(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        out << "#[path = \"handlers/" << snake_identifier(domain.name) << ".rs\"]\n";
        out << "mod " << rust_api_handler_domain_module_name(domain.name) << ";\n";
    }
    return out.str();
}

std::string rust_api_handler_registry_delegates(const std::vector<ApiHandlerDomain>& domains)
{
    std::ostringstream out;
    for (const auto& domain : domains)
    {
        const auto module_name = rust_api_handler_domain_module_name(domain.name);
        const auto type_name = rust_api_handler_domain_type_name(domain.name);
        for (const auto& api : domain.apis)
        {
            out << "    fn handle_" << snake_identifier(api.name)
                << "(&self, context: &ApiRequestContext) -> BackendResult<ApiResponse> {\n";
            out << "        " << module_name << "::" << type_name
                << " { backend: &self.backend }.handle_" << snake_identifier(api.name)
                << "(context)\n";
            out << "    }\n\n";
        }
    }
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

std::string rust_api_codec_modules(const std::vector<IrShape>& shapes)
{
    std::ostringstream out;
    for (const auto& shape : shapes)
    {
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
    std::ostringstream imports;
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
            terminal_states
                << "            EntityGcTerminalStateDescriptor {\n"
                << "                state: " << snake_identifier(entity.name)
                << "_model::" << rust_entity_state_constant_name(entity.name, state.name)
                << ".to_string(),\n"
                << "                after: " << snake_identifier(entity.name) << "_schema::"
                << upper_snake_identifier(entity.name + "_state_" + state.name + "_gc_after")
                << ".to_string(),\n"
                << "                mode: " << snake_identifier(entity.name) << "_schema::"
                << upper_snake_identifier(entity.name + "_state_" + state.name + "_gc_mode")
                << ".to_string(),\n"
                << "            },\n";
        }
        if (terminal_states.str().empty())
        {
            continue;
        }
        imports << "use crate::entity_" << snake_identifier(entity.name) << "::model as "
                << snake_identifier(entity.name) << "_model;\n";
        imports << "use crate::entity_" << snake_identifier(entity.name) << "::schema as "
                << snake_identifier(entity.name) << "_schema;\n";
        descriptors << "        EntityGcDescriptor {\n"
                    << "            entity: " << snake_identifier(entity.name)
                    << "_model::" << rust_entity_name_constant_name(entity.name)
                    << ".to_string(),\n"
                    << "            collection: " << snake_identifier(entity.name)
                    << "_model::" << rust_entity_name_constant_name(entity.name)
                    << ".to_string(),\n"
                    << "            terminal_states: vec![\n"
                    << terminal_states.str() << "            ],\n"
                    << "        },\n";
    }
    if (!imports.str().empty())
    {
        imports << "\n";
    }
    return TemplateRenderer::Values{
        {"entity_gc_entity_imports", imports.str()},
        {"entity_gc_descriptors", descriptors.str()},
    };
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
        out << "pub const " << rust_entity_name_constant_name(entity.name)
            << ": &str = " << rust_string(entity.name) << ";\n";
        for (const auto& field : entity.fields)
        {
            out << "pub const " << rust_entity_field_constant_name(entity.name, field.name)
                << ": &str = " << rust_string(field.name) << ";\n";
            out << "pub const "
                << rust_entity_field_type_name_constant_name(entity.name, field.name)
                << ": &str = " << rust_string(field.type) << ";\n";
        }
        for (const auto& index : entity.indexes)
        {
            out << "pub const " << rust_entity_index_constant_name(entity.name, index.name)
                << ": &str = " << rust_string(index.name) << ";\n";
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
                    << "_ON_PARENT_DELETE: &str = " << rust_string(*relation.on_parent_delete)
                    << ";\n";
            }
        }
        out << "\n";
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
        out << "use super::model::*;\n";
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
    out << "use crate::backend::{Backend, BackendResult};\n";
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

std::string rust_descriptor_types_file()
{
    return
        "use std::time::Duration;\n\n"
        "use crate::json::Json;\n\n"
        "#[derive(Debug, Clone)]\n"
        "pub struct LeaseDefinition {\n"
        "    pub name: String,\n"
        "    pub resource: Option<String>,\n"
        "    pub ttl: Duration,\n"
        "    pub renew_every: Option<Duration>,\n"
        "    pub holder: Option<String>,\n"
        "    pub fencing_token: bool,\n"
        "    pub max_ttl: Option<Duration>,\n"
        "}\n\n"
        "#[derive(Debug, Clone)]\n"
        "pub struct ApiDescriptor {\n"
        "    pub name: String,\n"
        "    pub method: Option<String>,\n"
        "    pub path: Option<String>,\n"
        "    pub input: Option<String>,\n"
        "    pub output: Option<String>,\n"
        "    pub error: Option<String>,\n"
        "    pub starts_workflow: Option<String>,\n"
        "    pub enqueues: Option<String>,\n"
        "}\n\n"
        "#[derive(Debug, Clone)]\n"
        "pub struct ApiServerDescriptor {\n"
        "    pub name: String,\n"
        "    pub serves: Vec<String>,\n"
        "    pub concurrency: i32,\n"
        "}\n\n"
        "#[derive(Debug, Clone)]\n"
        "pub struct ApiRouteDescriptor {\n"
        "    pub name: String,\n"
        "    pub server_name: String,\n"
        "    pub api_name: String,\n"
        "    pub method: Option<String>,\n"
        "    pub path: Option<String>,\n"
        "    pub input: Option<String>,\n"
        "    pub output: Option<String>,\n"
        "    pub error: Option<String>,\n"
        "}\n\n"
        "#[derive(Debug, Clone)]\n"
        "pub struct ApiRequestContext {\n"
        "    pub server_name: String,\n"
        "    pub api_name: String,\n"
        "    pub method: Option<String>,\n"
        "    pub path: Option<String>,\n"
        "    pub body: Json,\n"
        "}\n\n"
        "#[derive(Debug, Clone)]\n"
        "pub struct ApiResponse {\n"
        "    pub status_code: i32,\n"
        "    pub body: Json,\n"
        "}\n\n"
        "#[derive(Debug, Clone)]\n"
        "pub struct WorkerDescriptor {\n"
        "    pub name: String,\n"
        "    pub singleton: bool,\n"
        "    pub lease: Option<String>,\n"
        "    pub polls: Option<String>,\n"
        "    pub executes: Option<String>,\n"
        "    pub concurrency: i32,\n"
        "}\n\n"
        "#[derive(Debug, Clone)]\n"
        "pub struct WorkerContext {\n"
        "    pub worker_name: String,\n"
        "    pub singleton: bool,\n"
        "    pub lease: Option<String>,\n"
        "    pub polls: Option<String>,\n"
        "    pub executes: Option<String>,\n"
        "    pub concurrency: i32,\n"
        "}\n";
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
    if (api_shapes.shapes.empty())
    {
        return;
    }
    add_rust_raw_api_file(result, options, "api/shapes.rs", rust_shapes_module(api_shapes));
    for (const auto& shape : api_shapes.shapes)
    {
        add_rust_raw_api_file(
            result, options, "api/shapes/" + snake_identifier(shape.name) + ".rs",
            rust_shape_type_file(shape)
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
    add_rust_raw_common_file(result, options, "descriptors/types.rs", rust_descriptor_types_file());
    add_rust_descriptor_module_artifact(
        result, options, templates, "descriptors/core.rs", "core descriptors", diagnostics
    );
    add_rust_raw_common_file(
        result, options, "descriptors/events.rs", rust_event_descriptor_module(system)
    );
    add_rust_raw_common_file(
        result, options, "descriptors/external_systems.rs",
        rust_external_system_descriptor_module(system, templates)
    );
    add_rust_descriptor_module_artifact(
        result, options, templates, "descriptors/shapes.rs", "shape descriptors", diagnostics,
        rust_shape_descriptor_module_values(system)
    );
    for (const auto& shape : system.shapes)
    {
        add_generated_template_file(
            result, options.output_dir, templates,
            generated_template_path("descriptor_module.rs.tmpl"),
            common_artifact_path("descriptors/shapes/" + snake_identifier(shape.name) + ".rs"),
            diagnostics, GeneratedArtifactTier::Common, rust_shape_descriptor_module_values(shape)
        );
    }
    add_rust_descriptor_module_artifact(
        result, options, templates, "descriptors/runtime.rs", "runtime descriptors", diagnostics
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
        const auto entity_dir = "entities/" + snake_identifier(entity.name) + "/";
        add_rust_raw_common_file(
            result, options, entity_dir + "mod.rs",
            "pub mod model;\npub mod persistence;\npub mod schema;\n"
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
    out << "pub fn " << rust_api_descriptor_function_name(api) << "() -> Vec<ApiDescriptor> {\n";
    out << "    vec![\n";
    out << "        ApiDescriptor {\n";
    out << "            name: " << rust_string(api.name) << ".to_string(),\n";
    out << "            method: " << rust_optional_string_expr(api.method) << ",\n";
    out << "            path: " << rust_optional_string_expr(api.path) << ",\n";
    out << "            input: " << rust_optional_string_expr(api.input) << ",\n";
    out << "            output: " << rust_optional_string_expr(api.output) << ",\n";
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
        out << "            path: " << rust_optional_string_expr(api.path) << ",\n";
        out << "            input: " << rust_optional_string_expr(api.input) << ",\n";
        out << "            output: " << rust_optional_string_expr(api.output) << ",\n";
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
        add_rust_raw_api_file(
            result, options, "api/descriptors/" + snake_identifier(api.name) + ".rs",
            rust_api_descriptor_module(system, api)
        );
    }
}

std::string rust_api_descriptor_catalog_file(const IrSystem& system)
{
    std::ostringstream modules;
    std::ostringstream api_aggregation;
    std::ostringstream route_aggregation;
    std::ostringstream server_descriptors;
    for (const auto& api : system.apis)
    {
        const auto module = rust_api_descriptor_module_name(api);
        modules << "#[path = \"" << snake_identifier(api.name) << ".rs\"]\n";
        modules << "mod " << module << ";\n";
        api_aggregation << "    descriptors.extend(" << module
                        << "::" << rust_api_descriptor_function_name(api) << "());\n";
        route_aggregation << "    descriptors.extend(" << module
                          << "::" << rust_api_route_descriptor_function_name(api) << "());\n";
    }
    server_descriptors << "    vec![\n";
    for (const auto& api_server : system.api_servers)
    {
        server_descriptors << "        ApiServerDescriptor {\n";
        server_descriptors << "            name: " << rust_string(api_server.name)
                           << ".to_string(),\n";
        server_descriptors << "            serves: vec![";
        for (std::size_t i = 0; i < api_server.serves.size(); ++i)
        {
            if (i > 0)
            {
                server_descriptors << ", ";
            }
            server_descriptors << rust_string(api_server.serves[i]) << ".to_string()";
        }
        server_descriptors << "],\n";
        server_descriptors << "            concurrency: " << api_server.concurrency.value_or(1)
                           << ",\n";
        server_descriptors << "        },\n";
    }
    server_descriptors << "    ]\n";
    std::ostringstream out;
    out << modules.str() << "\n";
    out << "use crate::descriptor_types::{ApiDescriptor, ApiRouteDescriptor, ApiServerDescriptor};\n\n";
    out << "pub fn api_descriptors() -> Vec<ApiDescriptor> {\n";
    out << "    let mut descriptors = Vec::new();\n";
    out << api_aggregation.str();
    out << "    descriptors\n";
    out << "}\n\n";
    out << "pub fn api_server_descriptors() -> Vec<ApiServerDescriptor> {\n";
    out << server_descriptors.str();
    out << "}\n\n";
    out << "pub fn api_route_descriptors() -> Vec<ApiRouteDescriptor> {\n";
    out << "    let mut descriptors = Vec::new();\n";
    out << route_aggregation.str();
    out << "    descriptors\n";
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
    const auto include_api_composition = !system.api_servers.empty();

    add_rust_api_shape_type_artifacts(result, options, system);
    add_rust_api_descriptor_artifacts(result, options, system);
    add_rust_raw_api_file(
        result, options, "api/descriptors/catalog.rs", rust_api_descriptor_catalog_file(system)
    );
    add_rust_generated_template_file(
        result, options, templates, "api/api_descriptors.rs", GeneratedArtifactTier::Api,
        diagnostics
    );
    add_rust_generated_template_file(
        result, options, templates, "api/api_codecs.rs", GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_codec_modules", rust_api_codec_modules(api_codec_shapes(system))},
            {"api_codec_helpers", generate_api_codec_helpers_rs()},
            {"api_shape_import", rust_api_shape_import(system)}
        }
    );
    for (const auto& shape : api_codec_shapes(system))
    {
        add_rust_raw_api_file(
            result, options, rust_api_codec_shape_path(shape.name),
            rust_api_codec_shape_file(system, shape)
        );
    }
    add_rust_generated_template_file(
        result, options, templates, "api/api_handlers.rs", GeneratedArtifactTier::Api, diagnostics,
        TemplateRenderer::Values{
            {"api_operation_handler_methods", generate_api_operation_handler_methods_rs(system)}
        }
    );
    const auto handler_domains = api_handler_domains(system);
    for (const auto& domain : handler_domains)
    {
        add_rust_raw_api_file(
            result, options, rust_api_handler_domain_path(domain.name),
            rust_api_handler_domain_file(system, domain)
        );
    }
    add_rust_generated_template_file(
        result, options, templates, "api/api_handler_registry.rs", GeneratedArtifactTier::Api,
        diagnostics,
        TemplateRenderer::Values{
            {"api_operation_default_handler_methods",
             rust_api_handler_registry_delegates(handler_domains)},
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
        add_rust_generated_template_file(
            result, options, templates, "worker/main.rs", GeneratedArtifactTier::Worker, diagnostics
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
