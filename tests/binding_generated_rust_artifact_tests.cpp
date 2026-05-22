#include "binding_test_support.hpp"

namespace
{

statespec::GenerationResult generate_rust_bindings_for_artifact_tests()
{
    statespec::DiagnosticBag diagnostics;
    const auto result = statespec::generate_bindings(
        empty_system_spec(),
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Rust,
            "/tmp/statespec-artifact-tier-test/rust",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );
    require(!diagnostics.has_errors(), "Rust artifact path generation should not fail");
    return result;
}

void test_rust_binding_generator_emits_meaningful_artifact_filenames()
{
    const auto common = statespec::GeneratedArtifactTier::Common;
    const auto api = statespec::GeneratedArtifactTier::Api;
    const auto worker = statespec::GeneratedArtifactTier::Worker;

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
            {"common/descriptors.rs", common},
            {"common/Cargo.toml", common},
            {"common/lib.rs", common},
            {"common/Makefile", common},
            {"api/api_codecs.rs", api},
            {"api/api_descriptors.rs", api},
            {"api/api_handlers.rs", api},
            {"api/api_handler_registry.rs", api},
            {"api/external_system_operator_metadata_api.rs", api},
            {"worker/worker_contexts.rs", worker},
            {"worker/worker_descriptors.rs", worker},
            {"worker/worker_registry.rs", worker},
            {"worker/worker_application.rs", worker},
            {"worker/worker_runtime.rs", worker},
            {"worker/workflow_step_handlers.rs", worker},
            {"worker/workflow_runner.rs", worker},
            {"worker/worker_leases.rs", worker},
            {"worker/worker_queues.rs", worker},
            {"worker/worker_workflows.rs", worker},
            {"worker/main.rs", worker},
        }
    );
}

void test_rust_binding_generator_models_artifact_paths()
{
    const auto result = generate_rust_bindings_for_artifact_tests();
    require_generated_file_artifact_path(
        result, "descriptors.rs", "common/descriptors.rs", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "Cargo.toml", "common/Cargo.toml", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "lib.rs", "common/lib.rs", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "Makefile", "common/Makefile", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "api/api_descriptors.rs", "api/api_descriptors.rs",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/api_codecs.rs", "api/api_codecs.rs", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/api_handlers.rs", "api/api_handlers.rs", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/api_handler_registry.rs", "api/api_handler_registry.rs",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/external_system_operator_metadata_api.rs",
        "api/external_system_operator_metadata_api.rs", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "worker/worker_descriptors.rs", "worker/worker_descriptors.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_contexts.rs", "worker/worker_contexts.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_registry.rs", "worker/worker_registry.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_application.rs", "worker/worker_application.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_runtime.rs", "worker/worker_runtime.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/workflow_step_handlers.rs", "worker/workflow_step_handlers.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/workflow_runner.rs", "worker/workflow_runner.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_queues.rs", "worker/worker_queues.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_leases.rs", "worker/worker_leases.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/worker_workflows.rs", "worker/worker_workflows.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/main.rs", "worker/main.rs", statespec::GeneratedArtifactTier::Worker
    );
}

} // namespace

TEST_CASE("Rust binding generator emits meaningful production filenames")
{
    test_rust_binding_generator_emits_meaningful_artifact_filenames();
}

TEST_CASE("Rust binding generator models artifact paths")
{
    test_rust_binding_generator_models_artifact_paths();
}
