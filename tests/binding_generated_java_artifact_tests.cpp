#include "binding_test_support.hpp"

namespace
{

statespec::GenerationResult generate_java_bindings_for_artifact_tests()
{
    statespec::DiagnosticBag diagnostics;
    const auto result = statespec::generate_bindings(
        empty_system_spec(),
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Java,
            std::filesystem::path{statespec::test::ArtifactTierTestRoot} / "java",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );
    require(!diagnostics.has_errors(), "Java artifact path generation should not fail");
    return result;
}

void test_java_binding_generator_emits_meaningful_artifact_filenames()
{
    const auto common = statespec::GeneratedArtifactTier::Common;
    const auto api = statespec::GeneratedArtifactTier::Api;
    const auto worker = statespec::GeneratedArtifactTier::Worker;

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
            {"common/com/statespec/backend/SchemaCompatibility.java", common},
            {"common/com/statespec/backend/Workflow.java", common},
            {"common/com/statespec/backend/memory/InMemoryBackend.java", common},
            {"common/com/statespec/backend/memory/InMemoryTransaction.java", common},
            {"common/com/statespec/generated/Descriptors.java", common},
            {"common/com/statespec/generated/descriptors/ApiDescriptorModule.java", common},
            {"common/com/statespec/generated/descriptors/CoreDescriptorModule.java", common},
            {"common/com/statespec/generated/descriptors/EventDescriptorModule.java", common},
            {"common/com/statespec/generated/descriptors/RuntimeDescriptorModule.java", common},
            {"common/com/statespec/generated/descriptors/ShapeDescriptorModule.java", common},
            {"common/com/statespec/generated/descriptors/WorkerDescriptorModule.java", common},
            {"common/Makefile", common},
            {"api/com/statespec/generated/ApiCodecs.java", api},
            {"api/com/statespec/generated/ApiDescriptors.java", api},
            {"api/com/statespec/generated/ApiHandlers.java", api},
            {"api/com/statespec/generated/ApiHandlerRegistry.java", api},
            {"api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java", api},
            {"worker/com/statespec/generated/WorkerContexts.java", worker},
            {"worker/com/statespec/generated/WorkerDescriptors.java", worker},
            {"worker/com/statespec/generated/WorkerRegistry.java", worker},
        }
    );
}

void test_java_binding_generator_models_artifact_paths()
{
    const auto result = generate_java_bindings_for_artifact_tests();
    require_generated_file_artifact_path(
        result, "com/statespec/generated/Descriptors.java",
        "common/com/statespec/generated/Descriptors.java", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "com/statespec/generated/descriptors/ApiDescriptorModule.java",
        "common/com/statespec/generated/descriptors/ApiDescriptorModule.java",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "com/statespec/generated/descriptors/CoreDescriptorModule.java",
        "common/com/statespec/generated/descriptors/CoreDescriptorModule.java",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "com/statespec/generated/descriptors/EventDescriptorModule.java",
        "common/com/statespec/generated/descriptors/EventDescriptorModule.java",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "com/statespec/generated/descriptors/RuntimeDescriptorModule.java",
        "common/com/statespec/generated/descriptors/RuntimeDescriptorModule.java",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "com/statespec/generated/descriptors/ShapeDescriptorModule.java",
        "common/com/statespec/generated/descriptors/ShapeDescriptorModule.java",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "com/statespec/generated/descriptors/WorkerDescriptorModule.java",
        "common/com/statespec/generated/descriptors/WorkerDescriptorModule.java",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "Makefile", "common/Makefile", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "api/com/statespec/generated/ApiDescriptors.java",
        "api/com/statespec/generated/ApiDescriptors.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/com/statespec/generated/ApiCodecs.java",
        "api/com/statespec/generated/ApiCodecs.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/com/statespec/generated/ApiHandlers.java",
        "api/com/statespec/generated/ApiHandlers.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/com/statespec/generated/ApiHandlerRegistry.java",
        "api/com/statespec/generated/ApiHandlerRegistry.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java",
        "api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "worker/com/statespec/generated/WorkerDescriptors.java",
        "worker/com/statespec/generated/WorkerDescriptors.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/com/statespec/generated/WorkerContexts.java",
        "worker/com/statespec/generated/WorkerContexts.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        result, "worker/com/statespec/generated/WorkerRegistry.java",
        "worker/com/statespec/generated/WorkerRegistry.java",
        statespec::GeneratedArtifactTier::Worker
    );
}

} // namespace

TEST_CASE("Java binding generator emits meaningful production filenames")
{
    test_java_binding_generator_emits_meaningful_artifact_filenames();
}

TEST_CASE("Java binding generator models artifact paths")
{
    test_java_binding_generator_models_artifact_paths();
}
