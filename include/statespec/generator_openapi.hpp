#pragma once

#include "statespec/ast.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/generator.hpp"
#include "statespec/ir.hpp"

#include <filesystem>

namespace statespec
{

struct OpenApiGeneratorOptions
{
    std::filesystem::path output_dir;
};

GenerationResult generate_openapi(
    const Spec& spec,
    const OpenApiGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

GenerationResult generate_openapi(
    const IrSystem& system,
    const OpenApiGeneratorOptions& options,
    DiagnosticBag& diagnostics
);

} // namespace statespec
