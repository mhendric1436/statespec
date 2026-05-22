#include "binding_test_support.hpp"

namespace
{

statespec::GenerationResult generate_cpp_bindings_for_artifact_tests()
{
    statespec::DiagnosticBag diagnostics;
    const auto result = statespec::generate_bindings(
        empty_system_spec(),
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Cpp,
            "/tmp/statespec-artifact-tier-test/cpp",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );
    require(!diagnostics.has_errors(), "C++ artifact path generation should not fail");
    return result;
}

void test_cpp_binding_generator_emits_meaningful_artifact_filenames()
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
            {"common/runtime/codec_core.hpp", common},
            {"common/runtime/codec_feature_flags.hpp", common},
            {"common/runtime/codec_queues.hpp", common},
            {"common/runtime/codec_leases.hpp", common},
            {"common/runtime/codec_workflows.hpp", common},
            {"common/runtime/codec_observability.hpp", common},
            {"common/runtime/codec_logs.hpp", common},
            {"common/runtime/codec_metrics.hpp", common},
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
}

void test_cpp_binding_generator_models_artifact_paths()
{
    const auto result = generate_cpp_bindings_for_artifact_tests();
    require_generated_file_artifact_path(
        result, "descriptors.hpp", "common/descriptors.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "Makefile", "common/Makefile", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "api/api_descriptors.hpp", "api/api_descriptors.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/api_handlers.hpp", "api/api_handlers.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/api_dispatcher.hpp", "api/api_dispatcher.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/api_server.hpp", "api/api_server.hpp", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/api_routes.hpp", "api/api_routes.hpp", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/external_system_operator_metadata_api.hpp",
        "api/external_system_operator_metadata_api.hpp", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "worker/worker_descriptors.hpp", "worker/worker_descriptors.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_contexts.hpp", "worker/worker_contexts.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_registry.hpp", "worker/worker_registry.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_application.hpp", "worker/worker_application.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/workflow_step_handlers.hpp", "worker/workflow_step_handlers.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/workflow_runner.hpp", "worker/workflow_runner.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_handlers.hpp", "worker/worker_handlers.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_queues.hpp", "worker/worker_queues.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_leases.hpp", "worker/worker_leases.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_workflows.hpp", "worker/worker_workflows.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
}

} // namespace

TEST_CASE("C++ binding generator emits meaningful production filenames")
{
    test_cpp_binding_generator_emits_meaningful_artifact_filenames();
}

TEST_CASE("C++ binding generator models artifact paths")
{
    test_cpp_binding_generator_models_artifact_paths();
}
