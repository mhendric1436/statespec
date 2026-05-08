#pragma once

#include "statespec/ast.hpp"
#include "statespec/diagnostic.hpp"

#include <optional>
#include <string>
#include <vector>

namespace statespec
{

struct GeneratedFile
{
    std::string path;
    std::string content;
};

struct GenerationOptions
{
    bool dry_run = true;
    std::optional<std::string> target_override;
    std::optional<std::string> out_override;
};

struct GenerationResult
{
    std::vector<GeneratedFile> files;
};

class Generator
{
  public:
    GenerationResult generate(
        const Spec& spec,
        const GenerationOptions& options,
        DiagnosticBag& diagnostics
    ) const;

  private:
    void generate_target(
        const SystemDecl& system,
        const GenerateDecl& declaration,
        const GenerationOptions& options,
        GenerationResult& result,
        DiagnosticBag& diagnostics
    ) const;
};

} // namespace statespec
