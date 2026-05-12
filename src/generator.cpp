#include "generator_backend.hpp"

#include <unordered_set>

namespace statespec
{

namespace
{

std::vector<GenerateDecl> selected_declarations(
    const SystemDecl& system,
    const GenerationOptions& options
)
{
    std::vector<GenerateDecl> selected;
    for (const auto& declaration : system.generators)
    {
        if (!options.target_override.has_value() || declaration.target == *options.target_override)
        {
            selected.push_back(declaration);
        }
    }

    if (selected.empty() && options.target_override.has_value())
    {
        GenerateDecl declaration;
        declaration.target = *options.target_override;
        selected.push_back(declaration);
    }

    return selected;
}

} // namespace

GenerationResult Generator::generate(
    const Spec& spec,
    const GenerationOptions& options,
    DiagnosticBag& diagnostics
) const
{
    GenerationResult result;
    if (!spec.system.has_value())
    {
        diagnostics.error(SourceRange{}, "SSPEC5001", "generation requires a system declaration");
        return result;
    }

    const auto& system = *spec.system;
    const auto declarations = selected_declarations(system, options);
    if (declarations.empty())
    {
        diagnostics.error(
            system.range, "SSPEC5002", "generation requires at least one generate declaration"
        );
        return result;
    }

    for (const auto& declaration : declarations)
    {
        generate_target(system, declaration, options, result, diagnostics);
    }

    return result;
}

void Generator::generate_target(
    const SystemDecl& system,
    const GenerateDecl& declaration,
    const GenerationOptions& options,
    GenerationResult& result,
    DiagnosticBag& diagnostics
) const
{
    static const std::unordered_set<std::string> supported_targets{
        "mt", "dl", "qu", "wf", "openapi", "proto", "docs", "tests", "all",
    };

    if (supported_targets.find(declaration.target) == supported_targets.end())
    {
        diagnostics.error(
            declaration.range, "SSPEC5003",
            "unsupported generate target '" + declaration.target + "'"
        );
        return;
    }

    if (declaration.target == "all")
    {
        for (const auto& target : {"mt", "dl", "qu", "wf", "openapi"})
        {
            auto selected = declaration;
            selected.target = target;
            generate_target(system, selected, options, result, diagnostics);
        }
    }
    else if (declaration.target == "openapi")
    {
        generator_backend::generate_openapi(system, declaration, options, result);
    }
    else
    {
        generator_backend::generate_scaffold(system, declaration, options, result);
    }
}

} // namespace statespec
