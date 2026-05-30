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
            std::filesystem::path{statespec::test::ArtifactTierTestRoot} / "go",
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
            {"common/backend/schema_compatibility.go", common},
            {"common/backend/shape_types.go", common},
            {"common/backend/workflow.go", common},
            {"common/backend/descriptors.go", common},
            {"common/backend/descriptortypes/types.go", common},
            {"common/backend/events.go", common},
            {"common/backend/external_systems.go", common},
            {"common/backend/descriptors/core.go", common},
            {"common/backend/descriptors/runtime.go", common},
            {"common/backend/policy_descriptors.go", common},
            {"common/backend/values_enums_descriptors.go", common},
            {"common/backend/entity_repository.go", common},
            {"common/backend/runtime_registration.go", common},
            {"common/go.mod", common},
            {"common/Makefile", common},
            {"api/backend/api_codecs.go", api},
            {"api/backend/api_descriptors.go", api},
            {"api/backend/api_handlers.go", api},
            {"api/backend/api_handler_registry.go", api},
            {"api/backend/codecsupport/api_codecs.go", api},
            {"api/backend/descriptors/catalog.go", api},
            {"api/backend/shapes/catalog.go", api},
            {"api/backend/external_system_operator_metadata_api.go", api},
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
        result, "backend/descriptortypes/types.go", "common/backend/descriptortypes/types.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "backend/shape_types.go", "common/backend/shape_types.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "backend/events.go", "common/backend/events.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "backend/external_systems.go", "common/backend/external_systems.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "backend/descriptors/core.go", "common/backend/descriptors/core.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "backend/descriptors/runtime.go", "common/backend/descriptors/runtime.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "backend/policy_descriptors.go", "common/backend/policy_descriptors.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "backend/values_enums_descriptors.go", "common/backend/values_enums_descriptors.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "backend/entity_repository.go", "common/backend/entity_repository.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "backend/runtime_registration.go", "common/backend/runtime_registration.go",
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
        result, "api/backend/api_codecs.go", "api/backend/api_codecs.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/backend/codecsupport/api_codecs.go", "api/backend/codecsupport/api_codecs.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/backend/descriptors/catalog.go", "api/backend/descriptors/catalog.go",
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
        result, "api/backend/external_system_operator_metadata_api.go",
        "api/backend/external_system_operator_metadata_api.go",
        statespec::GeneratedArtifactTier::Api
    );
}

void test_go_entity_api_catalog_artifacts_are_operation_owned()
{
    const auto result = generate_entity_api_catalog_bindings(statespec::BindingLanguage::Go, "go");
    require_generated_artifact_exists(result, "api/backend/entities/account/catalog.go");
    require_generated_artifact_not_exists(result, "api/backend/entities/audit_log/catalog.go");
    require_generated_artifact_exists(result, "common/entities/account/model.go");
    require_generated_artifact_exists(result, "common/entities/audit_log/model.go");
    require_generated_artifact_exists(result, "common/entities/account/constants.go");
    require_generated_artifact_exists(result, "common/entities/audit_log/constants.go");
    require_generated_artifact_contains(
        result, "common/entities/account/constants.go", "AccountEntityName = \"Account\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.go", "AccountCollectionName = \"Account\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.go", "AccountFieldTenantId = \"tenant_id\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.go",
        "AccountFieldCreatedAtTypeName = \"timestamp\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.go",
        "AccountIndexByTenantAccount = \"by_tenant_account\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.go",
        "AccountIndexByTenantAccountHelperName = \"ListByTenantAccountTx\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.go", "AccountStatusActive = \"Active\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.go",
        "AccountKeyHelperName = "
        "\"AccountLookup\""
    );
    require_generated_artifact_not_contains(
        result, "common/entities/account/model.go", "AccountEntityName = \"Account\""
    );
    require_generated_artifact_contains(
        result, "api/backend/entities/account/catalog.go", "package account"
    );
    require_generated_artifact_contains(
        result, "api/backend/entities/account/catalog.go", "func EntityShapeDescriptors"
    );
    require_generated_artifact_contains(
        result, "api/backend/entities/account/catalog.go", "func EntityAPIDescriptors"
    );
    require_generated_artifact_contains(
        result, "api/backend/entities/account/catalog.go", "func EntityAPIRouteDescriptors"
    );
    require_generated_artifact_contains(
        result, "api/backend/entities/account/catalog.go", "func HandlerEntrypoints"
    );
    require_generated_artifact_contains(
        result, "api/backend/entities/account/catalog.go", "func NewHandlerRegistry"
    );
    require_generated_artifact_contains(
        result, "api/backend/entities/account/catalog.go", "CreateAccountApiDescriptors()"
    );
    require_generated_artifact_contains(
        result, "api/backend/entities/account/catalog.go", "CreateAccountApiRouteDescriptors()"
    );
    require_generated_artifact_contains(
        result, "api/backend/entities/account/create_account_descriptor.go", "package account"
    );
    require_generated_artifact_contains(
        result, "api/backend/entities/account/create_account_descriptor.go",
        "accountconstants \"statespec-generated/common/entities/account\""
    );
    require_generated_artifact_not_exists(result, "api/backend/descriptors/create_account.go");
    require_generated_artifact_contains(
        result, "api/backend/shapes/catalog.go", "account.EntityShapeDescriptors()"
    );
    require_generated_artifact_contains(
        result, "api/backend/descriptors/catalog.go", "account.EntityAPIDescriptors()"
    );
    require_generated_artifact_contains(
        result, "api/backend/descriptors/catalog.go", "account.EntityAPIRouteDescriptors()"
    );
    require_generated_artifact_contains(
        result, "api/backend/api_handler_registry.go", "account.NewHandlerRegistry"
    );
    require_generated_artifact_not_contains(
        result, "api/backend/descriptors/catalog.go", "CreateAccountAPIDescriptors()"
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

TEST_CASE("Go entity API catalog artifacts are emitted only for API-owned operations")
{
    test_go_entity_api_catalog_artifacts_are_operation_owned();
}
