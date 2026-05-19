#pragma once

#include "statespec/ast.hpp"
#include "statespec/binding_language.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/generator.hpp"
#include "statespec/ir.hpp"

#include <filesystem>
#include <string>

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
};

BindingGenerationTier parse_binding_generation_tier(const std::string& value);

std::string binding_generation_tier_name(BindingGenerationTier tier);

std::string supported_binding_generation_tiers_text();

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
