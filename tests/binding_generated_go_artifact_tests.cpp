#include "binding_test_support.hpp"

namespace
{

statespec::GenerationResult generate_go_bindings_for_artifact_tests()
{
    statespec::DiagnosticBag diagnostics;
    const auto result = statespec::generate_bindings(
        empty_system_spec(),
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Go,
            "/tmp/statespec-artifact-tier-test/go",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );
    require(!diagnostics.has_errors(), "Go artifact path generation should not fail");
    return result;
}

void test_go_binding_generator_emits_meaningful_artifact_filenames()
{
    const auto common = statespec::GeneratedArtifactTier::Common;
    const auto api = statespec::GeneratedArtifactTier::Api;
    const auto worker = statespec::GeneratedArtifactTier::Worker;

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
            {"common/backend/metric.go", common},
            {"common/backend/queue.go", common},
            {"common/backend/workflow.go", common},
            {"common/backend/descriptors.go", common},
            {"common/go.mod", common},
            {"common/Makefile", common},
            {"api/backend/api_application.go", api},
            {"api/backend/api_codecs.go", api},
            {"api/backend/api_descriptors.go", api},
            {"api/backend/api_handlers.go", api},
            {"api/backend/api_handler_registry.go", api},
            {"api/backend/api_dispatcher.go", api},
            {"api/backend/api_server.go", api},
            {"api/backend/api_routes.go", api},
            {"api/backend/external_system_operator_metadata_api.go", api},
            {"api/cmd/api/main.go", api},
            {"worker/backend/worker_contexts.go", worker},
            {"worker/backend/worker_descriptors.go", worker},
            {"worker/backend/worker_registry.go", worker},
            {"worker/backend/worker_application.go", worker},
            {"worker/backend/worker_runtime.go", worker},
            {"worker/backend/workflow_step_handlers.go", worker},
            {"worker/backend/workflow_runner.go", worker},
            {"worker/backend/worker_leases.go", worker},
            {"worker/backend/worker_queues.go", worker},
            {"worker/backend/worker_workflows.go", worker},
            {"worker/cmd/worker/main.go", worker},
        }
    );
}

void test_go_binding_generator_models_artifact_paths()
{
    const auto result = generate_go_bindings_for_artifact_tests();
    require_generated_file_artifact_path(
        result, "backend/descriptors.go", "common/backend/descriptors.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "go.mod", "common/go.mod", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "Makefile", "common/Makefile", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "api/backend/api_descriptors.go", "api/backend/api_descriptors.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/backend/api_application.go", "api/backend/api_application.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/backend/api_codecs.go", "api/backend/api_codecs.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/backend/api_handlers.go", "api/backend/api_handlers.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/backend/api_handler_registry.go", "api/backend/api_handler_registry.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/backend/api_dispatcher.go", "api/backend/api_dispatcher.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/backend/api_server.go", "api/backend/api_server.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/backend/api_routes.go", "api/backend/api_routes.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/backend/external_system_operator_metadata_api.go",
        "api/backend/external_system_operator_metadata_api.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/cmd/api/main.go", "api/cmd/api/main.go", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "worker/backend/worker_descriptors.go", "worker/backend/worker_descriptors.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/backend/worker_contexts.go", "worker/backend/worker_contexts.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/backend/worker_registry.go", "worker/backend/worker_registry.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/backend/worker_application.go", "worker/backend/worker_application.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/backend/worker_runtime.go", "worker/backend/worker_runtime.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/backend/workflow_step_handlers.go",
        "worker/backend/workflow_step_handlers.go", statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/backend/workflow_runner.go", "worker/backend/workflow_runner.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/backend/worker_queues.go", "worker/backend/worker_queues.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/backend/worker_leases.go", "worker/backend/worker_leases.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/backend/worker_workflows.go", "worker/backend/worker_workflows.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/cmd/worker/main.go", "worker/cmd/worker/main.go",
        statespec::GeneratedArtifactTier::Worker
    );
}

} // namespace

TEST_CASE("Go binding generator emits meaningful production filenames")
{
    test_go_binding_generator_emits_meaningful_artifact_filenames();
}

TEST_CASE("Go binding generator models artifact paths")
{
    test_go_binding_generator_models_artifact_paths();
}
