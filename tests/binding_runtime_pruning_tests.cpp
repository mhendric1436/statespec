#include "binding_test_support.hpp"

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace
{

struct LanguageRuntimePaths
{
    statespec::BindingLanguage language;
    std::string name;
    std::string descriptors_path;
    std::string module_manifest_path;
    std::vector<std::string> common_codec_paths;
    std::vector<std::string> feature_flag_paths;
    std::vector<std::string> queue_paths;
    std::vector<std::string> lease_paths;
    std::vector<std::string> workflow_paths;
    std::vector<std::string> observability_paths;
    std::vector<std::string> entity_gc_paths;
    std::vector<std::string> worker_queue_paths;
    std::vector<std::string> worker_lease_paths;
    std::vector<std::string> worker_workflow_paths;
    std::vector<std::string> worker_execution_paths;
    std::vector<std::string> api_app_paths;
    std::vector<std::string> worker_app_paths;
    std::string api_runtime_path;
    std::string api_process_path;
    std::string worker_runtime_path;
};

statespec::Spec runtime_pruning_spec(std::string name)
{
    statespec::Spec spec;
    statespec::SystemDecl system;
    system.name = std::move(name);
    spec.system = std::move(system);
    return spec;
}

statespec::FeatureFlagDecl feature_flag_decl()
{
    statespec::FeatureFlagDecl flag;
    flag.name = "FeatureFlagOnly";
    flag.type = "bool";
    flag.default_value = "false";
    flag.scope = "tenant";
    return flag;
}

statespec::QueueDecl queue_decl()
{
    statespec::QueueDecl queue;
    queue.name = "QueueOnly";
    queue.channel = "default";
    queue.visibility_timeout = "PT30S";
    queue.max_attempts = 3;

    statespec::MessageDecl message;
    message.name = "QueueMessage";
    message.idempotency_key = "message_id";
    message.payload_fields.push_back(statespec::FieldDecl{"message_id", "string", {}});
    queue.messages.push_back(std::move(message));
    return queue;
}

statespec::LeaseDecl lease_decl()
{
    statespec::LeaseDecl lease;
    lease.name = "LeaseOnly";
    lease.resource = "runtime:lease";
    lease.ttl = "PT30S";
    lease.renew_every = "PT10S";
    lease.holder = "worker_id";
    lease.fencing_token = true;
    lease.max_ttl = "PT5M";
    return lease;
}

statespec::WorkflowDecl workflow_decl()
{
    statespec::WorkflowDecl workflow;
    workflow.name = "WorkflowOnly";
    workflow.version = 1;
    workflow.singleton = false;
    workflow.expected_execution_time = "PT5M";
    workflow.start_step = "start";

    statespec::WorkflowStepDecl step;
    step.name = "start";
    step.expected_execution_time = "PT5S";
    step.max_retries = 2;
    workflow.steps.push_back(std::move(step));
    return workflow;
}

statespec::LogDecl log_decl()
{
    statespec::LogDecl log;
    log.name = "LogOnly";
    log.level = "info";
    log.event_name = "runtime.log.only";
    log.fields.push_back(statespec::FieldDecl{"tenant_id", "string", {}});
    return log;
}

statespec::MetricDecl metric_decl()
{
    statespec::MetricDecl metric;
    metric.name = "MetricOnly";
    metric.kind = "counter";
    metric.backend_name = "runtime_metric_only_total";
    metric.unit = "count";
    metric.labels.push_back(statespec::FieldDecl{"tenant_id", "string", {}});
    return metric;
}

statespec::ApiDecl workflow_api_decl()
{
    statespec::ApiDecl api;
    api.name = "StartWorkflowOnly";
    api.method = "POST";
    api.path = "/workflow-only";
    api.starts_workflow = "WorkflowOnly";
    return api;
}

statespec::ApiServerDecl api_server_decl()
{
    statespec::ApiServerDecl server;
    server.name = "RuntimeApi";
    server.serves.push_back("StartWorkflowOnly");
    server.concurrency = 4;
    return server;
}

statespec::WorkerDecl worker_decl()
{
    statespec::WorkerDecl worker;
    worker.name = "RuntimeWorker";
    worker.singleton = true;
    worker.lease = "LeaseOnly";
    worker.polls = "QueueOnly.QueueMessage";
    worker.executes = "WorkflowOnly";
    worker.concurrency = 2;
    return worker;
}

statespec::Spec only_feature_flags_spec()
{
    auto spec = runtime_pruning_spec("OnlyFeatureFlags");
    spec.system->feature_flags.push_back(feature_flag_decl());
    return spec;
}

statespec::Spec only_queues_spec()
{
    auto spec = runtime_pruning_spec("OnlyQueues");
    spec.system->queues.push_back(queue_decl());
    return spec;
}

statespec::Spec only_workflows_spec()
{
    auto spec = runtime_pruning_spec("OnlyWorkflows");
    spec.system->workflows.push_back(workflow_decl());
    return spec;
}

statespec::Spec only_observability_spec()
{
    auto spec = runtime_pruning_spec("OnlyObservability");
    spec.system->logs.push_back(log_decl());
    spec.system->metrics.push_back(metric_decl());
    return spec;
}

statespec::Spec mixed_api_worker_spec()
{
    auto spec = runtime_pruning_spec("MixedApiWorker");
    spec.system->queues.push_back(queue_decl());
    spec.system->leases.push_back(lease_decl());
    spec.system->workflows.push_back(workflow_decl());
    spec.system->apis.push_back(workflow_api_decl());
    spec.system->api_servers.push_back(api_server_decl());
    spec.system->workers.push_back(worker_decl());
    return spec;
}

statespec::EntityDecl gc_entity_decl()
{
    statespec::EntityDecl entity;
    entity.name = "GcTask";
    entity.key_fields = {"task_id"};
    entity.ownership = statespec::OwnershipDecl{"system", "self", "authoritative", {}};
    entity.fields.push_back(statespec::FieldDecl{"created_at", "timestamp", {}});
    entity.fields.push_back(statespec::FieldDecl{"updated_at", "timestamp", {}});
    entity.fields.push_back(statespec::FieldDecl{"status", "string", {}});
    entity.fields.push_back(statespec::FieldDecl{"task_id", "string", {}});

    statespec::StateMachineDecl state_machine;
    state_machine.states.push_back(statespec::StateDecl{"Active", false, std::nullopt, {}, {}});
    state_machine.states.push_back(
        statespec::StateDecl{
            "Deleted",
            true,
            statespec::GarbageCollectionPolicyDecl{"P30D", "tombstone", {}},
            {},
            {},
        }
    );
    state_machine.initial_state = "Active";
    state_machine.terminal_states.push_back("Deleted");
    state_machine.transitions.push_back(statespec::TransitionDecl{"Active", "Deleted", {}});
    entity.state_machine = std::move(state_machine);
    return entity;
}

statespec::ApiDecl gc_api_decl()
{
    statespec::ApiDecl api;
    api.name = "TouchGcTask";
    api.method = "POST";
    api.path = "/gc-tasks/{task_id}/touch";
    return api;
}

statespec::ApiServerDecl gc_api_server_decl()
{
    statespec::ApiServerDecl server;
    server.name = "GcApi";
    server.serves.push_back("TouchGcTask");
    server.concurrency = 2;
    return server;
}

statespec::Spec api_only_gc_deployment_spec()
{
    auto spec = runtime_pruning_spec("ApiOnlyGcDeployment");
    spec.system->entities.push_back(gc_entity_decl());
    spec.system->apis.push_back(gc_api_decl());
    spec.system->api_servers.push_back(gc_api_server_decl());
    return spec;
}

statespec::Spec worker_only_gc_deployment_spec()
{
    auto spec = runtime_pruning_spec("WorkerOnlyGcDeployment");
    spec.system->entities.push_back(gc_entity_decl());
    spec.system->queues.push_back(queue_decl());
    spec.system->leases.push_back(lease_decl());
    spec.system->workflows.push_back(workflow_decl());
    spec.system->workers.push_back(worker_decl());
    return spec;
}

statespec::Spec mixed_gc_deployment_spec()
{
    auto spec = worker_only_gc_deployment_spec();
    spec.system->name = "MixedGcDeployment";
    spec.system->apis.push_back(gc_api_decl());
    spec.system->api_servers.push_back(gc_api_server_decl());
    return spec;
}

statespec::GenerationResult generate_runtime_fixture(
    const statespec::Spec& spec,
    const LanguageRuntimePaths& paths,
    std::string_view fixture_name
)
{
    statespec::DiagnosticBag diagnostics;
    const auto result = statespec::generate_bindings(
        spec,
        statespec::BindingGeneratorOptions{
            paths.language,
            std::filesystem::path{"/tmp/statespec-runtime-pruning-test"} / paths.name /
                std::string{fixture_name},
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );
    require(
        !diagnostics.has_errors(), paths.name + " " + std::string{fixture_name} + " should generate"
    );
    return result;
}

bool has_artifact(
    const statespec::GenerationResult& result,
    const std::string& artifact_path
)
{
    for (const auto& file : result.files)
    {
        if (file.artifact_path == artifact_path)
        {
            return true;
        }
    }
    return false;
}

std::string artifact_content(
    const statespec::GenerationResult& result,
    const std::string& artifact_path
)
{
    for (const auto& file : result.files)
    {
        if (file.artifact_path == artifact_path)
        {
            return file.content;
        }
    }
    require(false, "expected generated artifact " + artifact_path);
    return {};
}

bool is_entity_descriptor_module_path(std::string_view artifact_path)
{
    return artifact_path.find("/descriptors/entities/") != std::string_view::npos ||
           (artifact_path.find("common/backend/") == 0 &&
            artifact_path.ends_with("_descriptors.go"));
}

std::string descriptor_content(
    const statespec::GenerationResult& result,
    const LanguageRuntimePaths& paths
)
{
    std::string content = artifact_content(result, paths.descriptors_path);
    for (const auto& file : result.files)
    {
        if (is_entity_descriptor_module_path(file.artifact_path))
        {
            content += '\n';
            content += file.content;
        }
    }
    return content;
}

void require_artifacts_present(
    const statespec::GenerationResult& result,
    const std::vector<std::string>& artifact_paths,
    const std::string& context
)
{
    for (const auto& path : artifact_paths)
    {
        require(has_artifact(result, path), context + " should emit " + path);
    }
}

void require_artifacts_absent(
    const statespec::GenerationResult& result,
    const std::vector<std::string>& artifact_paths,
    const std::string& context
)
{
    for (const auto& path : artifact_paths)
    {
        require(!has_artifact(result, path), context + " should not emit " + path);
    }
}

void require_contains(
    const std::string& content,
    const std::string& needle,
    const std::string& context
)
{
    require(content.find(needle) != std::string::npos, context + " should contain " + needle);
}

void require_not_contains(
    const std::string& content,
    const std::string& needle,
    const std::string& context
)
{
    require(content.find(needle) == std::string::npos, context + " should not contain " + needle);
}

std::vector<std::string> concat(std::initializer_list<std::vector<std::string>> groups)
{
    std::vector<std::string> paths;
    for (const auto& group : groups)
    {
        paths.insert(paths.end(), group.begin(), group.end());
    }
    return paths;
}

std::vector<std::string> all_runtime_paths(const LanguageRuntimePaths& paths)
{
    return concat(
        {paths.common_codec_paths, paths.feature_flag_paths, paths.queue_paths, paths.lease_paths,
         paths.workflow_paths, paths.observability_paths, paths.worker_queue_paths,
         paths.entity_gc_paths, paths.worker_lease_paths, paths.worker_workflow_paths,
         paths.worker_execution_paths}
    );
}

const std::array<
    LanguageRuntimePaths,
    4>&
language_runtime_paths()
{
    static const std::array<LanguageRuntimePaths, 4> paths{{
        {
            statespec::BindingLanguage::Cpp,
            "cpp",
            "common/descriptors.hpp",
            "common/Makefile",
            {"common/runtime/codec.hpp", "common/runtime/codec_core.hpp"},
            {"common/runtime/codec_feature_flags.hpp", "common/runtime/feature_flag_store.hpp"},
            {"common/runtime/codec_queues.hpp", "common/runtime/queue_store.hpp"},
            {"common/runtime/codec_leases.hpp", "common/runtime/lease_store.hpp"},
            {"common/runtime/codec_workflows.hpp", "common/runtime/workflow_store.hpp"},
            {"common/runtime/codec_observability.hpp", "common/runtime/codec_logs.hpp",
             "common/runtime/log_sink.hpp", "common/runtime/codec_metrics.hpp",
             "common/runtime/metric_sink.hpp"},
            {"common/runtime/entity_gc_descriptors.hpp", "common/runtime/entity_gc_repository.hpp",
             "common/runtime/entity_gc_workers.hpp"},
            {"worker/worker_queues.hpp"},
            {"worker/worker_leases.hpp"},
            {"worker/worker_workflows.hpp"},
            {"worker/workflow_runner.hpp", "worker/workflow_step_handlers.hpp"},
            {"api/api_application.hpp", "api/api_dispatcher.hpp", "api/api_process.hpp",
             "api/api_routes.hpp", "api/api_server.hpp", "api/api_transport.hpp", "api/main.cpp"},
            {"worker/worker_application.hpp", "worker/worker_process.hpp",
             "worker/worker_runtime.hpp", "worker/main.cpp"},
            "api/api_application.hpp",
            "api/api_process.hpp",
            "worker/worker_runtime.hpp",
        },
        {
            statespec::BindingLanguage::Go,
            "go",
            "common/backend/descriptors.go",
            "common/Makefile",
            {"common/backend/runtime/codec.go"},
            {"common/backend/runtime/codec_feature_flags.go",
             "common/backend/runtime/feature_flags.go"},
            {"common/backend/runtime/codec_queues.go", "common/backend/runtime/queues.go"},
            {"common/backend/runtime/codec_leases.go", "common/backend/runtime/leases.go"},
            {"common/backend/runtime/codec_workflows.go", "common/backend/runtime/workflows.go"},
            {"common/backend/runtime/codec_observability.go",
             "common/backend/runtime/codec_logs.go", "common/backend/runtime/logs.go",
             "common/backend/runtime/codec_metrics.go", "common/backend/runtime/metrics.go"},
            {"common/backend/runtime/entity_gc_descriptors.go",
             "common/backend/runtime/entity_gc_repository.go",
             "common/backend/runtime/entity_gc_workers.go"},
            {"worker/backend/worker_queues.go"},
            {"worker/backend/worker_leases.go"},
            {"worker/backend/worker_workflows.go"},
            {"worker/backend/workflow_runner.go", "worker/backend/workflow_step_handlers.go"},
            {"api/backend/api_application.go", "api/backend/api_dispatcher.go",
             "api/backend/api_process.go", "api/backend/api_routes.go", "api/backend/api_server.go",
             "api/backend/api_transport.go", "api/cmd/api/main.go"},
            {"worker/backend/worker_application.go", "worker/backend/worker_process.go",
             "worker/backend/worker_runtime.go", "worker/cmd/worker/main.go"},
            "api/backend/api_application.go",
            "api/backend/api_process.go",
            "worker/backend/worker_runtime.go",
        },
        {
            statespec::BindingLanguage::Java,
            "java",
            "common/com/statespec/generated/Descriptors.java",
            "common/Makefile",
            {"common/com/statespec/backend/runtime/Codec.java"},
            {"common/com/statespec/backend/runtime/FeatureFlagCodec.java",
             "common/com/statespec/backend/runtime/FeatureFlagStore.java"},
            {"common/com/statespec/backend/runtime/QueueCodec.java",
             "common/com/statespec/backend/runtime/QueueStore.java"},
            {"common/com/statespec/backend/runtime/LeaseCodec.java",
             "common/com/statespec/backend/runtime/LeaseStore.java"},
            {"common/com/statespec/backend/runtime/WorkflowCodec.java",
             "common/com/statespec/backend/runtime/WorkflowStore.java"},
            {"common/com/statespec/backend/runtime/ObservabilityCodec.java",
             "common/com/statespec/backend/runtime/LogCodec.java",
             "common/com/statespec/backend/runtime/LogSink.java",
             "common/com/statespec/backend/runtime/MetricCodec.java",
             "common/com/statespec/backend/runtime/MetricSink.java"},
            {"common/com/statespec/backend/runtime/EntityGcDescriptors.java",
             "common/com/statespec/backend/runtime/EntityGcRepository.java",
             "common/com/statespec/backend/runtime/EntityGcWorkers.java"},
            {"worker/com/statespec/generated/WorkerQueues.java"},
            {"worker/com/statespec/generated/WorkerLeases.java"},
            {"worker/com/statespec/generated/WorkerWorkflows.java"},
            {"worker/com/statespec/generated/WorkflowRunner.java",
             "worker/com/statespec/generated/WorkflowStepHandlers.java"},
            {"api/com/statespec/generated/ApiApplication.java",
             "api/com/statespec/generated/ApiProcess.java",
             "api/com/statespec/generated/ApiDispatcher.java",
             "api/com/statespec/generated/ApiRoutes.java",
             "api/com/statespec/generated/ApiServer.java",
             "api/com/statespec/generated/ApiTransport.java",
             "api/com/statespec/generated/ApiMain.java"},
            {"worker/com/statespec/generated/WorkerApplication.java",
             "worker/com/statespec/generated/WorkerProcess.java",
             "worker/com/statespec/generated/WorkerRuntime.java",
             "worker/com/statespec/generated/WorkerMain.java"},
            "api/com/statespec/generated/ApiApplication.java",
            "api/com/statespec/generated/ApiProcess.java",
            "worker/com/statespec/generated/WorkerRuntime.java",
        },
        {
            statespec::BindingLanguage::Rust,
            "rust",
            "common/descriptors.rs",
            "common/lib.rs",
            {"common/runtime/codec.rs", "common/runtime/codec_core.rs"},
            {"common/runtime/codec_feature_flags.rs", "common/runtime/feature_flags.rs"},
            {"common/runtime/codec_queues.rs", "common/runtime/queues.rs"},
            {"common/runtime/codec_leases.rs", "common/runtime/leases.rs"},
            {"common/runtime/codec_workflows.rs", "common/runtime/workflows.rs"},
            {"common/runtime/codec_observability.rs", "common/runtime/codec_logs.rs",
             "common/runtime/logs.rs", "common/runtime/codec_metrics.rs",
             "common/runtime/metrics.rs"},
            {"common/runtime/entity_gc_descriptors.rs", "common/runtime/entity_gc_repository.rs",
             "common/runtime/entity_gc_workers.rs"},
            {"worker/worker_queues.rs"},
            {"worker/worker_leases.rs"},
            {"worker/worker_workflows.rs"},
            {"worker/workflow_runner.rs", "worker/workflow_step_handlers.rs"},
            {"api/api_application.rs", "api/api_process.rs", "api/api_dispatcher.rs",
             "api/api_routes.rs", "api/api_server.rs", "api/api_transport.rs", "api/main.rs"},
            {"worker/worker_application.rs", "worker/worker_process.rs", "worker/worker_runtime.rs",
             "worker/main.rs"},
            "api/api_application.rs",
            "api/api_process.rs",
            "worker/worker_runtime.rs",
        },
    }};
    return paths;
}

void require_descriptor_names(
    const statespec::GenerationResult& result,
    const LanguageRuntimePaths& paths,
    const std::vector<std::string>& present_names,
    const std::vector<std::string>& absent_names,
    const std::string& context
)
{
    const auto descriptors = descriptor_content(result, paths);
    for (const auto& name : present_names)
    {
        require_contains(descriptors, name, context + " descriptors");
    }
    for (const auto& name : absent_names)
    {
        require_not_contains(descriptors, name, context + " descriptors");
    }
}

void require_rust_manifest_matches_runtime_usage(
    const statespec::GenerationResult& result,
    const std::vector<std::string>& present_modules,
    const std::vector<std::string>& absent_modules,
    const std::string& context
)
{
    const auto lib = artifact_content(result, "common/lib.rs");
    for (const auto& module : present_modules)
    {
        require_contains(lib, "pub mod " + module + ";", context + " rust lib.rs");
    }
    for (const auto& module : absent_modules)
    {
        require_not_contains(lib, "pub mod " + module + ";", context + " rust lib.rs");
    }
}

void require_api_gc_config(
    const std::string& content,
    statespec::BindingLanguage language,
    const std::string& context
)
{
    switch (language)
    {
    case statespec::BindingLanguage::Cpp:
        require_contains(content, "bool entity_gc_enabled = true", context);
        require_contains(content, "if (!config_.entity_gc_enabled)", context);
        break;
    case statespec::BindingLanguage::Go:
        require_contains(content, "EntityGCEnabled", context);
        require_contains(content, "if !process.Config.EntityGCEnabled", context);
        break;
    case statespec::BindingLanguage::Java:
        require_contains(content, "boolean entityGcEnabled", context);
        require_contains(content, "if (!config.entityGcEnabled())", context);
        break;
    case statespec::BindingLanguage::Rust:
        require_contains(content, "pub entity_gc_enabled: bool", context);
        require_contains(content, "if !self.config.entity_gc_enabled", context);
        break;
    }
}

void require_worker_gc_config(
    const std::string& content,
    statespec::BindingLanguage language,
    const std::string& context
)
{
    switch (language)
    {
    case statespec::BindingLanguage::Cpp:
        require_contains(content, "struct WorkerRuntimeConfig", context);
        require_contains(content, "bool entity_gc_enabled = true", context);
        require_contains(content, "if (!config_.entity_gc_enabled)", context);
        break;
    case statespec::BindingLanguage::Go:
        require_contains(content, "type WorkerTierRuntimeConfig struct", context);
        require_contains(content, "EntityGCEnabled", context);
        require_contains(content, "if !runtime.Config.EntityGCEnabled", context);
        break;
    case statespec::BindingLanguage::Java:
        require_contains(content, "record Config(boolean entityGcEnabled", context);
        require_contains(content, "if (!config.entityGcEnabled())", context);
        break;
    case statespec::BindingLanguage::Rust:
        require_contains(content, "pub struct WorkerRuntimeConfig", context);
        require_contains(content, "pub entity_gc_enabled: bool", context);
        require_contains(content, "if !self.config.entity_gc_enabled", context);
        break;
    }
}

void runtime_pruning_covers_no_runtime_domains()
{
    for (const auto& paths : language_runtime_paths())
    {
        const auto result =
            generate_runtime_fixture(runtime_pruning_spec("NoRuntime"), paths, "none");
        require_artifacts_absent(result, all_runtime_paths(paths), paths.name + " no-runtime");
        require_artifacts_absent(result, paths.api_app_paths, paths.name + " no-runtime");
        require_artifacts_absent(result, paths.worker_app_paths, paths.name + " no-runtime");
        require_descriptor_names(
            result, paths, {},
            {"FeatureFlagOnly", "QueueOnly", "LeaseOnly", "WorkflowOnly", "LogOnly", "MetricOnly"},
            paths.name + " no-runtime"
        );
        if (paths.language == statespec::BindingLanguage::Rust)
        {
            require_rust_manifest_matches_runtime_usage(
                result, {},
                {"runtime_feature_flags", "runtime_queues", "runtime_leases", "runtime_workflows",
                 "runtime_logs", "runtime_metrics", "worker_queues", "worker_leases",
                 "worker_workflows", "workflow_runner", "workflow_step_handlers"},
                "no-runtime"
            );
        }
    }
}

void runtime_pruning_covers_only_workflows()
{
    for (const auto& paths : language_runtime_paths())
    {
        const auto result = generate_runtime_fixture(only_workflows_spec(), paths, "workflows");
        require_artifacts_present(
            result,
            concat(
                {paths.common_codec_paths, paths.workflow_paths, paths.worker_workflow_paths,
                 paths.worker_execution_paths}
            ),
            paths.name + " workflow-only"
        );
        require_artifacts_absent(
            result,
            concat(
                {paths.feature_flag_paths, paths.queue_paths, paths.lease_paths,
                 paths.observability_paths, paths.entity_gc_paths, paths.worker_queue_paths,
                 paths.worker_lease_paths, paths.api_app_paths, paths.worker_app_paths}
            ),
            paths.name + " workflow-only"
        );
        require_descriptor_names(
            result, paths, {"WorkflowOnly"},
            {"FeatureFlagOnly", "QueueOnly", "LeaseOnly", "LogOnly", "MetricOnly"},
            paths.name + " workflow-only"
        );
        if (paths.language == statespec::BindingLanguage::Rust)
        {
            require_rust_manifest_matches_runtime_usage(
                result, {"runtime_workflows", "worker_workflows", "workflow_runner"},
                {"runtime_feature_flags", "runtime_queues", "runtime_leases", "runtime_logs",
                 "runtime_metrics", "worker_queues", "worker_leases", "worker_application"},
                "workflow-only"
            );
        }
    }
}

void runtime_pruning_covers_only_queues()
{
    for (const auto& paths : language_runtime_paths())
    {
        const auto result = generate_runtime_fixture(only_queues_spec(), paths, "queues");
        require_artifacts_present(
            result, concat({paths.common_codec_paths, paths.queue_paths, paths.worker_queue_paths}),
            paths.name + " queue-only"
        );
        require_artifacts_absent(
            result,
            concat(
                {paths.feature_flag_paths, paths.lease_paths, paths.workflow_paths,
                 paths.observability_paths, paths.entity_gc_paths, paths.worker_lease_paths,
                 paths.worker_workflow_paths, paths.worker_execution_paths, paths.api_app_paths,
                 paths.worker_app_paths}
            ),
            paths.name + " queue-only"
        );
        require_descriptor_names(
            result, paths, {"QueueOnly"},
            {"FeatureFlagOnly", "LeaseOnly", "WorkflowOnly", "LogOnly", "MetricOnly"},
            paths.name + " queue-only"
        );
        if (paths.language == statespec::BindingLanguage::Rust)
        {
            require_rust_manifest_matches_runtime_usage(
                result, {"runtime_queues", "worker_queues"},
                {"runtime_feature_flags", "runtime_leases", "runtime_workflows", "runtime_logs",
                 "runtime_metrics", "worker_leases", "worker_workflows", "workflow_runner"},
                "queue-only"
            );
        }
    }
}

void runtime_pruning_covers_only_feature_flags()
{
    for (const auto& paths : language_runtime_paths())
    {
        const auto result =
            generate_runtime_fixture(only_feature_flags_spec(), paths, "feature-flags");
        require_artifacts_present(
            result, concat({paths.common_codec_paths, paths.feature_flag_paths}),
            paths.name + " feature-flag-only"
        );
        require_artifacts_absent(
            result,
            concat(
                {paths.queue_paths, paths.lease_paths, paths.workflow_paths,
                 paths.observability_paths, paths.entity_gc_paths, paths.worker_queue_paths,
                 paths.worker_lease_paths, paths.worker_workflow_paths,
                 paths.worker_execution_paths, paths.api_app_paths, paths.worker_app_paths}
            ),
            paths.name + " feature-flag-only"
        );
        require_descriptor_names(
            result, paths, {"FeatureFlagOnly"},
            {"QueueOnly", "LeaseOnly", "WorkflowOnly", "LogOnly", "MetricOnly"},
            paths.name + " feature-flag-only"
        );
        if (paths.language == statespec::BindingLanguage::Rust)
        {
            require_rust_manifest_matches_runtime_usage(
                result, {"runtime_feature_flags"},
                {"runtime_queues", "runtime_leases", "runtime_workflows", "runtime_logs",
                 "runtime_metrics", "worker_queues", "worker_leases", "worker_workflows"},
                "feature-flag-only"
            );
        }
    }
}

void runtime_pruning_covers_only_observability()
{
    for (const auto& paths : language_runtime_paths())
    {
        const auto result =
            generate_runtime_fixture(only_observability_spec(), paths, "observability");
        require_artifacts_present(
            result, concat({paths.common_codec_paths, paths.observability_paths}),
            paths.name + " observability-only"
        );
        require_artifacts_absent(
            result,
            concat(
                {paths.feature_flag_paths, paths.queue_paths, paths.lease_paths,
                 paths.workflow_paths, paths.entity_gc_paths, paths.worker_queue_paths,
                 paths.worker_lease_paths, paths.worker_workflow_paths,
                 paths.worker_execution_paths, paths.api_app_paths, paths.worker_app_paths}
            ),
            paths.name + " observability-only"
        );
        require_descriptor_names(
            result, paths, {"LogOnly", "MetricOnly"},
            {"FeatureFlagOnly", "QueueOnly", "LeaseOnly", "WorkflowOnly"},
            paths.name + " observability-only"
        );
        if (paths.language == statespec::BindingLanguage::Rust)
        {
            require_rust_manifest_matches_runtime_usage(
                result, {"runtime_logs", "runtime_metrics"},
                {"runtime_feature_flags", "runtime_queues", "runtime_leases", "runtime_workflows",
                 "worker_queues", "worker_leases", "worker_workflows"},
                "observability-only"
            );
        }
    }
}

void runtime_pruning_covers_mixed_api_worker_app()
{
    for (const auto& paths : language_runtime_paths())
    {
        const auto result = generate_runtime_fixture(mixed_api_worker_spec(), paths, "mixed");
        require_artifacts_present(
            result,
            concat(
                {paths.common_codec_paths, paths.queue_paths, paths.lease_paths,
                 paths.workflow_paths, paths.worker_queue_paths, paths.worker_lease_paths,
                 paths.worker_workflow_paths, paths.worker_execution_paths, paths.api_app_paths,
                 paths.worker_app_paths}
            ),
            paths.name + " mixed"
        );
        require_artifacts_absent(
            result,
            concat({paths.feature_flag_paths, paths.observability_paths, paths.entity_gc_paths}),
            paths.name + " mixed"
        );
        require_descriptor_names(
            result, paths, {"QueueOnly", "LeaseOnly", "WorkflowOnly"},
            {"FeatureFlagOnly", "LogOnly", "MetricOnly"}, paths.name + " mixed"
        );

        const auto api_runtime = artifact_content(result, paths.api_runtime_path);
        const auto worker_runtime = artifact_content(result, paths.worker_runtime_path);
        for (const auto& content : {api_runtime, worker_runtime})
        {
            require_not_contains(content, "FeatureFlagStore", paths.name + " mixed runtime");
            require_not_contains(content, "feature_flags", paths.name + " mixed runtime");
            require_not_contains(content, "featureFlags", paths.name + " mixed runtime");
            require_not_contains(content, "LogSink", paths.name + " mixed runtime");
            require_not_contains(content, "MetricSink", paths.name + " mixed runtime");
            require_not_contains(content, "logs", paths.name + " mixed runtime");
            require_not_contains(content, "metrics", paths.name + " mixed runtime");
        }
        if (paths.language == statespec::BindingLanguage::Rust)
        {
            require_rust_manifest_matches_runtime_usage(
                result,
                {"runtime_queues", "runtime_leases", "runtime_workflows", "worker_queues",
                 "worker_leases", "worker_workflows", "worker_application", "workflow_runner"},
                {"runtime_feature_flags", "runtime_logs", "runtime_metrics"}, "mixed"
            );
        }
    }
}

void gc_deployment_covers_api_only_app()
{
    for (const auto& paths : language_runtime_paths())
    {
        const auto result =
            generate_runtime_fixture(api_only_gc_deployment_spec(), paths, "api-gc");
        require_artifacts_present(
            result, concat({paths.entity_gc_paths, paths.api_app_paths}), paths.name + " api-gc"
        );
        require_artifacts_absent(result, paths.worker_app_paths, paths.name + " api-gc");
        require_descriptor_names(
            result, paths, {"GcTask", "TouchGcTask", "GcApi"}, {}, paths.name + " api-gc"
        );
        require_api_gc_config(
            artifact_content(result, paths.api_process_path), paths.language, paths.name + " api-gc"
        );
        if (paths.language == statespec::BindingLanguage::Rust)
        {
            require_rust_manifest_matches_runtime_usage(
                result,
                {"runtime_entity_gc_descriptors", "runtime_entity_gc_repository",
                 "runtime_entity_gc_workers"},
                {"worker_runtime"}, "api-gc"
            );
        }
    }
}

void gc_deployment_covers_worker_only_app()
{
    for (const auto& paths : language_runtime_paths())
    {
        const auto result =
            generate_runtime_fixture(worker_only_gc_deployment_spec(), paths, "worker-gc");
        require_artifacts_present(
            result,
            concat(
                {paths.entity_gc_paths, paths.queue_paths, paths.lease_paths, paths.workflow_paths,
                 paths.worker_queue_paths, paths.worker_lease_paths, paths.worker_workflow_paths,
                 paths.worker_execution_paths, paths.worker_app_paths}
            ),
            paths.name + " worker-gc"
        );
        require_artifacts_absent(result, paths.api_app_paths, paths.name + " worker-gc");
        require_descriptor_names(
            result, paths, {"GcTask", "QueueOnly", "LeaseOnly", "WorkflowOnly", "RuntimeWorker"},
            {"TouchGcTask", "GcApi"}, paths.name + " worker-gc"
        );
        require_worker_gc_config(
            artifact_content(result, paths.worker_runtime_path), paths.language,
            paths.name + " worker-gc"
        );
        if (paths.language == statespec::BindingLanguage::Rust)
        {
            require_rust_manifest_matches_runtime_usage(
                result,
                {"runtime_entity_gc_descriptors", "runtime_entity_gc_repository",
                 "runtime_entity_gc_workers", "worker_runtime"},
                {}, "worker-gc"
            );
        }
    }
}

void gc_deployment_covers_mixed_app()
{
    for (const auto& paths : language_runtime_paths())
    {
        const auto result = generate_runtime_fixture(mixed_gc_deployment_spec(), paths, "mixed-gc");
        require_artifacts_present(
            result,
            concat(
                {paths.entity_gc_paths, paths.queue_paths, paths.lease_paths, paths.workflow_paths,
                 paths.worker_queue_paths, paths.worker_lease_paths, paths.worker_workflow_paths,
                 paths.worker_execution_paths, paths.api_app_paths, paths.worker_app_paths}
            ),
            paths.name + " mixed-gc"
        );
        require_descriptor_names(
            result, paths,
            {"GcTask", "TouchGcTask", "GcApi", "QueueOnly", "LeaseOnly", "WorkflowOnly",
             "RuntimeWorker"},
            {}, paths.name + " mixed-gc"
        );
        require_api_gc_config(
            artifact_content(result, paths.api_process_path), paths.language,
            paths.name + " mixed-gc"
        );
        require_worker_gc_config(
            artifact_content(result, paths.worker_runtime_path), paths.language,
            paths.name + " mixed-gc"
        );
        if (paths.language == statespec::BindingLanguage::Rust)
        {
            require_rust_manifest_matches_runtime_usage(
                result,
                {"runtime_entity_gc_descriptors", "runtime_entity_gc_repository",
                 "runtime_entity_gc_workers", "worker_runtime"},
                {}, "mixed-gc"
            );
        }
    }
}

} // namespace

TEST_CASE("binding runtime pruning covers no runtime domains")
{
    runtime_pruning_covers_no_runtime_domains();
}

TEST_CASE("binding runtime pruning covers only workflows")
{
    runtime_pruning_covers_only_workflows();
}

TEST_CASE("binding runtime pruning covers only queues")
{
    runtime_pruning_covers_only_queues();
}

TEST_CASE("binding runtime pruning covers only feature flags")
{
    runtime_pruning_covers_only_feature_flags();
}

TEST_CASE("binding runtime pruning covers only observability")
{
    runtime_pruning_covers_only_observability();
}

TEST_CASE("binding runtime pruning covers mixed API and worker app")
{
    runtime_pruning_covers_mixed_api_worker_app();
}

TEST_CASE("binding GC deployment covers API-only app")
{
    gc_deployment_covers_api_only_app();
}

TEST_CASE("binding GC deployment covers worker-only app")
{
    gc_deployment_covers_worker_only_app();
}

TEST_CASE("binding GC deployment covers mixed app")
{
    gc_deployment_covers_mixed_app();
}
