#include "catch2/catch_amalgamated.hpp"
#include "statespec/binding_language.hpp"
#include "statespec/generator_bindings.hpp"

#include <algorithm>
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

void test_binding_template_root_resolution()
{
    require_string_equal(
        statespec::default_binding_template_root(statespec::BindingLanguage::Cpp).generic_string(),
        "bindings/cpp", "default C++ template root"
    );
    require_string_equal(
        statespec::default_binding_template_root(statespec::BindingLanguage::Go).generic_string(),
        "bindings/go", "default Go template root"
    );
    require_string_equal(
        statespec::default_binding_template_root(statespec::BindingLanguage::Java).generic_string(),
        "bindings/java", "default Java template root"
    );
    require_string_equal(
        statespec::default_binding_template_root(statespec::BindingLanguage::Rust).generic_string(),
        "bindings/rust", "default Rust template root"
    );

    const statespec::BindingGeneratorOptions options{
        statespec::BindingLanguage::Go,
        std::filesystem::path{"/tmp/statespec-template-root-test"},
        statespec::BindingGenerationTier::All,
        std::filesystem::path{"templates/bindings"},
    };
    require_string_equal(
        statespec::resolve_binding_template_root(options).generic_string(), "templates/bindings/go",
        "explicit binding template root should be language-scoped"
    );
}

struct ExpectedBindingAppArtifact
{
    std::string path;
    statespec::GeneratedArtifactTier tier;
    statespec::BindingAppArtifactKind kind;
    bool generated = false;
};

std::vector<std::string>
sorted_app_artifact_paths(const std::vector<statespec::BindingAppArtifactModel>& artifacts)
{
    std::vector<std::string> paths;
    for (const auto& artifact : artifacts)
    {
        paths.push_back(artifact.artifact_path);
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

std::vector<std::string>
sorted_expected_app_artifact_paths(const std::vector<ExpectedBindingAppArtifact>& expected)
{
    std::vector<std::string> paths;
    for (const auto& artifact : expected)
    {
        paths.push_back(artifact.path);
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

void require_app_artifact_model(
    const std::vector<statespec::BindingAppArtifactModel>& artifacts,
    const ExpectedBindingAppArtifact& expected
)
{
    for (const auto& artifact : artifacts)
    {
        if (artifact.artifact_path == expected.path)
        {
            require(
                artifact.tier == expected.tier,
                "app artifact tier for " + expected.path + " should match"
            );
            require(
                artifact.kind == expected.kind,
                "app artifact kind for " + expected.path + " should match"
            );
            require(
                artifact.generated == expected.generated,
                "app artifact " + expected.path + " generated flag should match"
            );
            require(
                !artifact.description.empty(),
                "app artifact " + expected.path + " should describe its responsibility"
            );
            return;
        }
    }
    require(false, "expected app artifact model entry " + expected.path);
}

void require_exact_app_artifact_model(
    statespec::BindingLanguage language,
    const std::string& language_name,
    const std::vector<ExpectedBindingAppArtifact>& expected
)
{
    const auto artifacts = statespec::binding_app_artifact_model(language);
    for (const auto& artifact : expected)
    {
        require_app_artifact_model(artifacts, artifact);
    }

    const auto actual_paths = sorted_app_artifact_paths(artifacts);
    const auto expected_paths = sorted_expected_app_artifact_paths(expected);
    require(
        actual_paths == expected_paths,
        language_name + " app artifact model should match expected filenames"
    );
}

void test_binding_app_artifact_kind_names()
{
    require_string_equal(
        statespec::binding_app_artifact_kind_name(statespec::BindingAppArtifactKind::MemoryBackend),
        "memory_backend", "memory backend artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(
            statespec::BindingAppArtifactKind::MemoryTransaction
        ),
        "memory_transaction", "memory transaction artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(statespec::BindingAppArtifactKind::MemoryCodec),
        "memory_codec", "memory codec artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(
            statespec::BindingAppArtifactKind::ApiApplication
        ),
        "api_application", "API application artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(statespec::BindingAppArtifactKind::ApiServer),
        "api_server", "API server artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(
            statespec::BindingAppArtifactKind::WorkerApplication
        ),
        "worker_application", "worker application artifact kind name"
    );
    require_string_equal(
        statespec::binding_app_artifact_kind_name(statespec::BindingAppArtifactKind::WorkerMain),
        "worker_main", "worker main artifact kind name"
    );
}

void test_binding_app_artifact_models_define_application_filenames()
{
    const auto common = statespec::GeneratedArtifactTier::Common;
    const auto api = statespec::GeneratedArtifactTier::Api;
    const auto worker = statespec::GeneratedArtifactTier::Worker;
    using Kind = statespec::BindingAppArtifactKind;

    require_exact_app_artifact_model(
        statespec::BindingLanguage::Cpp, "cpp",
        {
            {"common/memory/backend.hpp", common, Kind::MemoryBackend},
            {"common/memory/transaction.hpp", common, Kind::MemoryTransaction},
            {"common/memory/codec.hpp", common, Kind::MemoryCodec},
            {"common/memory/feature_flag_store.hpp", common, Kind::MemoryFeatureFlagStore},
            {"common/memory/queue_store.hpp", common, Kind::MemoryQueueStore},
            {"common/memory/lease_store.hpp", common, Kind::MemoryLeaseStore},
            {"common/memory/workflow_store.hpp", common, Kind::MemoryWorkflowStore},
            {"common/memory/log_sink.hpp", common, Kind::MemoryLogSink},
            {"common/memory/metric_sink.hpp", common, Kind::MemoryMetricSink},
            {"api/api_application.hpp", api, Kind::ApiApplication},
            {"api/api_server.hpp", api, Kind::ApiServer, true},
            {"api/api_dispatcher.hpp", api, Kind::ApiDispatcher, true},
            {"api/api_handler_registry.hpp", api, Kind::ApiHandlerRegistry},
            {"api/main.cpp", api, Kind::ApiMain},
            {"worker/worker_application.hpp", worker, Kind::WorkerApplication, true},
            {"worker/worker_runtime.hpp", worker, Kind::WorkerRuntime},
            {"worker/worker_registry.hpp", worker, Kind::WorkerRegistry, true},
            {"worker/workflow_runner.hpp", worker, Kind::WorkflowRunner, true},
            {"worker/workflow_step_handlers.hpp", worker, Kind::WorkflowStepHandlers, true},
            {"worker/main.cpp", worker, Kind::WorkerMain},
        }
    );

    require_exact_app_artifact_model(
        statespec::BindingLanguage::Go, "go",
        {
            {"common/backend/memory/backend.go", common, Kind::MemoryBackend},
            {"common/backend/memory/transaction.go", common, Kind::MemoryTransaction},
            {"common/backend/memory/codec.go", common, Kind::MemoryCodec},
            {"common/backend/memory/feature_flags.go", common, Kind::MemoryFeatureFlagStore},
            {"common/backend/memory/queues.go", common, Kind::MemoryQueueStore},
            {"common/backend/memory/leases.go", common, Kind::MemoryLeaseStore},
            {"common/backend/memory/workflows.go", common, Kind::MemoryWorkflowStore},
            {"common/backend/memory/logs.go", common, Kind::MemoryLogSink},
            {"common/backend/memory/metrics.go", common, Kind::MemoryMetricSink},
            {"api/backend/api_application.go", api, Kind::ApiApplication},
            {"api/backend/api_server.go", api, Kind::ApiServer, true},
            {"api/backend/api_dispatcher.go", api, Kind::ApiDispatcher, true},
            {"api/backend/api_handler_registry.go", api, Kind::ApiHandlerRegistry},
            {"api/cmd/api/main.go", api, Kind::ApiMain},
            {"worker/backend/worker_application.go", worker, Kind::WorkerApplication, true},
            {"worker/backend/worker_runtime.go", worker, Kind::WorkerRuntime},
            {"worker/backend/worker_registry.go", worker, Kind::WorkerRegistry, true},
            {"worker/backend/workflow_runner.go", worker, Kind::WorkflowRunner, true},
            {"worker/backend/workflow_step_handlers.go", worker, Kind::WorkflowStepHandlers, true},
            {"worker/cmd/worker/main.go", worker, Kind::WorkerMain},
        }
    );

    require_exact_app_artifact_model(
        statespec::BindingLanguage::Java, "java",
        {
            {"common/com/statespec/backend/memory/InMemoryBackend.java", common,
             Kind::MemoryBackend},
            {"common/com/statespec/backend/memory/InMemoryTransaction.java", common,
             Kind::MemoryTransaction},
            {"common/com/statespec/backend/memory/InMemoryCodec.java", common, Kind::MemoryCodec},
            {"common/com/statespec/backend/memory/InMemoryFeatureFlagStore.java", common,
             Kind::MemoryFeatureFlagStore},
            {"common/com/statespec/backend/memory/InMemoryQueueStore.java", common,
             Kind::MemoryQueueStore},
            {"common/com/statespec/backend/memory/InMemoryLeaseStore.java", common,
             Kind::MemoryLeaseStore},
            {"common/com/statespec/backend/memory/InMemoryWorkflowStore.java", common,
             Kind::MemoryWorkflowStore},
            {"common/com/statespec/backend/memory/InMemoryLogSink.java", common,
             Kind::MemoryLogSink},
            {"common/com/statespec/backend/memory/InMemoryMetricSink.java", common,
             Kind::MemoryMetricSink},
            {"api/com/statespec/generated/ApiApplication.java", api, Kind::ApiApplication},
            {"api/com/statespec/generated/ApiServer.java", api, Kind::ApiServer, true},
            {"api/com/statespec/generated/ApiDispatcher.java", api, Kind::ApiDispatcher, true},
            {"api/com/statespec/generated/ApiHandlerRegistry.java", api, Kind::ApiHandlerRegistry},
            {"api/com/statespec/generated/ApiMain.java", api, Kind::ApiMain},
            {"worker/com/statespec/generated/WorkerApplication.java", worker,
             Kind::WorkerApplication, true},
            {"worker/com/statespec/generated/WorkerRuntime.java", worker, Kind::WorkerRuntime},
            {"worker/com/statespec/generated/WorkerRegistry.java", worker, Kind::WorkerRegistry,
             true},
            {"worker/com/statespec/generated/WorkflowRunner.java", worker, Kind::WorkflowRunner,
             true},
            {"worker/com/statespec/generated/WorkflowStepHandlers.java", worker,
             Kind::WorkflowStepHandlers, true},
            {"worker/com/statespec/generated/WorkerMain.java", worker, Kind::WorkerMain},
        }
    );

    require_exact_app_artifact_model(
        statespec::BindingLanguage::Rust, "rust",
        {
            {"common/memory/backend.rs", common, Kind::MemoryBackend},
            {"common/memory/transaction.rs", common, Kind::MemoryTransaction},
            {"common/memory/codec.rs", common, Kind::MemoryCodec},
            {"common/memory/feature_flags.rs", common, Kind::MemoryFeatureFlagStore},
            {"common/memory/queues.rs", common, Kind::MemoryQueueStore},
            {"common/memory/leases.rs", common, Kind::MemoryLeaseStore},
            {"common/memory/workflows.rs", common, Kind::MemoryWorkflowStore},
            {"common/memory/logs.rs", common, Kind::MemoryLogSink},
            {"common/memory/metrics.rs", common, Kind::MemoryMetricSink},
            {"api/api_application.rs", api, Kind::ApiApplication},
            {"api/api_server.rs", api, Kind::ApiServer, true},
            {"api/api_dispatcher.rs", api, Kind::ApiDispatcher, true},
            {"api/api_handler_registry.rs", api, Kind::ApiHandlerRegistry},
            {"api/main.rs", api, Kind::ApiMain},
            {"worker/worker_application.rs", worker, Kind::WorkerApplication, true},
            {"worker/worker_runtime.rs", worker, Kind::WorkerRuntime},
            {"worker/worker_registry.rs", worker, Kind::WorkerRegistry, true},
            {"worker/workflow_runner.rs", worker, Kind::WorkflowRunner, true},
            {"worker/workflow_step_handlers.rs", worker, Kind::WorkflowStepHandlers, true},
            {"worker/main.rs", worker, Kind::WorkerMain},
        }
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
            {},
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

struct ExpectedGeneratedArtifact
{
    std::string path;
    statespec::GeneratedArtifactTier tier;
};

std::vector<std::string> sorted_artifact_paths(const statespec::GenerationResult& result)
{
    std::vector<std::string> paths;
    for (const auto& file : result.files)
    {
        paths.push_back(file.artifact_path);
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

std::vector<std::string>
sorted_expected_artifact_paths(const std::vector<ExpectedGeneratedArtifact>& expected)
{
    std::vector<std::string> paths;
    for (const auto& artifact : expected)
    {
        paths.push_back(artifact.path);
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

void require_generated_artifact(
    const statespec::GenerationResult& result,
    const ExpectedGeneratedArtifact& expected
)
{
    for (const auto& file : result.files)
    {
        if (file.artifact_path == expected.path)
        {
            require(
                file.tier == expected.tier,
                "generated artifact tier for " + expected.path + " should match"
            );
            return;
        }
    }
    require(false, "expected generated artifact " + expected.path);
}

void require_exact_generated_artifact_manifest(
    statespec::BindingLanguage language,
    const std::string& language_name,
    const std::vector<ExpectedGeneratedArtifact>& expected
)
{
    statespec::DiagnosticBag diagnostics;
    const auto result = statespec::generate_bindings(
        empty_system_spec(),
        statespec::BindingGeneratorOptions{
            language,
            std::filesystem::path{"/tmp/statespec-artifact-manifest-test"} / language_name,
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );

    require(!diagnostics.has_errors(), language_name + " manifest generation should not fail");
    for (const auto& artifact : expected)
    {
        require_generated_artifact(result, artifact);
    }

    const auto actual_paths = sorted_artifact_paths(result);
    const auto expected_paths = sorted_expected_artifact_paths(expected);
    require(
        actual_paths == expected_paths,
        language_name + " generated artifact manifest should match expected production filenames"
    );
}

void test_binding_generators_emit_meaningful_artifact_filenames()
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
            {"common/workflow.hpp", common},
            {"common/memory/backend.hpp", common},
            {"common/memory/transaction.hpp", common},
            {"common/memory/codec.hpp", common},
            {"common/memory/feature_flag_store.hpp", common},
            {"common/memory/queue_store.hpp", common},
            {"common/memory/lease_store.hpp", common},
            {"common/memory/workflow_store.hpp", common},
            {"common/memory/log_sink.hpp", common},
            {"common/memory/metric_sink.hpp", common},
            {"common/descriptors.hpp", common},
            {"common/Makefile", common},
            {"api/api_descriptors.hpp", api},
            {"api/api_handlers.hpp", api},
            {"api/api_dispatcher.hpp", api},
            {"api/api_server.hpp", api},
            {"api/api_routes.hpp", api},
            {"api/external_system_operator_metadata_api.hpp", api},
            {"worker/worker_contexts.hpp", worker},
            {"worker/worker_descriptors.hpp", worker},
            {"worker/worker_registry.hpp", worker},
            {"worker/worker_application.hpp", worker},
            {"worker/workflow_step_handlers.hpp", worker},
            {"worker/workflow_runner.hpp", worker},
            {"worker/worker_handlers.hpp", worker},
            {"worker/worker_leases.hpp", worker},
            {"worker/worker_queues.hpp", worker},
            {"worker/worker_workflows.hpp", worker},
        }
    );

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
            {"common/backend/memory/codec.go", common},
            {"common/backend/memory/feature_flags.go", common},
            {"common/backend/memory/queues.go", common},
            {"common/backend/memory/leases.go", common},
            {"common/backend/memory/workflows.go", common},
            {"common/backend/memory/logs.go", common},
            {"common/backend/memory/metrics.go", common},
            {"common/backend/metric.go", common},
            {"common/backend/queue.go", common},
            {"common/backend/workflow.go", common},
            {"common/backend/descriptors.go", common},
            {"common/go.mod", common},
            {"common/Makefile", common},
            {"api/backend/api_descriptors.go", api},
            {"api/backend/api_handlers.go", api},
            {"api/backend/api_dispatcher.go", api},
            {"api/backend/api_server.go", api},
            {"api/backend/api_routes.go", api},
            {"api/backend/external_system_operator_metadata_api.go", api},
            {"worker/backend/worker_contexts.go", worker},
            {"worker/backend/worker_descriptors.go", worker},
            {"worker/backend/worker_registry.go", worker},
            {"worker/backend/worker_application.go", worker},
            {"worker/backend/workflow_step_handlers.go", worker},
            {"worker/backend/workflow_runner.go", worker},
            {"worker/backend/worker_handlers.go", worker},
            {"worker/backend/worker_leases.go", worker},
            {"worker/backend/worker_queues.go", worker},
            {"worker/backend/worker_workflows.go", worker},
        }
    );

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
            {"common/com/statespec/backend/Workflow.java", common},
            {"common/com/statespec/backend/memory/InMemoryBackend.java", common},
            {"common/com/statespec/backend/memory/InMemoryTransaction.java", common},
            {"common/com/statespec/backend/memory/InMemoryCodec.java", common},
            {"common/com/statespec/backend/memory/InMemoryFeatureFlagStore.java", common},
            {"common/com/statespec/backend/memory/InMemoryQueueStore.java", common},
            {"common/com/statespec/backend/memory/InMemoryLeaseStore.java", common},
            {"common/com/statespec/backend/memory/InMemoryWorkflowStore.java", common},
            {"common/com/statespec/backend/memory/InMemoryLogSink.java", common},
            {"common/com/statespec/backend/memory/InMemoryMetricSink.java", common},
            {"common/com/statespec/generated/Descriptors.java", common},
            {"common/Makefile", common},
            {"api/com/statespec/generated/ApiDescriptors.java", api},
            {"api/com/statespec/generated/ApiHandlers.java", api},
            {"api/com/statespec/generated/ApiDispatcher.java", api},
            {"api/com/statespec/generated/ApiServer.java", api},
            {"api/com/statespec/generated/ApiRoutes.java", api},
            {"api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java", api},
            {"worker/com/statespec/generated/WorkerContexts.java", worker},
            {"worker/com/statespec/generated/WorkerDescriptors.java", worker},
            {"worker/com/statespec/generated/WorkerRegistry.java", worker},
            {"worker/com/statespec/generated/WorkerApplication.java", worker},
            {"worker/com/statespec/generated/WorkflowStepHandlers.java", worker},
            {"worker/com/statespec/generated/WorkflowRunner.java", worker},
            {"worker/com/statespec/generated/WorkerHandlers.java", worker},
            {"worker/com/statespec/generated/WorkerLeases.java", worker},
            {"worker/com/statespec/generated/WorkerQueues.java", worker},
            {"worker/com/statespec/generated/WorkerWorkflows.java", worker},
        }
    );

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
            {"common/memory/codec.rs", common},
            {"common/memory/feature_flags.rs", common},
            {"common/memory/queues.rs", common},
            {"common/memory/leases.rs", common},
            {"common/memory/workflows.rs", common},
            {"common/memory/logs.rs", common},
            {"common/memory/metrics.rs", common},
            {"common/descriptors.rs", common},
            {"common/Cargo.toml", common},
            {"common/lib.rs", common},
            {"common/Makefile", common},
            {"api/api_descriptors.rs", api},
            {"api/api_handlers.rs", api},
            {"api/api_dispatcher.rs", api},
            {"api/api_server.rs", api},
            {"api/api_routes.rs", api},
            {"api/external_system_operator_metadata_api.rs", api},
            {"worker/worker_contexts.rs", worker},
            {"worker/worker_descriptors.rs", worker},
            {"worker/worker_registry.rs", worker},
            {"worker/worker_application.rs", worker},
            {"worker/workflow_step_handlers.rs", worker},
            {"worker/workflow_runner.rs", worker},
            {"worker/worker_handlers.rs", worker},
            {"worker/worker_leases.rs", worker},
            {"worker/worker_queues.rs", worker},
            {"worker/worker_workflows.rs", worker},
        }
    );
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
            {},
        },
        diagnostics
    );
    const auto go_result = statespec::generate_bindings(
        spec,
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Go,
            "/tmp/statespec-artifact-tier-test/go",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );
    const auto java_result = statespec::generate_bindings(
        spec,
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Java,
            "/tmp/statespec-artifact-tier-test/java",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );
    const auto rust_result = statespec::generate_bindings(
        spec,
        statespec::BindingGeneratorOptions{
            statespec::BindingLanguage::Rust,
            "/tmp/statespec-artifact-tier-test/rust",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );

    require(!diagnostics.has_errors(), "descriptor artifact path generation should not fail");
    require_generated_file_artifact_path(
        cpp_result, "descriptors.hpp", "common/descriptors.hpp",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        cpp_result, "Makefile", "common/Makefile", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        go_result, "backend/descriptors.go", "common/backend/descriptors.go",
        statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        go_result, "go.mod", "common/go.mod", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        go_result, "Makefile", "common/Makefile", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        java_result, "com/statespec/generated/Descriptors.java",
        "common/com/statespec/generated/Descriptors.java", statespec::GeneratedArtifactTier::Common
    );
    require_generated_file_artifact_path(
        java_result, "Makefile", "common/Makefile", statespec::GeneratedArtifactTier::Common
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
        rust_result, "Makefile", "common/Makefile", statespec::GeneratedArtifactTier::Common
    );

    require_generated_file_artifact_path(
        cpp_result, "api/api_descriptors.hpp", "api/api_descriptors.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        cpp_result, "api/api_handlers.hpp", "api/api_handlers.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        cpp_result, "api/api_dispatcher.hpp", "api/api_dispatcher.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        cpp_result, "api/api_server.hpp", "api/api_server.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        cpp_result, "api/api_routes.hpp", "api/api_routes.hpp",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        cpp_result, "api/external_system_operator_metadata_api.hpp",
        "api/external_system_operator_metadata_api.hpp", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "api/backend/api_descriptors.go", "api/backend/api_descriptors.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "api/backend/api_handlers.go", "api/backend/api_handlers.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "api/backend/api_dispatcher.go", "api/backend/api_dispatcher.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "api/backend/api_server.go", "api/backend/api_server.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "api/backend/api_routes.go", "api/backend/api_routes.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        go_result, "api/backend/external_system_operator_metadata_api.go",
        "api/backend/external_system_operator_metadata_api.go",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "api/com/statespec/generated/ApiDescriptors.java",
        "api/com/statespec/generated/ApiDescriptors.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "api/com/statespec/generated/ApiHandlers.java",
        "api/com/statespec/generated/ApiHandlers.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "api/com/statespec/generated/ApiDispatcher.java",
        "api/com/statespec/generated/ApiDispatcher.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "api/com/statespec/generated/ApiServer.java",
        "api/com/statespec/generated/ApiServer.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "api/com/statespec/generated/ApiRoutes.java",
        "api/com/statespec/generated/ApiRoutes.java", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        java_result, "api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java",
        "api/com/statespec/generated/ExternalSystemOperatorMetadataApi.java",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api/api_descriptors.rs", "api/api_descriptors.rs",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api/api_handlers.rs", "api/api_handlers.rs",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api/api_dispatcher.rs", "api/api_dispatcher.rs",
        statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api/api_server.rs", "api/api_server.rs", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api/api_routes.rs", "api/api_routes.rs", statespec::GeneratedArtifactTier::Api
    );
    require_generated_file_artifact_path(
        rust_result, "api/external_system_operator_metadata_api.rs",
        "api/external_system_operator_metadata_api.rs", statespec::GeneratedArtifactTier::Api
    );

    require_generated_file_artifact_path(
        cpp_result, "worker/worker_descriptors.hpp", "worker/worker_descriptors.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_contexts.hpp", "worker/worker_contexts.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_registry.hpp", "worker/worker_registry.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_application.hpp", "worker/worker_application.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/workflow_step_handlers.hpp", "worker/workflow_step_handlers.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/workflow_runner.hpp", "worker/workflow_runner.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_handlers.hpp", "worker/worker_handlers.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_queues.hpp", "worker/worker_queues.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_leases.hpp", "worker/worker_leases.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        cpp_result, "worker/worker_workflows.hpp", "worker/worker_workflows.hpp",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_descriptors.go", "worker/backend/worker_descriptors.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_contexts.go", "worker/backend/worker_contexts.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_registry.go", "worker/backend/worker_registry.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_application.go", "worker/backend/worker_application.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/workflow_step_handlers.go",
        "worker/backend/workflow_step_handlers.go", statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/workflow_runner.go", "worker/backend/workflow_runner.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_handlers.go", "worker/backend/worker_handlers.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_queues.go", "worker/backend/worker_queues.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_leases.go", "worker/backend/worker_leases.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        go_result, "worker/backend/worker_workflows.go", "worker/backend/worker_workflows.go",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerDescriptors.java",
        "worker/com/statespec/generated/WorkerDescriptors.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerContexts.java",
        "worker/com/statespec/generated/WorkerContexts.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerRegistry.java",
        "worker/com/statespec/generated/WorkerRegistry.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerApplication.java",
        "worker/com/statespec/generated/WorkerApplication.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkflowStepHandlers.java",
        "worker/com/statespec/generated/WorkflowStepHandlers.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkflowRunner.java",
        "worker/com/statespec/generated/WorkflowRunner.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerHandlers.java",
        "worker/com/statespec/generated/WorkerHandlers.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerQueues.java",
        "worker/com/statespec/generated/WorkerQueues.java", statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerLeases.java",
        "worker/com/statespec/generated/WorkerLeases.java", statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        java_result, "worker/com/statespec/generated/WorkerWorkflows.java",
        "worker/com/statespec/generated/WorkerWorkflows.java",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_descriptors.rs", "worker/worker_descriptors.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_contexts.rs", "worker/worker_contexts.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_registry.rs", "worker/worker_registry.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_application.rs", "worker/worker_application.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/workflow_step_handlers.rs", "worker/workflow_step_handlers.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/workflow_runner.rs", "worker/workflow_runner.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_handlers.rs", "worker/worker_handlers.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_queues.rs", "worker/worker_queues.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_leases.rs", "worker/worker_leases.rs",
        statespec::GeneratedArtifactTier::Worker
    );
    require_generated_file_artifact_path(
        rust_result, "worker/worker_workflows.rs", "worker/worker_workflows.rs",
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
            {},
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
        common_lib.find("pub mod api_descriptors;") == std::string::npos,
        "common lib excludes API descriptor module"
    );
    require(
        common_lib.find("pub mod worker_descriptors;") == std::string::npos,
        "common lib excludes worker module"
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
        api_lib.find("pub mod api_dispatcher;") != std::string::npos,
        "API lib declares API dispatcher module"
    );
    require(
        api_lib.find("pub mod api_routes;") != std::string::npos,
        "API lib declares API route module"
    );
    require(
        api_lib.find("pub mod api_server;") != std::string::npos,
        "API lib declares API server module"
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
        worker_lib.find("pub mod worker_descriptors;") != std::string::npos,
        "worker lib declares worker descriptor module"
    );
    require(
        worker_lib.find("pub mod worker_contexts;") != std::string::npos,
        "worker lib declares worker context module"
    );
    require(
        worker_lib.find("pub mod worker_registry;") != std::string::npos,
        "worker lib declares worker registry module"
    );
    require(
        worker_lib.find("pub mod worker_application;") != std::string::npos,
        "worker lib declares worker application module"
    );
    require(
        worker_lib.find("pub mod workflow_step_handlers;") != std::string::npos,
        "worker lib declares workflow step handler module"
    );
    require(
        worker_lib.find("pub mod workflow_runner;") != std::string::npos,
        "worker lib declares workflow runner module"
    );
    require(
        worker_lib.find("pub mod worker_workflows;") != std::string::npos,
        "worker lib declares worker workflow module"
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
        api_makefile.find("api/api_dispatcher.hpp") != std::string::npos,
        "API Makefile includes API dispatcher header"
    );
    require(
        api_makefile.find("api/api_routes.hpp") != std::string::npos,
        "API Makefile includes API routes header"
    );
    require(
        api_makefile.find("api/api_server.hpp") != std::string::npos,
        "API Makefile includes API server header"
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
        worker_makefile.find("worker/worker_descriptors.hpp") != std::string::npos,
        "worker Makefile includes worker descriptors header"
    );
    require(
        worker_makefile.find("worker/worker_registry.hpp") != std::string::npos,
        "worker Makefile includes worker registry header"
    );
    require(
        worker_makefile.find("worker/worker_application.hpp") != std::string::npos,
        "worker Makefile includes worker application header"
    );
    require(
        worker_makefile.find("worker/workflow_step_handlers.hpp") != std::string::npos,
        "worker Makefile includes workflow step handlers header"
    );
    require(
        worker_makefile.find("worker/workflow_runner.hpp") != std::string::npos,
        "worker Makefile includes workflow runner header"
    );
    require(
        worker_makefile.find("worker/worker_workflows.hpp") != std::string::npos,
        "worker Makefile includes worker workflows header"
    );
    require(
        worker_makefile.find("build-worker") != std::string::npos,
        "worker Makefile includes worker build target"
    );
    require(
        worker_makefile.find("package-worker") != std::string::npos,
        "worker Makefile includes worker package target"
    );

    require(!diagnostics.has_errors(), "cpp tier-aware Makefile generation should not fail");
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

TEST_CASE("binding template roots resolve predictably")
{
    test_binding_template_root_resolution();
}

TEST_CASE("binding app artifact kind names are stable")
{
    test_binding_app_artifact_kind_names();
}

TEST_CASE("binding app artifact models define application filenames")
{
    test_binding_app_artifact_models_define_application_filenames();
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

TEST_CASE("binding generators emit meaningful production filenames")
{
    test_binding_generators_emit_meaningful_artifact_filenames();
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

TEST_CASE("cpp package Makefile declarations follow selected tier")
{
    test_cpp_makefile_matches_selected_tier();
}
