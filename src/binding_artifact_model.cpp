#include "statespec/generator_bindings.hpp"

namespace statespec
{

std::string binding_app_artifact_kind_name(BindingAppArtifactKind kind)
{
    switch (kind)
    {
    case BindingAppArtifactKind::MemoryBackend:
        return "memory_backend";
    case BindingAppArtifactKind::MemoryTransaction:
        return "memory_transaction";
    case BindingAppArtifactKind::RuntimeCodec:
        return "runtime_codec";
    case BindingAppArtifactKind::RuntimeFeatureFlagStore:
        return "runtime_feature_flag_store";
    case BindingAppArtifactKind::RuntimeQueueStore:
        return "runtime_queue_store";
    case BindingAppArtifactKind::RuntimeLeaseStore:
        return "runtime_lease_store";
    case BindingAppArtifactKind::RuntimeWorkflowStore:
        return "runtime_workflow_store";
    case BindingAppArtifactKind::RuntimeLogSink:
        return "runtime_log_sink";
    case BindingAppArtifactKind::RuntimeMetricSink:
        return "runtime_metric_sink";
    case BindingAppArtifactKind::ApiApplication:
        return "api_application";
    case BindingAppArtifactKind::ApiProcess:
        return "api_process";
    case BindingAppArtifactKind::ApiServer:
        return "api_server";
    case BindingAppArtifactKind::ApiDispatcher:
        return "api_dispatcher";
    case BindingAppArtifactKind::ApiHandlerRegistry:
        return "api_handler_registry";
    case BindingAppArtifactKind::ApiMain:
        return "api_main";
    case BindingAppArtifactKind::WorkerApplication:
        return "worker_application";
    case BindingAppArtifactKind::WorkerRuntime:
        return "worker_runtime";
    case BindingAppArtifactKind::WorkerRegistry:
        return "worker_registry";
    case BindingAppArtifactKind::WorkflowRunner:
        return "workflow_runner";
    case BindingAppArtifactKind::WorkflowStepHandlers:
        return "workflow_step_handlers";
    case BindingAppArtifactKind::WorkerMain:
        return "worker_main";
    }
    return "api_application";
}

std::vector<BindingAppArtifactModel> binding_app_artifact_model(BindingLanguage language)
{
    using Kind = BindingAppArtifactKind;
    const auto common = GeneratedArtifactTier::Common;
    const auto api = GeneratedArtifactTier::Api;
    const auto worker = GeneratedArtifactTier::Worker;

    switch (language)
    {
    case BindingLanguage::Cpp:
        return {
            {"common/memory/backend.hpp", common, Kind::MemoryBackend,
             "In-memory backend composition root for local API and worker linking"},
            {"common/memory/transaction.hpp", common, Kind::MemoryTransaction,
             "In-memory optimistic-concurrency transaction"},
            {"common/runtime/codec.hpp", common, Kind::RuntimeCodec, "Runtime record JSON codec"},
            {"common/runtime/codec_core.hpp", common, Kind::RuntimeCodec,
             "Runtime shared codec helpers"},
            {"common/runtime/codec_feature_flags.hpp", common, Kind::RuntimeCodec,
             "Runtime feature flag JSON codec"},
            {"common/runtime/codec_queues.hpp", common, Kind::RuntimeCodec,
             "Runtime queue JSON codec"},
            {"common/runtime/codec_leases.hpp", common, Kind::RuntimeCodec,
             "Runtime lease JSON codec"},
            {"common/runtime/codec_workflows.hpp", common, Kind::RuntimeCodec,
             "Runtime workflow JSON codec"},
            {"common/runtime/codec_observability.hpp", common, Kind::RuntimeCodec,
             "Runtime log and metric JSON codec"},
            {"common/runtime/codec_logs.hpp", common, Kind::RuntimeCodec, "Runtime log JSON codec"},
            {"common/runtime/codec_metrics.hpp", common, Kind::RuntimeCodec,
             "Runtime metric JSON codec"},
            {"common/runtime/feature_flag_store.hpp", common, Kind::RuntimeFeatureFlagStore,
             "Backend-neutral feature flag store"},
            {"common/runtime/queue_store.hpp", common, Kind::RuntimeQueueStore,
             "Backend-neutral queue store"},
            {"common/runtime/lease_store.hpp", common, Kind::RuntimeLeaseStore,
             "Backend-neutral lease store"},
            {"common/runtime/workflow_store.hpp", common, Kind::RuntimeWorkflowStore,
             "Backend-neutral workflow store"},
            {"common/runtime/log_sink.hpp", common, Kind::RuntimeLogSink,
             "Backend-neutral log sink"},
            {"common/runtime/metric_sink.hpp", common, Kind::RuntimeMetricSink,
             "Backend-neutral metric sink"},
            {"api/api_application.hpp", api, Kind::ApiApplication,
             "API application composition root"},
            {"api/api_process.hpp", api, Kind::ApiProcess, "API process lifecycle runtime", true},
            {"api/api_server.hpp", api, Kind::ApiServer, "API server lifecycle and request loop",
             true},
            {"api/api_dispatcher.hpp", api, Kind::ApiDispatcher, "API route-to-handler dispatcher",
             true},
            {"api/api_handler_registry.hpp", api, Kind::ApiHandlerRegistry,
             "API handler registry extension point"},
            {"api/main.cpp", api, Kind::ApiMain, "API process entrypoint"},
            {"worker/worker_application.hpp", worker, Kind::WorkerApplication,
             "Worker application composition root", true},
            {"worker/worker_runtime.hpp", worker, Kind::WorkerRuntime,
             "Worker polling and execution runtime"},
            {"worker/worker_registry.hpp", worker, Kind::WorkerRegistry,
             "Worker declaration registry", true},
            {"worker/workflow_runner.hpp", worker, Kind::WorkflowRunner,
             "Workflow claim and step runner", true},
            {"worker/workflow_step_handlers.hpp", worker, Kind::WorkflowStepHandlers,
             "Workflow step handler extension point", true},
            {"worker/main.cpp", worker, Kind::WorkerMain, "Worker process entrypoint"},
        };
    case BindingLanguage::Go:
        return {
            {"common/backend/memory/backend.go", common, Kind::MemoryBackend,
             "In-memory backend composition root for local API and worker linking"},
            {"common/backend/memory/transaction.go", common, Kind::MemoryTransaction,
             "In-memory optimistic-concurrency transaction"},
            {"common/backend/runtime/codec.go", common, Kind::RuntimeCodec,
             "Runtime record JSON codec"},
            {"common/backend/runtime/codec_feature_flags.go", common, Kind::RuntimeCodec,
             "Runtime feature flag JSON codec"},
            {"common/backend/runtime/codec_queues.go", common, Kind::RuntimeCodec,
             "Runtime queue JSON codec"},
            {"common/backend/runtime/codec_leases.go", common, Kind::RuntimeCodec,
             "Runtime lease JSON codec"},
            {"common/backend/runtime/codec_workflows.go", common, Kind::RuntimeCodec,
             "Runtime workflow JSON codec"},
            {"common/backend/runtime/codec_observability.go", common, Kind::RuntimeCodec,
             "Runtime log and metric JSON codec"},
            {"common/backend/runtime/codec_logs.go", common, Kind::RuntimeCodec,
             "Runtime log JSON codec"},
            {"common/backend/runtime/codec_metrics.go", common, Kind::RuntimeCodec,
             "Runtime metric JSON codec"},
            {"common/backend/runtime/feature_flags.go", common, Kind::RuntimeFeatureFlagStore,
             "Backend-neutral feature flag store"},
            {"common/backend/runtime/queues.go", common, Kind::RuntimeQueueStore,
             "Backend-neutral queue store"},
            {"common/backend/runtime/leases.go", common, Kind::RuntimeLeaseStore,
             "Backend-neutral lease store"},
            {"common/backend/runtime/workflows.go", common, Kind::RuntimeWorkflowStore,
             "Backend-neutral workflow store"},
            {"common/backend/runtime/logs.go", common, Kind::RuntimeLogSink,
             "Backend-neutral log sink"},
            {"common/backend/runtime/metrics.go", common, Kind::RuntimeMetricSink,
             "Backend-neutral metric sink"},
            {"api/backend/api_application.go", api, Kind::ApiApplication,
             "API application composition root"},
            {"api/backend/api_process.go", api, Kind::ApiProcess, "API process lifecycle runtime",
             true},
            {"api/backend/api_server.go", api, Kind::ApiServer,
             "API server lifecycle and request loop", true},
            {"api/backend/api_dispatcher.go", api, Kind::ApiDispatcher,
             "API route-to-handler dispatcher", true},
            {"api/backend/api_handler_registry.go", api, Kind::ApiHandlerRegistry,
             "API handler registry extension point"},
            {"api/cmd/api/main.go", api, Kind::ApiMain, "API process entrypoint"},
            {"worker/backend/worker_application.go", worker, Kind::WorkerApplication,
             "Worker application composition root", true},
            {"worker/backend/worker_runtime.go", worker, Kind::WorkerRuntime,
             "Worker polling and execution runtime"},
            {"worker/backend/worker_registry.go", worker, Kind::WorkerRegistry,
             "Worker declaration registry", true},
            {"worker/backend/workflow_runner.go", worker, Kind::WorkflowRunner,
             "Workflow claim and step runner", true},
            {"worker/backend/workflow_step_handlers.go", worker, Kind::WorkflowStepHandlers,
             "Workflow step handler extension point", true},
            {"worker/cmd/worker/main.go", worker, Kind::WorkerMain, "Worker process entrypoint"},
        };
    case BindingLanguage::Java:
        return {
            {"common/com/statespec/backend/memory/InMemoryBackend.java", common,
             Kind::MemoryBackend,
             "In-memory backend composition root for local API and worker linking"},
            {"common/com/statespec/backend/memory/InMemoryTransaction.java", common,
             Kind::MemoryTransaction, "In-memory optimistic-concurrency transaction"},
            {"common/com/statespec/backend/runtime/Codec.java", common, Kind::RuntimeCodec,
             "Runtime record JSON codec"},
            {"common/com/statespec/backend/runtime/FeatureFlagCodec.java", common,
             Kind::RuntimeCodec, "Runtime feature flag JSON codec"},
            {"common/com/statespec/backend/runtime/QueueCodec.java", common, Kind::RuntimeCodec,
             "Runtime queue JSON codec"},
            {"common/com/statespec/backend/runtime/LeaseCodec.java", common, Kind::RuntimeCodec,
             "Runtime lease JSON codec"},
            {"common/com/statespec/backend/runtime/WorkflowCodec.java", common, Kind::RuntimeCodec,
             "Runtime workflow JSON codec"},
            {"common/com/statespec/backend/runtime/ObservabilityCodec.java", common,
             Kind::RuntimeCodec, "Runtime log and metric JSON codec"},
            {"common/com/statespec/backend/runtime/LogCodec.java", common, Kind::RuntimeCodec,
             "Runtime log JSON codec"},
            {"common/com/statespec/backend/runtime/MetricCodec.java", common, Kind::RuntimeCodec,
             "Runtime metric JSON codec"},
            {"common/com/statespec/backend/runtime/FeatureFlagStore.java", common,
             Kind::RuntimeFeatureFlagStore, "Backend-neutral feature flag store"},
            {"common/com/statespec/backend/runtime/QueueStore.java", common,
             Kind::RuntimeQueueStore, "Backend-neutral queue store"},
            {"common/com/statespec/backend/runtime/LeaseStore.java", common,
             Kind::RuntimeLeaseStore, "Backend-neutral lease store"},
            {"common/com/statespec/backend/runtime/WorkflowStore.java", common,
             Kind::RuntimeWorkflowStore, "Backend-neutral workflow store"},
            {"common/com/statespec/backend/runtime/LogSink.java", common, Kind::RuntimeLogSink,
             "Backend-neutral log sink"},
            {"common/com/statespec/backend/runtime/MetricSink.java", common,
             Kind::RuntimeMetricSink, "Backend-neutral metric sink"},
            {"api/com/statespec/generated/ApiApplication.java", api, Kind::ApiApplication,
             "API application composition root"},
            {"api/com/statespec/generated/ApiProcess.java", api, Kind::ApiProcess,
             "API process lifecycle runtime", true},
            {"api/com/statespec/generated/ApiServer.java", api, Kind::ApiServer,
             "API server lifecycle and request loop", true},
            {"api/com/statespec/generated/ApiDispatcher.java", api, Kind::ApiDispatcher,
             "API route-to-handler dispatcher", true},
            {"api/com/statespec/generated/ApiHandlerRegistry.java", api, Kind::ApiHandlerRegistry,
             "API handler registry extension point"},
            {"api/com/statespec/generated/ApiMain.java", api, Kind::ApiMain,
             "API process entrypoint"},
            {"worker/com/statespec/generated/WorkerApplication.java", worker,
             Kind::WorkerApplication, "Worker application composition root", true},
            {"worker/com/statespec/generated/WorkerRuntime.java", worker, Kind::WorkerRuntime,
             "Worker polling and execution runtime"},
            {"worker/com/statespec/generated/WorkerRegistry.java", worker, Kind::WorkerRegistry,
             "Worker declaration registry", true},
            {"worker/com/statespec/generated/WorkflowRunner.java", worker, Kind::WorkflowRunner,
             "Workflow claim and step runner", true},
            {"worker/com/statespec/generated/WorkflowStepHandlers.java", worker,
             Kind::WorkflowStepHandlers, "Workflow step handler extension point", true},
            {"worker/com/statespec/generated/WorkerMain.java", worker, Kind::WorkerMain,
             "Worker process entrypoint"},
        };
    case BindingLanguage::Rust:
        return {
            {"common/memory/backend.rs", common, Kind::MemoryBackend,
             "In-memory backend composition root for local API and worker linking"},
            {"common/memory/transaction.rs", common, Kind::MemoryTransaction,
             "In-memory optimistic-concurrency transaction"},
            {"common/runtime/codec.rs", common, Kind::RuntimeCodec, "Runtime record JSON codec"},
            {"common/runtime/codec_core.rs", common, Kind::RuntimeCodec,
             "Runtime shared codec helpers"},
            {"common/runtime/codec_feature_flags.rs", common, Kind::RuntimeCodec,
             "Runtime feature flag JSON codec"},
            {"common/runtime/codec_queues.rs", common, Kind::RuntimeCodec,
             "Runtime queue JSON codec"},
            {"common/runtime/codec_leases.rs", common, Kind::RuntimeCodec,
             "Runtime lease JSON codec"},
            {"common/runtime/codec_workflows.rs", common, Kind::RuntimeCodec,
             "Runtime workflow JSON codec"},
            {"common/runtime/codec_observability.rs", common, Kind::RuntimeCodec,
             "Runtime log and metric JSON codec"},
            {"common/runtime/codec_logs.rs", common, Kind::RuntimeCodec, "Runtime log JSON codec"},
            {"common/runtime/codec_metrics.rs", common, Kind::RuntimeCodec,
             "Runtime metric JSON codec"},
            {"common/runtime/feature_flags.rs", common, Kind::RuntimeFeatureFlagStore,
             "Backend-neutral feature flag store"},
            {"common/runtime/queues.rs", common, Kind::RuntimeQueueStore,
             "Backend-neutral queue store"},
            {"common/runtime/leases.rs", common, Kind::RuntimeLeaseStore,
             "Backend-neutral lease store"},
            {"common/runtime/workflows.rs", common, Kind::RuntimeWorkflowStore,
             "Backend-neutral workflow store"},
            {"common/runtime/logs.rs", common, Kind::RuntimeLogSink, "Backend-neutral log sink"},
            {"common/runtime/metrics.rs", common, Kind::RuntimeMetricSink,
             "Backend-neutral metric sink"},
            {"api/api_application.rs", api, Kind::ApiApplication,
             "API application composition root"},
            {"api/api_process.rs", api, Kind::ApiProcess, "API process lifecycle runtime", true},
            {"api/api_server.rs", api, Kind::ApiServer, "API server lifecycle and request loop",
             true},
            {"api/api_dispatcher.rs", api, Kind::ApiDispatcher, "API route-to-handler dispatcher",
             true},
            {"api/api_handler_registry.rs", api, Kind::ApiHandlerRegistry,
             "API handler registry extension point"},
            {"api/main.rs", api, Kind::ApiMain, "API process entrypoint"},
            {"worker/worker_application.rs", worker, Kind::WorkerApplication,
             "Worker application composition root", true},
            {"worker/worker_runtime.rs", worker, Kind::WorkerRuntime,
             "Worker polling and execution runtime"},
            {"worker/worker_registry.rs", worker, Kind::WorkerRegistry,
             "Worker declaration registry", true},
            {"worker/workflow_runner.rs", worker, Kind::WorkflowRunner,
             "Workflow claim and step runner", true},
            {"worker/workflow_step_handlers.rs", worker, Kind::WorkflowStepHandlers,
             "Workflow step handler extension point", true},
            {"worker/main.rs", worker, Kind::WorkerMain, "Worker process entrypoint"},
        };
    }
    return {};
}

} // namespace statespec
