#include "binding_test_support.hpp"

#include <string_view>

namespace
{

inline constexpr std::string_view CrudContractDiagnostic = "SSPEC5105";

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

statespec::IrEntity account_ir_entity()
{
    statespec::IrEntity entity;
    entity.name = "Account";
    entity.key_fields = {"tenant_id", "account_id"};
    entity.fields = {
        {"created_at", "timestamp"}, {"updated_at", "timestamp"}, {"status", "string"},
        {"tenant_id", "string"},     {"account_id", "string"},    {"display_name", "string"},
    };
    entity.indexes.push_back({"by_tenant_status", {"tenant_id", "status"}, false});
    return entity;
}

statespec::IrShape shape(
    std::string name,
    std::vector<statespec::IrField> fields
)
{
    statespec::IrShape result;
    result.name = std::move(name);
    result.fields = std::move(fields);
    return result;
}

statespec::IrSystem crud_contract_system()
{
    statespec::IrSystem system;
    system.name = "CrudContractSystem";
    system.entities.push_back(account_ir_entity());
    system.shapes.push_back(shape("CreateAccountRequest", {{"display_name", "string"}}));
    system.shapes.push_back(shape(
        "AccountResponse", {
                               {"tenant_id", "string"},
                               {"account_id", "string"},
                               {"display_name", "string"},
                               {"status", "string"},
                               {"created_at", "timestamp"},
                               {"updated_at", "timestamp"},
                           }
    ));
    system.shapes.push_back(shape(
        "ListAccountsByStatusResponse", {
                                            {"tenant_id", "string"},
                                            {"status", "string"},
                                            {"accounts", "AccountResponse[]"},
                                        }
    ));
    return system;
}

statespec::GenerationResult generate_language_bindings(
    statespec::BindingLanguage language,
    const statespec::IrSystem& system,
    statespec::DiagnosticBag& diagnostics
)
{
    const statespec::BindingGeneratorOptions options{
        language,
        std::filesystem::path{statespec::test::ArtifactManifestTestRoot} / "crud-contract",
        statespec::BindingGenerationTier::All,
        {},
    };
    switch (language)
    {
    case statespec::BindingLanguage::Cpp:
        return statespec::generate_cpp_bindings(system, options, diagnostics);
    case statespec::BindingLanguage::Go:
        return statespec::generate_go_bindings(system, options, diagnostics);
    case statespec::BindingLanguage::Java:
        return statespec::generate_java_bindings(system, options, diagnostics);
    case statespec::BindingLanguage::Rust:
        return statespec::generate_rust_bindings(system, options, diagnostics);
    }
    return {};
}

bool diagnostics_have_code(
    const statespec::DiagnosticBag& diagnostics,
    std::string_view code
)
{
    for (const auto& diagnostic : diagnostics.all())
    {
        if (diagnostic.severity == statespec::DiagnosticSeverity::Error && diagnostic.code == code)
        {
            return true;
        }
    }
    return false;
}

void require_crud_contract_failure(
    statespec::BindingLanguage language,
    const statespec::IrSystem& system
)
{
    statespec::DiagnosticBag diagnostics;
    const auto result = generate_language_bindings(language, system, diagnostics);
    require(result.files.empty(), "invalid CRUD handler metadata should not emit files");
    require(
        diagnostics_have_code(diagnostics, CrudContractDiagnostic),
        "invalid CRUD handler metadata should emit SSPEC5105"
    );
}

void require_all_binding_generators_reject_crud_contract(const statespec::IrSystem& system)
{
    require_crud_contract_failure(statespec::BindingLanguage::Cpp, system);
    require_crud_contract_failure(statespec::BindingLanguage::Go, system);
    require_crud_contract_failure(statespec::BindingLanguage::Java, system);
    require_crud_contract_failure(statespec::BindingLanguage::Rust, system);
}

void test_binding_generators_reject_partial_crud_metadata()
{
    auto system = crud_contract_system();
    system.apis.push_back(
        statespec::IrApi{
            "CreateAccount",
            std::string{"POST"},
            std::string{"/v1/tenants/{tenant_id}/accounts/{account_id}"},
            std::string{"CreateAccountRequest"},
            std::string{"AccountResponse"},
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::string{"Account"},
            std::nullopt,
            {},
        }
    );

    require_all_binding_generators_reject_crud_contract(system);
}

void test_binding_generators_reject_missing_crud_shapes()
{
    auto system = crud_contract_system();
    system.apis.push_back(
        statespec::IrApi{
            "CreateAccount",
            std::string{"POST"},
            std::string{"/v1/tenants/{tenant_id}/accounts/{account_id}"},
            std::string{"MissingCreateAccountRequest"},
            std::string{"AccountResponse"},
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::string{"Account"},
            std::string{"create"},
            {},
        }
    );

    require_all_binding_generators_reject_crud_contract(system);
}

void test_binding_generators_reject_list_selector_without_route_parameters()
{
    auto system = crud_contract_system();
    system.apis.push_back(
        statespec::IrApi{
            "ListAccountsByStatus",
            std::string{"GET"},
            std::string{"/v1/tenants/{tenant_id}/accounts/by-status"},
            std::nullopt,
            std::string{"ListAccountsByStatusResponse"},
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::string{"Account"},
            std::string{"list"},
            {"tenant_id", "status"},
        }
    );

    require_all_binding_generators_reject_crud_contract(system);
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

TEST_CASE("binding generators reject partial CRUD handler metadata")
{
    test_binding_generators_reject_partial_crud_metadata();
}

TEST_CASE("binding generators reject missing CRUD request and response shapes")
{
    test_binding_generators_reject_missing_crud_shapes();
}

TEST_CASE("binding generators reject list selectors without route parameters")
{
    test_binding_generators_reject_list_selector_without_route_parameters();
}
