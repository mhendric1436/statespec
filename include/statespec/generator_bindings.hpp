#pragma once

#include "statespec/ast.hpp"
#include "statespec/binding_language.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/generator.hpp"

#include <filesystem>

namespace statespec
{

struct BindingGeneratorOptions
{
    BindingLanguage language;
    std::filesystem::path output_dir;
};

GenerationResult generate_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

GenerationResult generate_cpp_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

GenerationResult generate_go_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

GenerationResult generate_java_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

GenerationResult generate_rust_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

} // namespace statespec
