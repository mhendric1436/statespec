#include "binding_test_support.hpp"

namespace
{

void test_generated_artifact_tiers_default_to_common()
{
    statespec::GeneratedFile file;
    file.path = "generated.txt";
    file.content = "content";
    require(
        file.tier == statespec::GeneratedArtifactTier::Common,
        "generated files should default to common tier"
    );
}

void test_binding_generators_assign_artifact_tiers()
{
    require_generated_files_have_tiered_artifact_paths(statespec::BindingLanguage::Cpp, "cpp");
    require_generated_files_have_tiered_artifact_paths(statespec::BindingLanguage::Go, "go");
    require_generated_files_have_tiered_artifact_paths(statespec::BindingLanguage::Java, "java");
    require_generated_files_have_tiered_artifact_paths(statespec::BindingLanguage::Rust, "rust");
}

void test_binding_generators_emit_meaningful_artifact_filenames()
{
    const auto common = statespec::GeneratedArtifactTier::Common;
    const auto api = statespec::GeneratedArtifactTier::Api;
    const auto worker = statespec::GeneratedArtifactTier::Worker;

    require_exact_generated_artifact_manifest(
        statespec::BindingLanguage::Cpp, "cpp",
        {
            {"common/backend.hpp", common},
            {"common/external_system.hpp", common},
            {"common/feature_flag.hpp", common},
            {"common/json.hpp", common},
            {"common/lease.hpp", common},
            {"common/log.hpp", common},
            {"common/metric.hpp", common},
            {"common/queue.hpp", common},
            {"common/workflow.hpp", common},
            {"common/memory/backend.hpp", common},
            {"common/memory/transaction.hpp", common},
            {"common/runtime/codec.hpp", common},
            {"common/runtime/feature_flag_store.hpp", common},
            {"common/runtime/queue_store.hpp", common},
            {"common/runtime/lease_store.hpp", common},
            {"common/runtime/workflow_store.hpp", common},
            {"common/runtime/log_sink.hpp", common},
            {"common/runtime/metric_sink.hpp", common},
            {"common/descriptors.hpp", common},
            {"common/Makefile", common},
            {"api/api_descriptors.hpp", api},
            {"api/api_handlers.hpp", api},
            {"api/api_dispatcher.hpp", api},
            {"api/api_server.hpp", api},
            {"api/api_routes.hpp", api},
            {"api/external_system_operator_metadata_api.hpp", api},
            {"worker/worker_contexts.hpp", worker},
            {"worker/worker_descriptors.hpp", worker},
            {"worker/worker_registry.hpp", worker},
            {"worker/worker_application.hpp", worker},
            {"worker/workflow_step_handlers.hpp", worker},
            {"worker/workflow_runner.hpp", worker},
            {"worker/worker_handlers.hpp", worker},
            {"worker/worker_leases.hpp", worker},
            {"worker/worker_queues.hpp", worker},
            {"worker/worker_workflows.hpp", worker},
        }
    );

    require_exact_generated_artifact_manifest(
        statespec::BindingLanguage::Go, "go",
        {
            {"common/backend/backend.go", common},
            {"common/backend/external_system.go", common},
            {"common/backend/feature_flag.go", common},
            {"common/backend/json.go", common},
            {"common/backend/lease.go", common},
            {"common/backend/log.go", common},
            {"common/backend/memory/backend.go", common},
            {"common/backend/memory/transaction.go", common},
            {"common/backend/runtime/codec.go", common},
            {"common/backend/runtime/feature_flags.go", common},
            {"common/backend/runtime/queues.go", common},
            {"common/backend/runtime/leases.go", common},
            {"common/backend/runtime/workflows.go", common},
            {"common/backend/runtime/logs.go", common},
            {"common/backend/runtime/metrics.go", common},
            {"common/backend/metric.go", common},
            {"common/backend/queue.go", common},
            {"common/backend/workflow.go", common},
            {"common/backend/descriptors.go", common},
            {"common/go.mod", common},
            {"common/Makefile", common},
            {"api/backend/api_descriptors.go", api},
            {"api/backend/api_handlers.go", api},
            {"api/backend/api_dispatcher.go", api},
            {"api/backend/api_server.go", api},
            {"api/backend/api_routes.go", api},
            {"api/backend/external_system_operator_metadata_api.go", api},
            {"worker/backend/worker_contexts.go", worker},
            {"worker/backend/worker_descriptors.go", worker},
            {"worker/backend/worker_registry.go", worker},
            {"worker/backend/worker_application.go", worker},
            {"worker/backend/workflow_step_handlers.go", worker},
            {"worker/backend/workflow_runner.go", worker},
            {"worker/backend/worker_handlers.go", worker},
            {"worker/backend/worker_leases.go", worker},
            {"worker/backend/worker_queues.go", worker},
            {"worker/backend/worker_workflows.go", worker},
        }
    );

    require_exact_generated_artifact_manifest(
        statespec::BindingLanguage::Java, "java",
        {
            {"common/com/statespec/backend/Backend.java", common},
            {"common/com/statespec/backend/ExternalSystem.java", common},
            {"common/com/statespec/backend/FeatureFlag.java", common},
            {"common/com/statespec/backend/Json.java", common},
            {"common/com/statespec/backend/Lease.java", common},
            {"common/com/statespec/backend/Log.java", common},
            {"common/com/statespec/backend/Metric.java", common},
            {"common/com/statespec/backend/Queue.java", common},
            {"common/com/statespec/backend/Workflow.java", common},
            {"common/com/statespec/backend/memory/InMemoryBackend.java", common},
            {"common/com/statespec/backend/memory/InMemoryTransaction.java", common},
            {"common/com/statespec/backend/runtime/Codec.java", common},
            {"common/com/statespec/backend/runtime/FeatureFlagStore.java", common},
            {"common/com/statespec/backend/runtime/QueueStore.java", common},
            {"common/com/statespec/backend/runtime/LeaseStore.java", common},
            {"common/com/statespec/backend/runtime/WorkflowStore.java", common},
            {"common/com/statespec/backend/runtime/LogSink.java", common},
            {"common/com/statespec/backend/runtime/MetricSink.java", common},
            {"common/com/statespec/generated/Descriptors.java", common},
            {"common/Makefile", common},
            {"api/com/statespec/generated/ApiDescriptors.java", api},
            {"api/com/statespec/generated/ApiHandlers.java", api},
            {"api/com/statespec/generated/ApiDispatcher.java", api},
            {"api/com/statespec/generated/ApiServer.java", api},
            {"api/com/statespec/generated/ApiRoutes.java", api},
            {"api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java", api},
            {"worker/com/statespec/generated/WorkerContexts.java", worker},
            {"worker/com/statespec/generated/WorkerDescriptors.java", worker},
            {"worker/com/statespec/generated/WorkerRegistry.java", worker},
            {"worker/com/statespec/generated/WorkerApplication.java", worker},
            {"worker/com/statespec/generated/WorkflowStepHandlers.java", worker},
            {"worker/com/statespec/generated/WorkflowRunner.java", worker},
            {"worker/com/statespec/generated/WorkerHandlers.java", worker},
            {"worker/com/statespec/generated/WorkerLeases.java", worker},
            {"worker/com/statespec/generated/WorkerQueues.java", worker},
            {"worker/com/statespec/generated/WorkerWorkflows.java", worker},
        }
    );

    require_exact_generated_artifact_manifest(
        statespec::BindingLanguage::Rust, "rust",
        {
            {"common/backend.rs", common},
            {"common/external_system.rs", common},
            {"common/feature_flag.rs", common},
            {"common/json.rs", common},
            {"common/lease.rs", common},
            {"common/log.rs", common},
            {"common/metric.rs", common},
            {"common/queue.rs", common},
            {"common/workflow.rs", common},
            {"common/memory/backend.rs", common},
            {"common/memory/transaction.rs", common},
            {"common/runtime/codec.rs", common},
            {"common/runtime/feature_flags.rs", common},
            {"common/runtime/queues.rs", common},
            {"common/runtime/leases.rs", common},
            {"common/runtime/workflows.rs", common},
            {"common/runtime/logs.rs", common},
            {"common/runtime/metrics.rs", common},
            {"common/descriptors.rs", common},
            {"common/Cargo.toml", common},
            {"common/lib.rs", common},
            {"common/Makefile", common},
            {"api/api_descriptors.rs", api},
            {"api/api_handlers.rs", api},
            {"api/api_dispatcher.rs", api},
            {"api/api_server.rs", api},
            {"api/api_routes.rs", api},
            {"api/external_system_operator_metadata_api.rs", api},
            {"worker/worker_contexts.rs", worker},
            {"worker/worker_descriptors.rs", worker},
            {"worker/worker_registry.rs", worker},
            {"worker/worker_application.rs", worker},
            {"worker/workflow_step_handlers.rs", worker},
            {"worker/workflow_runner.rs", worker},
            {"worker/worker_handlers.rs", worker},
            {"worker/worker_leases.rs", worker},
            {"worker/worker_queues.rs", worker},
            {"worker/worker_workflows.rs", worker},
        }
    );
}

void test_shared_descriptor_artifact_paths()
{
    const auto spec = empty_system_spec();
    statespec::DiagnosticBag diagnostics;

    const auto cpp_result = statespec::generate_bindings(
        spec,
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Cpp,
            "/tmp/statespec-artifact-tier-test/cpp",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );
    const auto go_result = statespec::generate_bindings(
        spec,
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Go,
            "/tmp/statespec-artifact-tier-test/go",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );
    const auto java_result = statespec::generate_bindings(
        spec,
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Java,
            "/tmp/statespec-artifact-tier-test/java",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );
    const auto rust_result = statespec::generate_bindings(
        spec,
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Rust,
            "/tmp/statespec-artifact-tier-test/rust",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );

    require(!diagnostics.has_errors(), "descriptor artifact path generation should not fail");
    require_generated_file_artifact_path(
        cpp_result, "descriptors.hpp", "common/descriptors.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        cpp_result, "Makefile", "common/Makefile", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        go_result, "backend/descriptors.go", "common/backend/descriptors.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        go_result, "go.mod", "common/go.mod", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        go_result, "Makefile", "common/Makefile", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        java_result, "com/statespec/generated/Descriptors.java",
        "common/com/statespec/generated/Descriptors.java", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        java_result, "Makefile", "common/Makefile", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        rust_result, "descriptors.rs", "common/descriptors.rs",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        rust_result, "Cargo.toml", "common/Cargo.toml", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        rust_result, "lib.rs", "common/lib.rs", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        rust_result, "Makefile", "common/Makefile", statespec::GeneratedArtifactTier::Common
    );

    require_generated_file_artifact_path(
        cpp_result, "api/api_descriptors.hpp", "api/api_descriptors.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        cpp_result, "api/api_handlers.hpp", "api/api_handlers.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        cpp_result, "api/api_dispatcher.hpp", "api/api_dispatcher.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        cpp_result, "api/api_server.hpp", "api/api_server.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        cpp_result, "api/api_routes.hpp", "api/api_routes.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        cpp_result, "api/external_system_operator_metadata_api.hpp",
        "api/external_system_operator_metadata_api.hpp", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "api/backend/api_descriptors.go", "api/backend/api_descriptors.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "api/backend/api_handlers.go", "api/backend/api_handlers.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "api/backend/api_dispatcher.go", "api/backend/api_dispatcher.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "api/backend/api_server.go", "api/backend/api_server.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "api/backend/api_routes.go", "api/backend/api_routes.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "api/backend/external_system_operator_metadata_api.go",
        "api/backend/external_system_operator_metadata_api.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "api/com/statespec/generated/ApiDescriptors.java",
        "api/com/statespec/generated/ApiDescriptors.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "api/com/statespec/generated/ApiHandlers.java",
        "api/com/statespec/generated/ApiHandlers.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "api/com/statespec/generated/ApiDispatcher.java",
        "api/com/statespec/generated/ApiDispatcher.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "api/com/statespec/generated/ApiServer.java",
        "api/com/statespec/generated/ApiServer.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "api/com/statespec/generated/ApiRoutes.java",
        "api/com/statespec/generated/ApiRoutes.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java",
        "api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api/api_descriptors.rs", "api/api_descriptors.rs",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api/api_handlers.rs", "api/api_handlers.rs",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api/api_dispatcher.rs", "api/api_dispatcher.rs",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api/api_server.rs", "api/api_server.rs", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api/api_routes.rs", "api/api_routes.rs", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api/external_system_operator_metadata_api.rs",
        "api/external_system_operator_metadata_api.rs", statespec::GeneratedArtifactTier::Api
    );

    require_generated_file_artifact_path(
        cpp_result, "worker/worker_descriptors.hpp", "worker/worker_descriptors.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_contexts.hpp", "worker/worker_contexts.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_registry.hpp", "worker/worker_registry.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_application.hpp", "worker/worker_application.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/workflow_step_handlers.hpp", "worker/workflow_step_handlers.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/workflow_runner.hpp", "worker/workflow_runner.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_handlers.hpp", "worker/worker_handlers.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_queues.hpp", "worker/worker_queues.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_leases.hpp", "worker/worker_leases.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_workflows.hpp", "worker/worker_workflows.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_descriptors.go", "worker/backend/worker_descriptors.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_contexts.go", "worker/backend/worker_contexts.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_registry.go", "worker/backend/worker_registry.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_application.go", "worker/backend/worker_application.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/workflow_step_handlers.go",
        "worker/backend/workflow_step_handlers.go", statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/workflow_runner.go", "worker/backend/workflow_runner.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_handlers.go", "worker/backend/worker_handlers.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_queues.go", "worker/backend/worker_queues.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_leases.go", "worker/backend/worker_leases.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_workflows.go", "worker/backend/worker_workflows.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerDescriptors.java",
        "worker/com/statespec/generated/WorkerDescriptors.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerContexts.java",
        "worker/com/statespec/generated/WorkerContexts.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerRegistry.java",
        "worker/com/statespec/generated/WorkerRegistry.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerApplication.java",
        "worker/com/statespec/generated/WorkerApplication.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkflowStepHandlers.java",
        "worker/com/statespec/generated/WorkflowStepHandlers.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkflowRunner.java",
        "worker/com/statespec/generated/WorkflowRunner.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerHandlers.java",
        "worker/com/statespec/generated/WorkerHandlers.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerQueues.java",
        "worker/com/statespec/generated/WorkerQueues.java", statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerLeases.java",
        "worker/com/statespec/generated/WorkerLeases.java", statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerWorkflows.java",
        "worker/com/statespec/generated/WorkerWorkflows.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_descriptors.rs", "worker/worker_descriptors.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_contexts.rs", "worker/worker_contexts.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_registry.rs", "worker/worker_registry.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_application.rs", "worker/worker_application.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/workflow_step_handlers.rs", "worker/workflow_step_handlers.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/workflow_runner.rs", "worker/workflow_runner.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_handlers.rs", "worker/worker_handlers.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_queues.rs", "worker/worker_queues.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_leases.rs", "worker/worker_leases.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_workflows.rs", "worker/worker_workflows.rs",
        statespec::GeneratedArtifactTier::Worker
    );
}

} // namespace

TEST_CASE("generated artifact tier defaults to common")
{
    test_generated_artifact_tiers_default_to_common();
}

TEST_CASE("binding generators tag current files as common artifacts")
{
    test_binding_generators_assign_artifact_tiers();
}

TEST_CASE("binding generators emit meaningful production filenames")
{
    test_binding_generators_emit_meaningful_artifact_filenames();
}

TEST_CASE("binding generators model shared descriptor artifact paths")
{
    test_shared_descriptor_artifact_paths();
}
