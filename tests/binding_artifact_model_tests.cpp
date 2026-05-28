#include "binding_test_support.hpp"

namespace
{

void test_binding_template_root_resolution()
{
    require_string_equal(
        statespec::default_binding_template_root(statespec::BindingLanguage::Cpp).generic_string(),
        "bindings/cpp", "default C++ template root"
    );
    require_string_equal(
        statespec::default_binding_template_root(statespec::BindingLanguage::Go).generic_string(),
        "bindings/go", "default Go template root"
    );
    require_string_equal(
        statespec::default_binding_template_root(statespec::BindingLanguage::Java).generic_string(),
        "bindings/java", "default Java template root"
    );
    require_string_equal(
        statespec::default_binding_template_root(statespec::BindingLanguage::Rust).generic_string(),
        "bindings/rust", "default Rust template root"
    );

    const statespec::BindingGeneratorOptions options{
        statespec::BindingLanguage::Go,
        std::filesystem::path{"/tmp/statespec-template-root-test"},
        statespec::BindingGenerationTier::All,
        std::filesystem::path{"templates/bindings"},
    };
    require_string_equal(
        statespec::resolve_binding_template_root(options).generic_string(), "templates/bindings/go",
        "explicit binding template root should be language-scoped"
    );
}

void test_binding_app_artifact_kind_names()
{
    require_string_equal(
        statespec::binding_app_artifact_kind_name(statespec::BindingAppArtifactKind::MemoryBackend),
        "memory_backend", "memory backend artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(
            statespec::BindingAppArtifactKind::MemoryTransaction
        ),
        "memory_transaction", "memory transaction artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(statespec::BindingAppArtifactKind::RuntimeCodec),
        "runtime_codec", "runtime codec artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(
            statespec::BindingAppArtifactKind::RuntimeEntityGcDescriptors
        ),
        "runtime_entity_gc_descriptors", "runtime entity GC descriptor artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(
            statespec::BindingAppArtifactKind::RuntimeEntityGcDescriptorModule
        ),
        "runtime_entity_gc_descriptor_module",
        "runtime entity GC descriptor module artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(
            statespec::BindingAppArtifactKind::RuntimeEntityGcRepository
        ),
        "runtime_entity_gc_repository", "runtime entity GC repository artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(
            statespec::BindingAppArtifactKind::RuntimeEntityGcWorkers
        ),
        "runtime_entity_gc_workers", "runtime entity GC worker artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(
            statespec::BindingAppArtifactKind::RuntimeEntityGcRegistration
        ),
        "runtime_entity_gc_registration", "runtime entity GC registration artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(
            statespec::BindingAppArtifactKind::ApiApplication
        ),
        "api_application", "API application artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(statespec::BindingAppArtifactKind::ApiProcess),
        "api_process", "API process artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(statespec::BindingAppArtifactKind::ApiServer),
        "api_server", "API server artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(statespec::BindingAppArtifactKind::ApiTransport),
        "api_transport", "API transport artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(statespec::BindingAppArtifactKind::ApiCodecs),
        "api_codecs", "API codecs artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(
            statespec::BindingAppArtifactKind::WorkerApplication
        ),
        "worker_application", "worker application artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(statespec::BindingAppArtifactKind::WorkerProcess),
        "worker_process", "worker process artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(statespec::BindingAppArtifactKind::WorkerMain),
        "worker_main", "worker main artifact kind name"
    );
}

void test_binding_app_artifact_models_define_application_filenames()
{
    const auto common = statespec::GeneratedArtifactTier::Common;
    const auto api = statespec::GeneratedArtifactTier::Api;
    const auto worker = statespec::GeneratedArtifactTier::Worker;
    using Kind = statespec::BindingAppArtifactKind;

    require_exact_app_artifact_model(
        statespec::BindingLanguage::Cpp, "cpp",
        {
            {"common/memory/backend.hpp", common, Kind::MemoryBackend},
            {"common/memory/transaction.hpp", common, Kind::MemoryTransaction},
            {"common/runtime/codec.hpp", common, Kind::RuntimeCodec},
            {"common/runtime/codec_core.hpp", common, Kind::RuntimeCodec},
            {"common/runtime/codec_feature_flags.hpp", common, Kind::RuntimeCodec},
            {"common/runtime/codec_queues.hpp", common, Kind::RuntimeCodec},
            {"common/runtime/codec_leases.hpp", common, Kind::RuntimeCodec},
            {"common/runtime/codec_workflows.hpp", common, Kind::RuntimeCodec},
            {"common/runtime/codec_observability.hpp", common, Kind::RuntimeCodec},
            {"common/runtime/codec_logs.hpp", common, Kind::RuntimeCodec},
            {"common/runtime/codec_metrics.hpp", common, Kind::RuntimeCodec},
            {"common/runtime/feature_flag_store.hpp", common, Kind::RuntimeFeatureFlagStore},
            {"common/runtime/queue_store.hpp", common, Kind::RuntimeQueueStore},
            {"common/runtime/lease_store.hpp", common, Kind::RuntimeLeaseStore},
            {"common/runtime/workflow_store.hpp", common, Kind::RuntimeWorkflowStore},
            {"common/runtime/log_sink.hpp", common, Kind::RuntimeLogSink},
            {"common/runtime/metric_sink.hpp", common, Kind::RuntimeMetricSink},
            {"common/runtime/entity_gc_descriptors.hpp", common, Kind::RuntimeEntityGcDescriptors},
            {"common/runtime/entity_gc_repository.hpp", common, Kind::RuntimeEntityGcRepository},
            {"common/runtime/entity_gc_workers.hpp", common, Kind::RuntimeEntityGcWorkers},
            {"common/runtime/entity_gc_registration.hpp", common,
             Kind::RuntimeEntityGcRegistration},
            {"api/api_application.hpp", api, Kind::ApiApplication},
            {"api/api_process.hpp", api, Kind::ApiProcess, true},
            {"api/api_server.hpp", api, Kind::ApiServer, true},
            {"api/api_transport.hpp", api, Kind::ApiTransport, true},
            {"api/api_dispatcher.hpp", api, Kind::ApiDispatcher, true},
            {"api/api_codec_support.hpp", api, Kind::ApiCodecs},
            {"api/api_handler_registry.hpp", api, Kind::ApiHandlerRegistry},
            {"api/api_handler_registry_support.hpp", api, Kind::ApiHandlerRegistry},
            {"api/main.cpp", api, Kind::ApiMain},
            {"worker/worker_application.hpp", worker, Kind::WorkerApplication, true},
            {"worker/worker_process.hpp", worker, Kind::WorkerProcess, true},
            {"worker/worker_runtime.hpp", worker, Kind::WorkerRuntime},
            {"worker/worker_registry.hpp", worker, Kind::WorkerRegistry, true},
            {"worker/workflow_runner.hpp", worker, Kind::WorkflowRunner, true},
            {"worker/workflow_step_handlers.hpp", worker, Kind::WorkflowStepHandlers, true},
            {"worker/main.cpp", worker, Kind::WorkerMain},
        }
    );

    require_exact_app_artifact_model(
        statespec::BindingLanguage::Go, "go",
        {
            {"common/backend/memory/backend.go", common, Kind::MemoryBackend},
            {"common/backend/memory/transaction.go", common, Kind::MemoryTransaction},
            {"common/backend/runtime/codec.go", common, Kind::RuntimeCodec},
            {"common/backend/runtime/codec_feature_flags.go", common, Kind::RuntimeCodec},
            {"common/backend/runtime/codec_queues.go", common, Kind::RuntimeCodec},
            {"common/backend/runtime/codec_leases.go", common, Kind::RuntimeCodec},
            {"common/backend/runtime/codec_workflows.go", common, Kind::RuntimeCodec},
            {"common/backend/runtime/codec_observability.go", common, Kind::RuntimeCodec},
            {"common/backend/runtime/codec_logs.go", common, Kind::RuntimeCodec},
            {"common/backend/runtime/codec_metrics.go", common, Kind::RuntimeCodec},
            {"common/backend/runtime/feature_flags.go", common, Kind::RuntimeFeatureFlagStore},
            {"common/backend/runtime/queues.go", common, Kind::RuntimeQueueStore},
            {"common/backend/runtime/leases.go", common, Kind::RuntimeLeaseStore},
            {"common/backend/runtime/workflows.go", common, Kind::RuntimeWorkflowStore},
            {"common/backend/runtime/logs.go", common, Kind::RuntimeLogSink},
            {"common/backend/runtime/metrics.go", common, Kind::RuntimeMetricSink},
            {"common/backend/runtime/entity_gc_descriptors.go", common,
             Kind::RuntimeEntityGcDescriptors},
            {"common/backend/runtime/entity_gc_repository.go", common,
             Kind::RuntimeEntityGcRepository},
            {"common/backend/runtime/entity_gc_workers.go", common, Kind::RuntimeEntityGcWorkers},
            {"common/backend/runtime/entity_gc_registration.go", common,
             Kind::RuntimeEntityGcRegistration},
            {"api/backend/api_application.go", api, Kind::ApiApplication},
            {"api/backend/api_process.go", api, Kind::ApiProcess, true},
            {"api/backend/api_server.go", api, Kind::ApiServer, true},
            {"api/backend/api_transport.go", api, Kind::ApiTransport, true},
            {"api/backend/api_dispatcher.go", api, Kind::ApiDispatcher, true},
            {"api/backend/api_handler_registry.go", api, Kind::ApiHandlerRegistry},
            {"api/cmd/api/main.go", api, Kind::ApiMain},
            {"worker/backend/worker_application.go", worker, Kind::WorkerApplication, true},
            {"worker/backend/worker_process.go", worker, Kind::WorkerProcess, true},
            {"worker/backend/worker_runtime.go", worker, Kind::WorkerRuntime},
            {"worker/backend/worker_registry.go", worker, Kind::WorkerRegistry, true},
            {"worker/backend/workflow_runner.go", worker, Kind::WorkflowRunner, true},
            {"worker/backend/workflow_step_handlers.go", worker, Kind::WorkflowStepHandlers, true},
            {"worker/cmd/worker/main.go", worker, Kind::WorkerMain},
        }
    );

    require_exact_app_artifact_model(
        statespec::BindingLanguage::Java, "java",
        {
            {"common/com/statespec/backend/memory/InMemoryBackend.java", common,
             Kind::MemoryBackend},
            {"common/com/statespec/backend/memory/InMemoryTransaction.java", common,
             Kind::MemoryTransaction},
            {"common/com/statespec/backend/runtime/Codec.java", common, Kind::RuntimeCodec},
            {"common/com/statespec/backend/runtime/FeatureFlagCodec.java", common,
             Kind::RuntimeCodec},
            {"common/com/statespec/backend/runtime/QueueCodec.java", common, Kind::RuntimeCodec},
            {"common/com/statespec/backend/runtime/LeaseCodec.java", common, Kind::RuntimeCodec},
            {"common/com/statespec/backend/runtime/WorkflowCodec.java", common, Kind::RuntimeCodec},
            {"common/com/statespec/backend/runtime/ObservabilityCodec.java", common,
             Kind::RuntimeCodec},
            {"common/com/statespec/backend/runtime/LogCodec.java", common, Kind::RuntimeCodec},
            {"common/com/statespec/backend/runtime/MetricCodec.java", common, Kind::RuntimeCodec},
            {"common/com/statespec/backend/runtime/FeatureFlagStore.java", common,
             Kind::RuntimeFeatureFlagStore},
            {"common/com/statespec/backend/runtime/QueueStore.java", common,
             Kind::RuntimeQueueStore},
            {"common/com/statespec/backend/runtime/LeaseStore.java", common,
             Kind::RuntimeLeaseStore},
            {"common/com/statespec/backend/runtime/WorkflowStore.java", common,
             Kind::RuntimeWorkflowStore},
            {"common/com/statespec/backend/runtime/LogSink.java", common, Kind::RuntimeLogSink},
            {"common/com/statespec/backend/runtime/MetricSink.java", common,
             Kind::RuntimeMetricSink},
            {"common/com/statespec/backend/runtime/EntityGcDescriptors.java", common,
             Kind::RuntimeEntityGcDescriptors},
            {"common/com/statespec/backend/runtime/EntityGcRepository.java", common,
             Kind::RuntimeEntityGcRepository},
            {"common/com/statespec/backend/runtime/EntityGcWorkers.java", common,
             Kind::RuntimeEntityGcWorkers},
            {"common/com/statespec/backend/runtime/EntityGcRegistration.java", common,
             Kind::RuntimeEntityGcRegistration},
            {"api/com/statespec/generated/ApiApplication.java", api, Kind::ApiApplication},
            {"api/com/statespec/generated/ApiProcess.java", api, Kind::ApiProcess, true},
            {"api/com/statespec/generated/ApiServer.java", api, Kind::ApiServer, true},
            {"api/com/statespec/generated/ApiTransport.java", api, Kind::ApiTransport, true},
            {"api/com/statespec/generated/ApiDispatcher.java", api, Kind::ApiDispatcher, true},
            {"api/com/statespec/generated/ApiHandlerRegistry.java", api, Kind::ApiHandlerRegistry},
            {"api/com/statespec/generated/ApiMain.java", api, Kind::ApiMain},
            {"worker/com/statespec/generated/WorkerApplication.java", worker,
             Kind::WorkerApplication, true},
            {"worker/com/statespec/generated/WorkerProcess.java", worker, Kind::WorkerProcess,
             true},
            {"worker/com/statespec/generated/WorkerRuntime.java", worker, Kind::WorkerRuntime},
            {"worker/com/statespec/generated/WorkerRegistry.java", worker, Kind::WorkerRegistry,
             true},
            {"worker/com/statespec/generated/WorkflowRunner.java", worker, Kind::WorkflowRunner,
             true},
            {"worker/com/statespec/generated/WorkflowStepHandlers.java", worker,
             Kind::WorkflowStepHandlers, true},
            {"worker/com/statespec/generated/WorkerMain.java", worker, Kind::WorkerMain},
        }
    );

    require_exact_app_artifact_model(
        statespec::BindingLanguage::Rust, "rust",
        {
            {"common/memory/backend.rs", common, Kind::MemoryBackend},
            {"common/memory/transaction.rs", common, Kind::MemoryTransaction},
            {"common/runtime/codec.rs", common, Kind::RuntimeCodec},
            {"common/runtime/codec_core.rs", common, Kind::RuntimeCodec},
            {"common/runtime/codec_feature_flags.rs", common, Kind::RuntimeCodec},
            {"common/runtime/codec_queues.rs", common, Kind::RuntimeCodec},
            {"common/runtime/codec_leases.rs", common, Kind::RuntimeCodec},
            {"common/runtime/codec_workflows.rs", common, Kind::RuntimeCodec},
            {"common/runtime/codec_observability.rs", common, Kind::RuntimeCodec},
            {"common/runtime/codec_logs.rs", common, Kind::RuntimeCodec},
            {"common/runtime/codec_metrics.rs", common, Kind::RuntimeCodec},
            {"common/runtime/feature_flags.rs", common, Kind::RuntimeFeatureFlagStore},
            {"common/runtime/queues.rs", common, Kind::RuntimeQueueStore},
            {"common/runtime/leases.rs", common, Kind::RuntimeLeaseStore},
            {"common/runtime/workflows.rs", common, Kind::RuntimeWorkflowStore},
            {"common/runtime/logs.rs", common, Kind::RuntimeLogSink},
            {"common/runtime/metrics.rs", common, Kind::RuntimeMetricSink},
            {"common/runtime/entity_gc_descriptors.rs", common, Kind::RuntimeEntityGcDescriptors},
            {"common/runtime/entity_gc_repository.rs", common, Kind::RuntimeEntityGcRepository},
            {"common/runtime/entity_gc_workers.rs", common, Kind::RuntimeEntityGcWorkers},
            {"common/runtime/entity_gc_registration.rs", common, Kind::RuntimeEntityGcRegistration},
            {"api/api_application.rs", api, Kind::ApiApplication},
            {"api/api_process.rs", api, Kind::ApiProcess, true},
            {"api/api_server.rs", api, Kind::ApiServer, true},
            {"api/api_transport.rs", api, Kind::ApiTransport, true},
            {"api/api_dispatcher.rs", api, Kind::ApiDispatcher, true},
            {"api/api_handler_registry.rs", api, Kind::ApiHandlerRegistry},
            {"api/main.rs", api, Kind::ApiMain},
            {"worker/worker_application.rs", worker, Kind::WorkerApplication, true},
            {"worker/worker_process.rs", worker, Kind::WorkerProcess, true},
            {"worker/worker_runtime.rs", worker, Kind::WorkerRuntime},
            {"worker/worker_registry.rs", worker, Kind::WorkerRegistry, true},
            {"worker/workflow_runner.rs", worker, Kind::WorkflowRunner, true},
            {"worker/workflow_step_handlers.rs", worker, Kind::WorkflowStepHandlers, true},
            {"worker/main.rs", worker, Kind::WorkerMain},
        }
    );
}

} // namespace

TEST_CASE("binding template roots resolve predictably")
{
    test_binding_template_root_resolution();
}

TEST_CASE("binding app artifact kind names are stable")
{
    test_binding_app_artifact_kind_names();
}

TEST_CASE("binding app artifact models define application filenames")
{
    test_binding_app_artifact_models_define_application_filenames();
}
