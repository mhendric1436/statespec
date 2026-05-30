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
            {"common/shape_types.hpp", common},
            {"common/workflow.hpp", common},
            {"common/memory/backend.hpp", common},
            {"common/memory/transaction.hpp", common},
            {"common/descriptors.hpp", common},
            {"common/descriptors/core.hpp", common},
            {"common/descriptors/events.hpp", common},
            {"common/descriptors/external_systems.hpp", common},
            {"common/descriptors/policies.hpp", common},
            {"common/descriptors/runtime.hpp", common},
            {"common/descriptors/types.hpp", common},
            {"common/descriptors/values_enums.hpp", common},
            {"common/entity_repository.hpp", common},
            {"common/runtime_registration.hpp", common},
            {"common/Makefile", common},
            {"api/api_codecs.hpp", api},
            {"api/api_codec_support.hpp", api},
            {"api/api_descriptors.hpp", api},
            {"api/api_handlers.hpp", api},
            {"api/api_handler_registry.hpp", api},
            {"api/api_handler_registry_support.hpp", api},
            {"api/descriptors/catalog.hpp", api},
            {"api/external_system_operator_metadata_api.hpp", api},
            {"api/shapes.hpp", api},
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
        result, "descriptors/external_systems.hpp", "common/descriptors/external_systems.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/policies.hpp", "common/descriptors/policies.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/types.hpp", "common/descriptors/types.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "shape_types.hpp", "common/shape_types.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/runtime.hpp", "common/descriptors/runtime.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "descriptors/values_enums.hpp", "common/descriptors/values_enums.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "entity_repository.hpp", "common/entity_repository.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        result, "runtime_registration.hpp", "common/runtime_registration.hpp",
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
        result, "api/descriptors/catalog.hpp", "api/descriptors/catalog.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        result, "api/external_system_operator_metadata_api.hpp",
        "api/external_system_operator_metadata_api.hpp", statespec::GeneratedArtifactTier::Api
    );
}

void test_cpp_entity_api_catalog_artifacts_are_operation_owned()
{
    const auto result =
        generate_entity_api_catalog_bindings(statespec::BindingLanguage::Cpp, "cpp");
    require_generated_artifact_exists(result, "api/entities/account/catalog.hpp");
    require_generated_artifact_not_exists(result, "api/entities/audit_log/catalog.hpp");
    require_generated_artifact_exists(result, "common/entities/account/model.hpp");
    require_generated_artifact_exists(result, "common/entities/audit_log/model.hpp");
    require_generated_artifact_exists(result, "common/entities/account/constants.hpp");
    require_generated_artifact_exists(result, "common/entities/audit_log/constants.hpp");
    require_generated_artifact_exists(result, "api/entities/account/constants.hpp");
    require_generated_artifact_not_exists(result, "api/entities/audit_log/constants.hpp");
    require_generated_artifact_exists(result, "api/servers/entity_api/constants.hpp");
    require_generated_artifact_contains(
        result, "common/entities/account/constants.hpp", "namespace constants"
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.hpp",
        "inline constexpr const char* kAccountEntityName = \"Account\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.hpp",
        "inline constexpr const char* kAccountCollectionName = \"Account\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.hpp",
        "inline constexpr const char* kAccountFieldTenantId = \"tenant_id\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.hpp",
        "inline constexpr const char* kAccountFieldCreatedAtTypeName = \"timestamp\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.hpp",
        "inline constexpr const char* kAccountIndexByTenantAccount = \"by_tenant_account\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.hpp",
        "inline constexpr const char* kAccountIndexByTenantAccountHelperName = "
        "\"listByTenantAccountTx\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.hpp",
        "inline constexpr const char* kAccountStatusActive = \"Active\""
    );
    require_generated_artifact_contains(
        result, "common/entities/account/constants.hpp",
        "inline constexpr const char* kAccountKeyHelperName = \"account_lookup\""
    );
    require_generated_artifact_not_contains(
        result, "common/entities/account/model.hpp",
        "inline constexpr const char* kAccountEntityName = \"Account\""
    );
    require_generated_artifact_contains(
        result, "api/entities/account/constants.hpp",
        "inline constexpr const char* kCreateAccountApiName = \"CreateAccount\""
    );
    require_generated_artifact_contains(
        result, "api/entities/account/constants.hpp",
        "inline constexpr const char* kEntityApiCreateAccountRouteName = "
        "\"EntityApi.CreateAccount\""
    );
    require_generated_artifact_contains(
        result, "api/entities/account/constants.hpp",
        "inline constexpr const char* kCreateAccountRequestShapeName = "
        "\"CreateAccountRequest\""
    );
    require_generated_artifact_contains(
        result, "api/entities/account/constants.hpp",
        "inline constexpr const char* kAccountListResponseEnvelopeName = "
        "::statespec_generated::entities::account::constants::kAccountEntityPluralName"
    );
    require_generated_artifact_contains(
        result, "api/servers/entity_api/constants.hpp",
        "inline constexpr const char* kEntityApiServerName = \"EntityApi\""
    );
    require_generated_artifact_contains(
        result, "api/servers/entity_api/constants.hpp",
        "inline constexpr int kEntityApiServerConcurrency = 1"
    );
    require_generated_artifact_not_contains(
        result, "api/servers/entity_api/constants.hpp", "CreateAccount"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.hpp", "#include \"shapes.hpp\""
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.hpp", "#include \"codecs.hpp\""
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.hpp", "#include \"handlers.hpp\""
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.hpp", "#include \"registry.hpp\""
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.hpp", "#include \"descriptors/create_account.hpp\""
    );
    require_generated_artifact_exists(
        result, "api/entities/account/descriptors/create_account.hpp"
    );
    require_generated_artifact_not_contains(
        result, "api/entities/account/descriptors/create_account.hpp",
        "#include \"../../common/entities/account/constants.hpp\""
    );
    require_generated_artifact_contains(
        result, "api/entities/account/descriptors/create_account.hpp",
        "#include \"../../../../common/entities/account/constants.hpp\""
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.hpp", "shape_descriptors()"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.hpp", "api_descriptors()"
    );
    require_generated_artifact_contains(result, "api/entities/account/catalog.hpp", "api_names()");
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.hpp", "constants::kCreateAccountApiName"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.hpp", "api_route_descriptors()"
    );
    require_generated_artifact_contains(
        result, "api/entities/account/catalog.hpp", "handler_entrypoints()"
    );
    require_generated_artifact_contains(
        result, "api/shapes.hpp", "api::entities::account::shape_descriptors"
    );
    require_generated_artifact_contains(
        result, "api/descriptors/catalog.hpp", "api::entities::account::api_descriptors"
    );
    require_generated_artifact_contains(
        result, "api/descriptors/catalog.hpp", "api::entities::account::api_route_descriptors"
    );
    require_generated_artifact_contains(
        result, "api/api_handler_registry.hpp",
        "api::entities::account::register_handler_invokers(handlers)"
    );
    require_generated_artifact_not_contains(
        result, "api/descriptors/catalog.hpp", "create_account_api_descriptor"
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

TEST_CASE("C++ entity API catalog artifacts are emitted only for API-owned operations")
{
    test_cpp_entity_api_catalog_artifacts_are_operation_owned();
}
