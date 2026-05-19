#include "catch2/catch_amalgamated.hpp"
#include "statespec/binding_language.hpp"
#include "statespec/generator_bindings.hpp"

#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

void require(
    bool condition,
    const std::string& message
)
{
    INFO(message);
    REQUIRE(condition);
}

void require_equal(
    statespec::BindingLanguage actual,
    statespec::BindingLanguage expected,
    const std::string& message
)
{
    require(actual == expected, message);
}

void require_equal(
    statespec::BindingGenerationTier actual,
    statespec::BindingGenerationTier expected,
    const std::string& message
)
{
    require(actual == expected, message);
}

void require_string_equal(
    const std::string& actual,
    const std::string& expected,
    const std::string& message
)
{
    require(actual == expected, message + ": expected '" + expected + "', got '" + actual + "'");
}

void test_parse_canonical_names()
{
    require_equal(
        statespec::parse_binding_language("cpp"), statespec::BindingLanguage::Cpp,
        "cpp should parse to Cpp"
    );
    require_equal(
        statespec::parse_binding_language("go"), statespec::BindingLanguage::Go,
        "go should parse to Go"
    );
    require_equal(
        statespec::parse_binding_language("java"), statespec::BindingLanguage::Java,
        "java should parse to Java"
    );
    require_equal(
        statespec::parse_binding_language("rust"), statespec::BindingLanguage::Rust,
        "rust should parse to Rust"
    );
}

void test_parse_aliases()
{
    require_equal(
        statespec::parse_binding_language("c++"), statespec::BindingLanguage::Cpp,
        "c++ should parse to Cpp"
    );
    require_equal(
        statespec::parse_binding_language("cplusplus"), statespec::BindingLanguage::Cpp,
        "cplusplus should parse to Cpp"
    );
    require_equal(
        statespec::parse_binding_language("golang"), statespec::BindingLanguage::Go,
        "golang should parse to Go"
    );
    require_equal(
        statespec::parse_binding_language("rs"), statespec::BindingLanguage::Rust,
        "rs should parse to Rust"
    );
}

void test_parse_is_case_and_separator_tolerant()
{
    require_equal(
        statespec::parse_binding_language(" Cpp "), statespec::BindingLanguage::Cpp,
        "Cpp with whitespace should parse to Cpp"
    );
    require_equal(
        statespec::parse_binding_language("GO"), statespec::BindingLanguage::Go,
        "GO should parse to Go"
    );
    require_equal(
        statespec::parse_binding_language("JaVa"), statespec::BindingLanguage::Java,
        "JaVa should parse to Java"
    );
    require_equal(
        statespec::parse_binding_language("r-u_s-t"), statespec::BindingLanguage::Rust,
        "r-u_s-t should parse to Rust"
    );
}

void test_to_string()
{
    require_string_equal(statespec::to_string(statespec::BindingLanguage::Cpp), "cpp", "Cpp text");
    require_string_equal(statespec::to_string(statespec::BindingLanguage::Go), "go", "Go text");
    require_string_equal(
        statespec::to_string(statespec::BindingLanguage::Java), "java", "Java text"
    );
    require_string_equal(
        statespec::to_string(statespec::BindingLanguage::Rust), "rust", "Rust text"
    );
}

void test_supported_languages()
{
    const auto languages = statespec::supported_binding_languages();
    require(languages.size() == 4, "expected four supported binding languages");
    require_string_equal(languages[0], "cpp", "supported language 0");
    require_string_equal(languages[1], "go", "supported language 1");
    require_string_equal(languages[2], "java", "supported language 2");
    require_string_equal(languages[3], "rust", "supported language 3");
    require_string_equal(
        statespec::supported_binding_languages_text(), "cpp|go|java|rust", "supported language text"
    );
}

void require_tier_parse(
    const std::string& value,
    statespec::BindingGenerationTier expected,
    const std::string& expected_name
)
{
    const auto actual = statespec::parse_binding_generation_tier(value);
    require_equal(actual, expected, value + " should parse to expected binding generation tier");
    require_string_equal(
        statespec::binding_generation_tier_name(actual), expected_name,
        value + " binding generation tier name"
    );
}

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

void test_invalid_language_throws()
{
    bool threw = false;
    try
    {
        (void)statespec::parse_binding_language("python");
    }
    catch (const statespec::BindingLanguageError& ex)
    {
        threw = true;
        const std::string message = ex.what();
        require(
            message.find("python") != std::string::npos,
            "invalid language error should include rejected value"
        );
        require(
            message.find("cpp|go|java|rust") != std::string::npos,
            "invalid language error should include supported languages"
        );
    }
    catch (...)
    {
        require(false, "invalid language should throw BindingLanguageError");
    }

    require(threw, "invalid language should throw");
}

void require_field_type(
    const std::string& type_name,
    statespec::FieldDescriptorType expected,
    const std::string& expected_name
)
{
    const auto actual = statespec::classify_field_descriptor_type(type_name);
    require(
        actual == expected,
        "field descriptor classification for " + type_name + " should be " + expected_name
    );
    require_string_equal(
        statespec::field_descriptor_type_name(actual), expected_name,
        "field descriptor type name for " + type_name
    );
}

void test_field_descriptor_type_classification()
{
    require_field_type("string", statespec::FieldDescriptorType::String, "string");
    require_field_type("bool", statespec::FieldDescriptorType::Bool, "bool");
    require_field_type("int", statespec::FieldDescriptorType::Int, "int");
    require_field_type("int32", statespec::FieldDescriptorType::Int32, "int32");
    require_field_type("int64", statespec::FieldDescriptorType::Int64, "int64");
    require_field_type("long", statespec::FieldDescriptorType::Long, "long");
    require_field_type("double", statespec::FieldDescriptorType::Double, "double");
    require_field_type("decimal", statespec::FieldDescriptorType::Decimal, "decimal");
    require_field_type("json", statespec::FieldDescriptorType::Json, "json");
    require_field_type("timestamp", statespec::FieldDescriptorType::Timestamp, "timestamp");
    require_field_type("duration", statespec::FieldDescriptorType::Duration, "duration");
    require_field_type("uuid", statespec::FieldDescriptorType::Uuid, "uuid");
    require_field_type("OrderStatus", statespec::FieldDescriptorType::Named, "named");
    require_field_type("list<string>", statespec::FieldDescriptorType::List, "list");
    require_field_type("string[]", statespec::FieldDescriptorType::List, "list");
    require_field_type("set<uuid>", statespec::FieldDescriptorType::Set, "set");
    require_field_type("map<string,json>", statespec::FieldDescriptorType::Map, "map");
    require_field_type("optional<string>", statespec::FieldDescriptorType::Optional, "optional");
    require_field_type("OrderStatus?", statespec::FieldDescriptorType::Optional, "optional");
    require_field_type("ref<Account>", statespec::FieldDescriptorType::Reference, "reference");
    require_field_type("  uuid  ", statespec::FieldDescriptorType::Uuid, "uuid");
}

statespec::Spec empty_system_spec()
{
    statespec::Spec spec;
    statespec::SystemDecl system;
    system.name = "EmptySystem";
    spec.system = system;
    return spec;
}

void require_generated_files_have_tiered_artifact_paths(
    statespec::BindingLanguage language,
    const std::string& language_name
)
{
    statespec::DiagnosticBag diagnostics;
    const auto result = statespec::generate_bindings(
        empty_system_spec(),
        statespec::BindingGeneratorOptions{
            language,
            std::filesystem::path{"/tmp/statespec-artifact-tier-test"} / language_name,
            statespec::BindingGenerationTier::All,
        },
        diagnostics
    );

    require(!diagnostics.has_errors(), (language_name + " generation should not fail").c_str());
    require(!result.files.empty(), (language_name + " generation should emit files").c_str());
    for (const auto& file : result.files)
    {
        if (file.tier == statespec::GeneratedArtifactTier::Common)
        {
            require(
                file.artifact_path.rfind("common/", 0) == 0,
                (language_name + " common artifact should have common path: " + file.path).c_str()
            );
        }
        else if (file.tier == statespec::GeneratedArtifactTier::Api)
        {
            require(
                file.artifact_path.rfind("api/", 0) == 0,
                (language_name + " API artifact should have API path: " + file.path).c_str()
            );
        }
        else if (file.tier == statespec::GeneratedArtifactTier::Worker)
        {
            require(
                file.artifact_path.rfind("worker/", 0) == 0,
                (language_name + " worker artifact should have worker path: " + file.path).c_str()
            );
        }
    }
}

void require_generated_file_artifact_path(
    const statespec::GenerationResult& result,
    const std::string& emitted_suffix,
    const std::string& expected_artifact_path,
    statespec::GeneratedArtifactTier expected_tier
)
{
    for (const auto& file : result.files)
    {
        if (file.path.size() >= emitted_suffix.size() &&
            file.path.compare(
                file.path.size() - emitted_suffix.size(), emitted_suffix.size(), emitted_suffix
            ) == 0)
        {
            require_string_equal(
                file.artifact_path, expected_artifact_path,
                "generated file artifact path for " + emitted_suffix
            );
            require(
                file.tier == expected_tier, ("generated file tier for " + emitted_suffix).c_str()
            );
            return;
        }
    }
    require(false, ("expected generated file ending in " + emitted_suffix).c_str());
}

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

void test_shared_descriptor_artifact_paths()
{
    const auto spec = empty_system_spec();
    statespec::DiagnosticBag diagnostics;

    const auto cpp_result = statespec::generate_bindings(
        spec,
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Cpp,
            "/tmp/statespec-artifact-tier-test/cpp",
            statespec::BindingGenerationTier::All,
        },
        diagnostics
    );
    const auto go_result = statespec::generate_bindings(
        spec,
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Go,
            "/tmp/statespec-artifact-tier-test/go",
            statespec::BindingGenerationTier::All,
        },
        diagnostics
    );
    const auto java_result = statespec::generate_bindings(
        spec,
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Java,
            "/tmp/statespec-artifact-tier-test/java",
            statespec::BindingGenerationTier::All,
        },
        diagnostics
    );
    const auto rust_result = statespec::generate_bindings(
        spec,
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Rust,
            "/tmp/statespec-artifact-tier-test/rust",
            statespec::BindingGenerationTier::All,
        },
        diagnostics
    );

    require(!diagnostics.has_errors(), "descriptor artifact path generation should not fail");
    require_generated_file_artifact_path(
        cpp_result, "system_descriptors.hpp", "common/system_descriptors.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        cpp_result, "CMakeLists.txt", "common/CMakeLists.txt",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        go_result, "backend/descriptors.go", "common/backend/descriptors.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        go_result, "go.mod", "common/go.mod", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        java_result, "com/statespec/generated/Descriptors.java",
        "common/com/statespec/generated/Descriptors.java", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        java_result, "build.gradle", "common/build.gradle", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        rust_result, "descriptors.rs", "common/descriptors.rs",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        rust_result, "Cargo.toml", "common/Cargo.toml", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        rust_result, "lib.rs", "common/lib.rs", statespec::GeneratedArtifactTier::Common
    );

    require_generated_file_artifact_path(
        cpp_result, "api_artifacts.hpp", "api/api_artifacts.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "backend/api_artifacts.go", "api/backend/api_artifacts.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "com/statespec/generated/ApiArtifacts.java",
        "api/com/statespec/generated/ApiArtifacts.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api_artifacts.rs", "api/api_artifacts.rs",
        statespec::GeneratedArtifactTier::Api
    );

    require_generated_file_artifact_path(
        cpp_result, "worker_artifacts.hpp", "worker/worker_artifacts.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "backend/worker_artifacts.go", "worker/backend/worker_artifacts.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "com/statespec/generated/WorkerArtifacts.java",
        "worker/com/statespec/generated/WorkerArtifacts.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker_artifacts.rs", "worker/worker_artifacts.rs",
        statespec::GeneratedArtifactTier::Worker
    );
}

statespec::GenerationResult generate_for_tier(
    statespec::BindingLanguage language,
    statespec::BindingGenerationTier tier,
    const std::string& language_name,
    statespec::DiagnosticBag& diagnostics
)
{
    return statespec::generate_bindings(
        empty_system_spec(),
        statespec::BindingGeneratorOptions{
            language,
            std::filesystem::path{"/tmp/statespec-tier-selection-test"} / language_name /
                statespec::binding_generation_tier_name(tier),
            tier,
        },
        diagnostics
    );
}

bool result_has_tier(
    const statespec::GenerationResult& result,
    statespec::GeneratedArtifactTier tier
)
{
    for (const auto& file : result.files)
    {
        if (file.tier == tier)
        {
            return true;
        }
    }
    return false;
}

std::string generated_file_content(
    const statespec::GenerationResult& result,
    const std::string& emitted_suffix
)
{
    for (const auto& file : result.files)
    {
        if (file.path.size() >= emitted_suffix.size() &&
            file.path.compare(
                file.path.size() - emitted_suffix.size(), emitted_suffix.size(), emitted_suffix
            ) == 0)
        {
            return file.content;
        }
    }
    require(false, "expected generated file ending in " + emitted_suffix);
    return {};
}

void require_tier_selection(
    statespec::BindingLanguage language,
    const std::string& language_name
)
{
    statespec::DiagnosticBag diagnostics;

    const auto common_result = generate_for_tier(
        language, statespec::BindingGenerationTier::Common, language_name, diagnostics
    );
    require(!diagnostics.has_errors(), language_name + " common tier generation should not fail");
    require(
        result_has_tier(common_result, statespec::GeneratedArtifactTier::Common),
        language_name + " common tier should include common artifacts"
    );
    require(
        !result_has_tier(common_result, statespec::GeneratedArtifactTier::Api),
        language_name + " common tier should exclude API artifacts"
    );
    require(
        !result_has_tier(common_result, statespec::GeneratedArtifactTier::Worker),
        language_name + " common tier should exclude worker artifacts"
    );

    const auto api_result = generate_for_tier(
        language, statespec::BindingGenerationTier::Api, language_name, diagnostics
    );
    require(!diagnostics.has_errors(), language_name + " API tier generation should not fail");
    require(
        result_has_tier(api_result, statespec::GeneratedArtifactTier::Common),
        language_name + " API tier should include common artifacts"
    );
    require(
        result_has_tier(api_result, statespec::GeneratedArtifactTier::Api),
        language_name + " API tier should include API artifacts"
    );
    require(
        !result_has_tier(api_result, statespec::GeneratedArtifactTier::Worker),
        language_name + " API tier should exclude worker artifacts"
    );

    const auto worker_result = generate_for_tier(
        language, statespec::BindingGenerationTier::Worker, language_name, diagnostics
    );
    require(!diagnostics.has_errors(), language_name + " worker tier generation should not fail");
    require(
        result_has_tier(worker_result, statespec::GeneratedArtifactTier::Common),
        language_name + " worker tier should include common artifacts"
    );
    require(
        !result_has_tier(worker_result, statespec::GeneratedArtifactTier::Api),
        language_name + " worker tier should exclude API artifacts"
    );
    require(
        result_has_tier(worker_result, statespec::GeneratedArtifactTier::Worker),
        language_name + " worker tier should include worker artifacts"
    );
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
        common_lib.find("pub mod api_artifacts;") == std::string::npos,
        "common lib excludes API module"
    );
    require(
        common_lib.find("pub mod worker_artifacts;") == std::string::npos,
        "common lib excludes worker module"
    );

    const auto api_result = generate_for_tier(
        statespec::BindingLanguage::Rust, statespec::BindingGenerationTier::Api, "rust", diagnostics
    );
    const auto api_lib = generated_file_content(api_result, "lib.rs");
    require(
        api_lib.find("pub mod api_artifacts;") != std::string::npos, "API lib declares API module"
    );
    require(
        api_lib.find("pub mod worker_artifacts;") == std::string::npos,
        "API lib excludes worker module"
    );

    const auto worker_result = generate_for_tier(
        statespec::BindingLanguage::Rust, statespec::BindingGenerationTier::Worker, "rust",
        diagnostics
    );
    const auto worker_lib = generated_file_content(worker_result, "lib.rs");
    require(
        worker_lib.find("pub mod api_artifacts;") == std::string::npos,
        "worker lib excludes API module"
    );
    require(
        worker_lib.find("pub mod worker_artifacts;") != std::string::npos,
        "worker lib declares worker module"
    );

    require(!diagnostics.has_errors(), "rust tier-aware lib generation should not fail");
}

} // namespace

TEST_CASE("binding language parses canonical names")
{
    test_parse_canonical_names();
}

TEST_CASE("binding language parses aliases")
{
    test_parse_aliases();
}

TEST_CASE("binding language parsing is case and separator tolerant")
{
    test_parse_is_case_and_separator_tolerant();
}

TEST_CASE("binding language converts to strings")
{
    test_to_string();
}

TEST_CASE("binding language lists supported languages")
{
    test_supported_languages();
}

TEST_CASE("binding generation tier parses canonical names")
{
    test_binding_generation_tier_parse();
}

TEST_CASE("binding generation tier rejects unsupported values")
{
    test_invalid_binding_generation_tier_throws();
}

TEST_CASE("binding language rejects unsupported languages")
{
    test_invalid_language_throws();
}

TEST_CASE("field descriptor type classification normalizes grammar types")
{
    test_field_descriptor_type_classification();
}

TEST_CASE("generated artifact tier defaults to common")
{
    test_generated_artifact_tiers_default_to_common();
}

TEST_CASE("binding generators tag current files as common artifacts")
{
    test_binding_generators_assign_artifact_tiers();
}

TEST_CASE("binding generators model shared descriptor artifact paths")
{
    test_shared_descriptor_artifact_paths();
}

TEST_CASE("binding generators filter artifacts by selected tier")
{
    test_binding_generation_tier_selection();
}

TEST_CASE("rust package module declarations follow selected tier")
{
    test_rust_lib_rs_matches_selected_tier();
}
