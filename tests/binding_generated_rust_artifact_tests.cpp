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
            std::filesystem::path{statespec::test::ArtifactTierTestRoot} / "rust",
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
            {"common/schema_compatibility.rs", common},
            {"common/workflow.rs", common},
            {"common/memory/backend.rs", common},
            {"common/memory/transaction.rs", common},
            {"common/descriptors.rs", common},
            {"common/descriptors/types.rs", common},
            {"common/descriptors/core.rs", common},
            {"common/descriptors/events.rs", common},
            {"common/descriptors/external_systems.rs", common},
            {"common/descriptors/runtime.rs", common},
            {"common/descriptors/shapes.rs", common},
            {"common/entity_repository.rs", common},
            {"common/runtime_registration.rs", common},
            {"common/Cargo.toml", common},
            {"common/lib.rs", common},
            {"common/Makefile", common},
            {"api/api_codecs.rs", api},
            {"api/api_descriptors.rs", api},
            {"api/api_handlers.rs", api},
            {"api/api_handler_registry.rs", api},
            {"api/descriptors/catalog.rs", api},
            {"api/external_system_operator_metadata_api.rs", api},
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
        result, "descriptors/types.rs", "common/descriptors/types.rs",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/core.rs", "common/descriptors/core.rs",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/events.rs", "common/descriptors/events.rs",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/external_systems.rs", "common/descriptors/external_systems.rs",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/runtime.rs", "common/descriptors/runtime.rs",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "entity_repository.rs", "common/entity_repository.rs",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "runtime_registration.rs", "common/runtime_registration.rs",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/shapes.rs", "common/descriptors/shapes.rs",
        statespec::GeneratedArtifactTier::Common
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
        result, "api/descriptors/catalog.rs", "api/descriptors/catalog.rs",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/external_system_operator_metadata_api.rs",
        "api/external_system_operator_metadata_api.rs", statespec::GeneratedArtifactTier::Api
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
