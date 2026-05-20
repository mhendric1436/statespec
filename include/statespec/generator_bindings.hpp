#pragma once

#include "statespec/ast.hpp"
#include "statespec/binding_language.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/generator.hpp"
#include "statespec/ir.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace statespec
{

enum class BindingGenerationTier
{
    All,
    Common,
    Api,
    Worker,
};

struct BindingGeneratorOptions
{
    BindingLanguage language;
    std::filesystem::path output_dir;
    BindingGenerationTier tier = BindingGenerationTier::All;
    std::filesystem::path template_root;
};

enum class BindingAppArtifactKind
{
    MemoryBackend,
    MemoryTransaction,
    MemoryFeatureFlagStore,
    MemoryQueueStore,
    MemoryLeaseStore,
    MemoryWorkflowStore,
    MemoryLogSink,
    MemoryMetricSink,
    ApiApplication,
    ApiServer,
    ApiDispatcher,
    ApiHandlerRegistry,
    ApiMain,
    WorkerApplication,
    WorkerRuntime,
    WorkerRegistry,
    WorkflowRunner,
    WorkflowStepHandlers,
    WorkerMain,
};

struct BindingAppArtifactModel
{
    std::string artifact_path;
    GeneratedArtifactTier tier = GeneratedArtifactTier::Common;
    BindingAppArtifactKind kind = BindingAppArtifactKind::ApiApplication;
    std::string description;
    bool generated = false;
};

BindingGenerationTier parse_binding_generation_tier(const std::string& value);

std::string binding_generation_tier_name(BindingGenerationTier tier);

std::string supported_binding_generation_tiers_text();

std::string binding_app_artifact_kind_name(BindingAppArtifactKind kind);

std::vector<BindingAppArtifactModel> binding_app_artifact_model(BindingLanguage language);

std::filesystem::path default_binding_template_root(BindingLanguage language);

std::filesystem::path resolve_binding_template_root(const BindingGeneratorOptions& options);

enum class FieldDescriptorType
{
    String,
    Bool,
    Int,
    Int32,
    Int64,
    Long,
    Double,
    Decimal,
    Json,
    Timestamp,
    Duration,
    Uuid,
    Named,
    List,
    Set,
    Map,
    Optional,
    Reference,
};

FieldDescriptorType classify_field_descriptor_type(const std::string& type_name);

std::string field_descriptor_type_name(FieldDescriptorType type);

GenerationResult generate_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

GenerationResult generate_cpp_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

GenerationResult generate_go_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

GenerationResult generate_java_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

GenerationResult generate_rust_bindings(
    const IrSystem& system,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

} // namespace statespec
