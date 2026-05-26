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
            std::filesystem::path{statespec::test::ArtifactTierTestRoot} / "cpp",
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
            {"common/schema_compatibility.hpp", common},
            {"common/workflow.hpp", common},
            {"common/memory/backend.hpp", common},
            {"common/memory/transaction.hpp", common},
            {"common/descriptors.hpp", common},
            {"common/descriptors/apis.hpp", common},
            {"common/descriptors/core.hpp", common},
            {"common/descriptors/events.hpp", common},
            {"common/descriptors/runtime.hpp", common},
            {"common/descriptors/shapes.hpp", common},
            {"common/descriptors/workers.hpp", common},
            {"common/shapes.hpp", common},
            {"common/Makefile", common},
            {"api/api_codecs.hpp", api},
            {"api/api_codec_support.hpp", api},
            {"api/api_descriptors.hpp", api},
            {"api/api_handlers.hpp", api},
            {"api/api_handler_registry.hpp", api},
            {"api/api_handler_registry_support.hpp", api},
            {"api/external_system_operator_metadata_api.hpp", api},
            {"worker/worker_contexts.hpp", worker},
            {"worker/worker_descriptors.hpp", worker},
            {"worker/worker_registry.hpp", worker},
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
        result, "descriptors/core.hpp", "common/descriptors/core.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/events.hpp", "common/descriptors/events.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/shapes.hpp", "common/descriptors/shapes.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "shapes.hpp", "common/shapes.hpp", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/apis.hpp", "common/descriptors/apis.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/workers.hpp", "common/descriptors/workers.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/runtime.hpp", "common/descriptors/runtime.hpp",
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
        result, "api/api_codecs.hpp", "api/api_codecs.hpp", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/api_codec_support.hpp", "api/api_codec_support.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/api_handlers.hpp", "api/api_handlers.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/api_handler_registry.hpp", "api/api_handler_registry.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/api_handler_registry_support.hpp", "api/api_handler_registry_support.hpp",
        statespec::GeneratedArtifactTier::Api
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
