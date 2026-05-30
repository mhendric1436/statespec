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
            {"common/shape_types.rs", common},
            {"common/workflow.rs", common},
            {"common/memory/backend.rs", common},
            {"common/memory/transaction.rs", common},
            {"common/descriptors.rs", common},
            {"common/descriptors/types.rs", common},
            {"common/descriptors/core.rs", common},
            {"common/descriptors/events.rs", common},
            {"common/descriptors/external_systems.rs", common},
            {"common/descriptors/observability.rs", common},
            {"common/descriptors/policies.rs", common},
            {"common/descriptors/runtime.rs", common},
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
            {"api/shapes.rs", api},
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
        result, "shape_types.rs", "common/shape_types.rs", statespec::GeneratedArtifactTier::Common
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
        result, "descriptors/observability.rs", "common/descriptors/observability.rs",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/policies.rs", "common/descriptors/policies.rs",
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

void test_rust_entity_api_catalog_artifacts_are_operation_owned()
{
    const auto result =
        generate_entity_api_catalog_bindings(statespec::BindingLanguage::Rust, "rust");
    require_generated_artifact_exists(result, "api/entities/account/catalog.rs");
    require_generated_artifact_not_exists(result, "api/entities/audit_log/catalog.rs");
    require_generated_artifact_exists(result, "common/entities/account/model.rs");
    require_generated_artifact_exists(result, "common/entities/audit_log/model.rs");
    require_generated_artifact_exists(result, "common/entities/account/constants.rs");
    require_generated_artifact_exists(result, "common/entities/audit_log/constants.rs");
    require_generated_artifact_contains(
        result, "common/entities/account/constants.rs",
        "pub const ACCOUNT_ENTITY_NAME: &str = \"Account\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.rs",
        "pub const ACCOUNT_COLLECTION_NAME: &str = \"Account\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.rs",
        "pub const ACCOUNT_FIELD_TENANT_ID: &str = \"tenant_id\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.rs",
        "pub const ACCOUNT_FIELD_CREATED_AT_TYPE_NAME: &str = \"timestamp\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.rs",
        "pub const ACCOUNT_INDEX_BY_TENANT_ACCOUNT: &str = \"by_tenant_account\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.rs",
        "pub const ACCOUNT_INDEX_BY_TENANT_ACCOUNT_HELPER_NAME: &str = "
        "\"list_by_tenant_account_tx\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.rs",
        "pub const ACCOUNT_STATUS_ACTIVE: &str = \"Active\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.rs",
        "pub const ACCOUNT_KEY_HELPER_NAME: &str = \"account_lookup\""
    );
    require_generated_artifact_not_contains(
        result, "common/entities/account/model.rs",
        "pub const ACCOUNT_ENTITY_NAME: &str = \"Account\""
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.rs",
        "use crate::api_shapes::entity_account_shapes as shapes;"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.rs", "#[path = \"codecs.rs\"]"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.rs", "#[path = \"handlers.rs\"]"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.rs", "#[path = \"registry.rs\"]"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.rs", "pub fn shape_descriptors()"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.rs", "pub fn api_descriptors()"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.rs", "pub fn api_route_descriptors()"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.rs", "pub fn handler_entrypoints()"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.rs", "#[path = \"descriptors/create_account.rs\"]"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/descriptors/create_account.rs",
        "pub fn create_account_api_descriptors()"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/descriptors/create_account.rs",
        "use crate::entity_account::constants as entity_constants;"
    );
    require_generated_artifact_not_exists(result, "api/descriptors/create_account.rs");
    require_generated_artifact_contains(
        result, "api/shapes.rs", "entity_account_catalog::shape_descriptors()"
    );
    require_generated_artifact_contains(
        result, "api/descriptors/catalog.rs", "entity_account_catalog::api_descriptors()"
    );
    require_generated_artifact_contains(
        result, "api/descriptors/catalog.rs", "entity_account_catalog::api_route_descriptors()"
    );
    require_generated_artifact_contains(
        result, "api/api_handler_registry.rs",
        "api_handler_registry_account::register_handler_invokers::<B>(handlers)"
    );
    require_generated_artifact_not_contains(
        result, "api/descriptors/catalog.rs", "create_account_api_descriptors()"
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

TEST_CASE("Rust entity API catalog artifacts are emitted only for API-owned operations")
{
    test_rust_entity_api_catalog_artifacts_are_operation_owned();
}
