#include "binding_test_support.hpp"

namespace
{

void test_binding_generation_tier_parse()
{
    require_tier_parse("all", statespec::BindingGenerationTier::All, "all");
    require_tier_parse("common", statespec::BindingGenerationTier::Common, "common");
    require_tier_parse("api", statespec::BindingGenerationTier::Api, "api");
    require_tier_parse("worker", statespec::BindingGenerationTier::Worker, "worker");
    require_tier_parse(" api ", statespec::BindingGenerationTier::Api, "api");
    require_string_equal(
        statespec::supported_binding_generation_tiers_text(), "all|common|api|worker",
        "supported binding generation tiers text"
    );
}

void test_invalid_binding_generation_tier_throws()
{
    bool threw = false;
    try
    {
        (void)statespec::parse_binding_generation_tier("database");
    }
    catch (const std::invalid_argument& ex)
    {
        threw = true;
        const std::string message = ex.what();
        require(
            message.find("database") != std::string::npos,
            "invalid binding generation tier error should include rejected value"
        );
        require(
            message.find("all|common|api|worker") != std::string::npos,
            "invalid binding generation tier error should include supported tiers"
        );
    }
    catch (...)
    {
        require(false, "invalid binding generation tier should throw invalid_argument");
    }

    require(threw, "invalid binding generation tier should throw");
}

void test_binding_generation_tier_selection()
{
    require_tier_selection(statespec::BindingLanguage::Cpp, "cpp");
    require_tier_selection(statespec::BindingLanguage::Go, "go");
    require_tier_selection(statespec::BindingLanguage::Java, "java");
    require_tier_selection(statespec::BindingLanguage::Rust, "rust");
}

void test_rust_lib_rs_matches_selected_tier()
{
    statespec::DiagnosticBag diagnostics;

    const auto common_result = generate_for_tier(
        statespec::BindingLanguage::Rust, statespec::BindingGenerationTier::Common, "rust",
        diagnostics
    );
    const auto common_lib = generated_file_content(common_result, "lib.rs");
    require(
        common_lib.find("pub mod descriptors;") != std::string::npos,
        "common lib declares descriptors"
    );
    require(
        common_lib.find("pub mod api_descriptors;") == std::string::npos,
        "common lib excludes API descriptor module"
    );
    require(
        common_lib.find("pub mod worker_descriptors;") == std::string::npos,
        "common lib excludes worker module"
    );
    require(
        common_lib.find("pub mod runtime_feature_flags;") == std::string::npos,
        "common lib excludes unused feature flag runtime module"
    );
    require(
        common_lib.find("pub mod runtime_queues;") == std::string::npos,
        "common lib excludes unused queue runtime module"
    );
    require(
        common_lib.find("pub mod runtime_workflows;") == std::string::npos,
        "common lib excludes unused workflow runtime module"
    );
    require(
        common_lib.find("pub mod runtime_logs;") == std::string::npos,
        "common lib excludes unused log runtime module"
    );
    require(
        common_lib.find("pub mod runtime_metrics;") == std::string::npos,
        "common lib excludes unused metric runtime module"
    );

    const auto api_result = generate_for_tier(
        statespec::BindingLanguage::Rust, statespec::BindingGenerationTier::Api, "rust", diagnostics
    );
    const auto api_lib = generated_file_content(api_result, "lib.rs");
    require(
        api_lib.find("pub mod api_descriptors;") != std::string::npos,
        "API lib declares API descriptor module"
    );
    require(
        api_lib.find("pub mod api_handlers;") != std::string::npos,
        "API lib declares API handler module"
    );
    require(
        api_lib.find("pub mod api_dispatcher;") == std::string::npos,
        "API lib excludes API dispatcher module when no api_server is declared"
    );
    require(
        api_lib.find("pub mod api_routes;") == std::string::npos,
        "API lib excludes API route module when no api_server is declared"
    );
    require(
        api_lib.find("pub mod api_server;") == std::string::npos,
        "API lib excludes API server module when no api_server is declared"
    );
    require(
        api_lib.find("pub mod api_transport;") == std::string::npos,
        "API lib excludes API transport module when no api_server is declared"
    );
    require(
        api_lib.find("pub mod external_system_operator_metadata_api;") != std::string::npos,
        "API lib declares external-system operator metadata API module"
    );

    const auto worker_result = generate_for_tier(
        statespec::BindingLanguage::Rust, statespec::BindingGenerationTier::Worker, "rust",
        diagnostics
    );
    const auto worker_lib = generated_file_content(worker_result, "lib.rs");
    require(
        worker_lib.find("pub mod api_descriptors;") == std::string::npos,
        "worker lib excludes API module"
    );
    require(
        worker_lib.find("pub mod worker_descriptors;") == std::string::npos,
        "worker lib excludes worker descriptor module when no workers or workflows are declared"
    );
    require(
        worker_lib.find("pub mod worker_contexts;") == std::string::npos,
        "worker lib excludes worker context module when no workers or workflows are declared"
    );
    require(
        worker_lib.find("pub mod worker_registry;") == std::string::npos,
        "worker lib excludes worker registry module when no workers or workflows are declared"
    );
    require(
        worker_lib.find("pub mod worker_application;") == std::string::npos,
        "worker lib excludes worker application module when no worker is declared"
    );
    require(
        worker_lib.find("pub mod workflow_step_handlers;") == std::string::npos,
        "worker lib excludes workflow step handler module when no worker or workflow is declared"
    );
    require(
        worker_lib.find("pub mod workflow_runner;") == std::string::npos,
        "worker lib excludes workflow runner module when no worker or workflow is declared"
    );
    require(
        worker_lib.find("pub mod worker_workflows;") == std::string::npos,
        "worker lib excludes worker workflow module when workflows are unused"
    );

    require(!diagnostics.has_errors(), "rust tier-aware lib generation should not fail");
}

void test_cpp_makefile_matches_selected_tier()
{
    statespec::DiagnosticBag diagnostics;

    const auto common_result = generate_for_tier(
        statespec::BindingLanguage::Cpp, statespec::BindingGenerationTier::Common, "cpp",
        diagnostics
    );
    const auto common_makefile = generated_file_content(common_result, "Makefile");
    require(
        common_makefile.find("CXX ?= clang++") != std::string::npos,
        "common Makefile uses clang++ by default"
    );
    require(
        common_makefile.find("check-common") != std::string::npos,
        "common Makefile declares common check"
    );
    require(
        common_makefile.find("build-common") != std::string::npos,
        "common Makefile declares common build target"
    );
    require(
        common_makefile.find("package-common") != std::string::npos,
        "common Makefile declares common package target"
    );
    require(
        common_makefile.find("api/api_descriptors.hpp") == std::string::npos,
        "common Makefile excludes API header"
    );
    require(
        common_makefile.find("worker/worker_descriptors.hpp") == std::string::npos,
        "common Makefile excludes worker header"
    );

    const auto api_result = generate_for_tier(
        statespec::BindingLanguage::Cpp, statespec::BindingGenerationTier::Api, "cpp", diagnostics
    );
    const auto api_makefile = generated_file_content(api_result, "Makefile");
    require(
        api_makefile.find("api/api_descriptors.hpp") != std::string::npos,
        "API Makefile includes API descriptors header"
    );
    require(
        api_makefile.find("api/api_handlers.hpp") != std::string::npos,
        "API Makefile includes API handlers header"
    );
    require(
        api_makefile.find("api/api_dispatcher.hpp") == std::string::npos,
        "API Makefile excludes API dispatcher header when no api_server is declared"
    );
    require(
        api_makefile.find("api/api_routes.hpp") == std::string::npos,
        "API Makefile excludes API routes header when no api_server is declared"
    );
    require(
        api_makefile.find("api/api_server.hpp") == std::string::npos,
        "API Makefile excludes API server header when no api_server is declared"
    );
    require(
        api_makefile.find("api/api_transport.hpp") == std::string::npos,
        "API Makefile excludes API transport header when no api_server is declared"
    );
    require(
        api_makefile.find("build-api") != std::string::npos,
        "API Makefile includes API build target"
    );
    require(
        api_makefile.find("package-api") != std::string::npos,
        "API Makefile includes API package target"
    );
    require(
        api_makefile.find("worker/worker_descriptors.hpp") == std::string::npos,
        "API Makefile excludes worker header"
    );

    const auto worker_result = generate_for_tier(
        statespec::BindingLanguage::Cpp, statespec::BindingGenerationTier::Worker, "cpp",
        diagnostics
    );
    const auto worker_makefile = generated_file_content(worker_result, "Makefile");
    require(
        worker_makefile.find("api/api_descriptors.hpp") == std::string::npos,
        "worker Makefile excludes API header"
    );
    require(
        worker_makefile.find("worker/worker_descriptors.hpp") == std::string::npos,
        "worker Makefile excludes worker descriptors header when no workers or workflows are declared"
    );
    require(
        worker_makefile.find("worker/worker_registry.hpp") == std::string::npos,
        "worker Makefile excludes worker registry header when no workers or workflows are declared"
    );
    require(
        worker_makefile.find("worker/worker_application.hpp") == std::string::npos,
        "worker Makefile excludes worker application header when no worker is declared"
    );
    require(
        worker_makefile.find("worker/workflow_step_handlers.hpp") == std::string::npos,
        "worker Makefile excludes workflow step handlers header when no worker or workflow is "
        "declared"
    );
    require(
        worker_makefile.find("worker/workflow_runner.hpp") == std::string::npos,
        "worker Makefile excludes workflow runner header when no worker or workflow is declared"
    );
    require(
        worker_makefile.find("worker/worker_workflows.hpp") == std::string::npos,
        "worker Makefile excludes worker workflows header when workflows are unused"
    );
    require(
        worker_makefile.find("build-worker") == std::string::npos,
        "worker Makefile excludes worker build target when no workers or workflows are declared"
    );
    require(
        worker_makefile.find("package-worker") == std::string::npos,
        "worker Makefile excludes worker package target when no workers or workflows are declared"
    );

    require(!diagnostics.has_errors(), "cpp tier-aware Makefile generation should not fail");
}

} // namespace

TEST_CASE("binding generation tier parses canonical names")
{
    test_binding_generation_tier_parse();
}

TEST_CASE("binding generation tier rejects unsupported values")
{
    test_invalid_binding_generation_tier_throws();
}

TEST_CASE("binding generators filter artifacts by selected tier")
{
    test_binding_generation_tier_selection();
}

TEST_CASE("rust package module declarations follow selected tier")
{
    test_rust_lib_rs_matches_selected_tier();
}

TEST_CASE("cpp package Makefile declarations follow selected tier")
{
    test_cpp_makefile_matches_selected_tier();
}
