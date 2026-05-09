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

std::optional<std::string> target_scoped_output_root(
    const std::optional<std::string>& root,
    const std::string& target
)
{
    if (!root.has_value())
    {
        return std::nullopt;
    }
    return generator_backend::join_path(*root, target);
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
        "mt", "dl", "qu", "wf", "openapi", "proto", "docs", "tests",
    };

    if (declaration.target == "all")
    {
        for (const auto& target : {"mt", "dl", "qu", "wf", "openapi"})
        {
            GenerateDecl expanded = declaration;
            expanded.target = target;
            expanded.out = target_scoped_output_root(declaration.out, target);

            GenerationOptions expanded_options = options;
            expanded_options.target_override = target;
            expanded_options.out_override = target_scoped_output_root(options.out_override, target);

            generate_target(system, expanded, expanded_options, result, diagnostics);
        }
        return;
    }

    if (supported_targets.find(declaration.target) == supported_targets.end())
    {
        diagnostics.error(
            declaration.range, "SSPEC5003",
            "unsupported generate target '" + declaration.target + "'"
        );
        return;
    }

    if (declaration.target == "mt")
    {
        generator_backend::generate_mt(system, declaration, options, result);
    }
    else if (declaration.target == "dl")
    {
        generator_backend::generate_dl(system, declaration, options, result);
    }
    else if (declaration.target == "qu")
    {
        generator_backend::generate_qu(system, declaration, options, result);
    }
    else if (declaration.target == "wf")
    {
        generator_backend::generate_wf(system, declaration, options, result);
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
