#pragma once

#include "test_constants.hpp"
#include "test_support.hpp"

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

[[maybe_unused]] void require(
    bool condition,
    const std::string& message
)
{
    INFO(message);
    REQUIRE(condition);
}

[[maybe_unused]] void require_equal(
    statespec::BindingLanguage actual,
    statespec::BindingLanguage expected,
    const std::string& message
)
{
    require(actual == expected, message);
}

[[maybe_unused]] void require_equal(
    statespec::BindingGenerationTier actual,
    statespec::BindingGenerationTier expected,
    const std::string& message
)
{
    require(actual == expected, message);
}

[[maybe_unused]] void require_string_equal(
    const std::string& actual,
    const std::string& expected,
    const std::string& message
)
{
    require(actual == expected, message + ": expected '" + expected + "', got '" + actual + "'");
}

[[maybe_unused]] void require_tier_parse(
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

struct ExpectedBindingAppArtifact
{
    std::string path;
    statespec::GeneratedArtifactTier tier;
    statespec::BindingAppArtifactKind kind;
    bool generated = false;
};

[[maybe_unused]] std::vector<std::string>
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

[[maybe_unused]] std::vector<std::string>
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

[[maybe_unused]] void require_app_artifact_model(
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

[[maybe_unused]] void require_exact_app_artifact_model(
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

[[maybe_unused]] void require_field_type(
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

[[maybe_unused]] statespec::Spec empty_system_spec()
{
    statespec::Spec spec;
    statespec::SystemDecl system;
    system.name = "EmptySystem";
    spec.system = system;
    return spec;
}

[[maybe_unused]] statespec::Spec entity_api_catalog_system_spec()
{
    return statespec::test::parse_text(R"sspec(
        system CatalogSystem {
          tenant scoped_by tenant_id
          system_tenant configured

          entity Account {
            key tenant_id, account_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              tenant_id string
              account_id string
              display_name string
            }
            state_machine {
              state Active
              state Deleted {
                terminal: true
                garbage_collection {
                  after: P30D
                  mode: tombstone
                }
              }
              initial Active
              terminal [Deleted]
              Active -> Deleted
            }
            indexes {
              unique by_tenant_account on tenant_id, account_id
            }
            api {
              resource "/v1/tenants/{tenant_id}/accounts/{account_id}"
              create {
                fields [display_name]
              }
              get
            }
          }

          entity AuditLog {
            key tenant_id, audit_log_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              tenant_id string
              audit_log_id string
              description string
            }
            state_machine {
              state Active
              initial Active
            }
          }
        }
    )sspec");
}

[[maybe_unused]] statespec::GenerationResult generate_entity_api_catalog_bindings(
    statespec::BindingLanguage language,
    const std::string& language_name
)
{
    statespec::DiagnosticBag diagnostics;
    const auto result = statespec::generate_bindings(
        entity_api_catalog_system_spec(),
        statespec::BindingGeneratorOptions{
            language,
            std::filesystem::path{statespec::test::ArtifactTierTestRoot} / language_name /
                "entity-api-catalog",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );
    require(
        !diagnostics.has_errors(), language_name + " entity API catalog generation should not fail"
    );
    return result;
}

[[maybe_unused]] statespec::Spec child_workflow_system_spec()
{
    return statespec::test::parse_text(R"sspec(
        system ChildWorkflowSystem {
          entity Account {
            key account_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              account_id uuid
              desired_task_count int
            }
            state_machine {
              state Active
              initial Active
            }
          }

          entity Task {
            key task_id
            ownership {
              authority: system
              system_of_record: self
              lifecycle: authoritative
            }
            fields {
              created_at timestamp
              updated_at timestamp
              status string
              task_id uuid
              account_id uuid
            }
            state_machine {
              state Active
              state Failed
              initial Active
            }
          }

          workflow TaskLifecycle {
            version 1
            singleton false
            expected_execution_time PT1M
            start run_task
            step run_task {
              expected_execution_time PT10S
              max_retries 1
            }
          }

          workflow AccountLifecycle {
            version 1
            singleton false
            expected_execution_time PT5M
            start run_account
            child_workflow tasks {
              child_entity Task
              child_workflow TaskLifecycle
              child_id task_id uuid
              parent_ref account_id = account.account_id
              desired_count account.desired_task_count
              create {
                task_id: task_id
                account_id: account.account_id
              }
              success when Task.status == Active
              failure when Task.status == Failed
            }
            step run_account {
              expected_execution_time PT10S
              max_retries 1
            }
          }
        }
    )sspec");
}

[[maybe_unused]] statespec::GenerationResult generate_child_workflow_bindings(
    statespec::BindingLanguage language,
    const std::string& language_name
)
{
    statespec::DiagnosticBag diagnostics;
    const auto result = statespec::generate_bindings(
        child_workflow_system_spec(),
        statespec::BindingGeneratorOptions{
            language,
            std::filesystem::path{statespec::test::ArtifactTierTestRoot} / language_name /
                "child-workflow",
            statespec::BindingGenerationTier::All,
            {},
        },
        diagnostics
    );
    require(
        !diagnostics.has_errors(), language_name + " child workflow generation should not fail"
    );
    return result;
}

[[maybe_unused]] bool generated_artifact_exists(
    const statespec::GenerationResult& result,
    const std::string& artifact_path
)
{
    for (const auto& file : result.files)
    {
        if (file.artifact_path == artifact_path)
        {
            return true;
        }
    }
    return false;
}

[[maybe_unused]] void require_generated_artifact_exists(
    const statespec::GenerationResult& result,
    const std::string& artifact_path
)
{
    require(
        generated_artifact_exists(result, artifact_path),
        "expected generated artifact " + artifact_path
    );
}

[[maybe_unused]] void require_generated_artifact_not_exists(
    const statespec::GenerationResult& result,
    const std::string& artifact_path
)
{
    require(
        !generated_artifact_exists(result, artifact_path),
        "did not expect generated artifact " + artifact_path
    );
}

[[maybe_unused]] std::string generated_artifact_content(
    const statespec::GenerationResult& result,
    const std::string& artifact_path
)
{
    for (const auto& file : result.files)
    {
        if (file.artifact_path == artifact_path)
        {
            return file.content;
        }
    }
    require(false, "expected generated artifact " + artifact_path);
    return {};
}

[[maybe_unused]] void require_generated_artifact_contains(
    const statespec::GenerationResult& result,
    const std::string& artifact_path,
    const std::string& expected
)
{
    const auto content = generated_artifact_content(result, artifact_path);
    require(
        content.find(expected) != std::string::npos,
        "expected " + artifact_path + " to contain " + expected
    );
}

[[maybe_unused]] void require_generated_artifact_not_contains(
    const statespec::GenerationResult& result,
    const std::string& artifact_path,
    const std::string& unexpected
)
{
    const auto content = generated_artifact_content(result, artifact_path);
    require(
        content.find(unexpected) == std::string::npos,
        "did not expect " + artifact_path + " to contain " + unexpected
    );
}

[[maybe_unused]] void require_generated_files_have_tiered_artifact_paths(
    statespec::BindingLanguage language,
    const std::string& language_name
)
{
    statespec::DiagnosticBag diagnostics;
    const auto result = statespec::generate_bindings(
        empty_system_spec(),
        statespec::BindingGeneratorOptions{
            language,
            std::filesystem::path{statespec::test::ArtifactTierTestRoot} / language_name,
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

[[maybe_unused]] std::vector<std::string>
sorted_artifact_paths(const statespec::GenerationResult& result)
{
    std::vector<std::string> paths;
    for (const auto& file : result.files)
    {
        paths.push_back(file.artifact_path);
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

[[maybe_unused]] std::vector<std::string>
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

[[maybe_unused]] void require_generated_artifact(
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

[[maybe_unused]] void require_exact_generated_artifact_manifest(
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
            std::filesystem::path{statespec::test::ArtifactManifestTestRoot} / language_name,
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

[[maybe_unused]] void require_generated_file_artifact_path(
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

[[maybe_unused]] statespec::GenerationResult generate_for_tier(
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
            std::filesystem::path{statespec::test::TierSelectionTestRoot} / language_name /
                statespec::binding_generation_tier_name(tier),
            tier,
            {},
        },
        diagnostics
    );
}

[[maybe_unused]] bool result_has_tier(
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

[[maybe_unused]] std::string generated_file_content(
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

[[maybe_unused]] void require_tier_selection(
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
        !result_has_tier(api_result, statespec::GeneratedArtifactTier::Api),
        language_name + " API tier should exclude API artifacts when the spec has no API surface"
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
        !result_has_tier(worker_result, statespec::GeneratedArtifactTier::Worker),
        language_name + " worker tier should exclude worker artifacts when no workers or workflows "
                        "are declared"
    );
}

} // namespace
