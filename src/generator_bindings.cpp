#include "statespec/generator_bindings.hpp"

namespace statespec
{

namespace
{

bool validate_binding_generation_request(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    bool valid = true;

    if (!spec.system.has_value())
    {
        diagnostics.error(
            SourceRange{}, "SSPEC5101", "binding generation requires a system declaration"
        );
        valid = false;
    }

    if (options.output_dir.empty())
    {
        diagnostics.error(
            SourceRange{}, "SSPEC5102", "binding generation requires an output directory"
        );
        valid = false;
    }

    return valid;
}

} // namespace

GenerationResult generate_bindings(
    const Spec& spec,
    const BindingGeneratorOptions& options,
    DiagnosticBag& diagnostics
)
{
    if (!validate_binding_generation_request(spec, options, diagnostics))
    {
        return GenerationResult{};
    }

    const auto system = lower_to_ir(spec);

    switch (options.language)
    {
    case BindingLanguage::Cpp:
        return generate_cpp_bindings(system, options, diagnostics);
    case BindingLanguage::Go:
        return generate_go_bindings(system, options, diagnostics);
    case BindingLanguage::Java:
        return generate_java_bindings(system, options, diagnostics);
    case BindingLanguage::Rust:
        return generate_rust_bindings(system, options, diagnostics);
    }

    diagnostics.error(SourceRange{}, "SSPEC5104", "unsupported binding generator language");
    return GenerationResult{};
}

} // namespace statespec
