#include "statespec/generator_bindings.hpp"

#include <string>

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
        diagnostics.error(SourceRange{}, "SSPEC5101", "binding generation requires a system declaration");
        valid = false;
    }

    if (options.output_dir.empty())
    {
        diagnostics.error(SourceRange{}, "SSPEC5102", "binding generation requires an output directory");
        valid = false;
    }

    return valid;
}

GenerationResult language_generator_not_implemented(
    BindingLanguage language,
    DiagnosticBag& diagnostics
)
{
    diagnostics.error(
        SourceRange{}, "SSPEC5103",
        "binding generator for language '" + to_string(language) + "' is not implemented yet"
    );
    return GenerationResult{};
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

    switch (options.language)
    {
    case BindingLanguage::Cpp:
        return generate_cpp_bindings(spec, options, diagnostics);
    case BindingLanguage::Go:
        return generate_go_bindings(spec, options, diagnostics);
    case BindingLanguage::Java:
        return generate_java_bindings(spec, options, diagnostics);
    case BindingLanguage::Rust:
        return generate_rust_bindings(spec, options, diagnostics);
    }

    diagnostics.error(SourceRange{}, "SSPEC5104", "unsupported binding generator language");
    return GenerationResult{};
}

GenerationResult generate_cpp_bindings(
    const Spec&,
    const BindingGeneratorOptions&,
    DiagnosticBag& diagnostics
)
{
    return language_generator_not_implemented(BindingLanguage::Cpp, diagnostics);
}

GenerationResult generate_go_bindings(
    const Spec&,
    const BindingGeneratorOptions&,
    DiagnosticBag& diagnostics
)
{
    return language_generator_not_implemented(BindingLanguage::Go, diagnostics);
}

GenerationResult generate_java_bindings(
    const Spec&,
    const BindingGeneratorOptions&,
    DiagnosticBag& diagnostics
)
{
    return language_generator_not_implemented(BindingLanguage::Java, diagnostics);
}

GenerationResult generate_rust_bindings(
    const Spec&,
    const BindingGeneratorOptions&,
    DiagnosticBag& diagnostics
)
{
    return language_generator_not_implemented(BindingLanguage::Rust, diagnostics);
}

} // namespace statespec
