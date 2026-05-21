#pragma once

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/lexer.hpp"
#include "statespec/parser.hpp"
#include "statespec/source.hpp"
#include "statespec/validator.hpp"

#include <string>
#include <utility>

namespace
{

[[maybe_unused]] void require(
    bool condition,
    const char* message
)
{
    INFO(message);
    REQUIRE(condition);
}

[[maybe_unused]] statespec::DiagnosticBag validate_text(const std::string& text)
{
    statespec::DiagnosticBag diagnostics;
    statespec::SourceFile source{"validator_test.sspec", text};
    statespec::Lexer lexer{source};
    auto tokens = lexer.lex(diagnostics);
    statespec::Parser parser{std::move(tokens)};
    auto spec = parser.parse(diagnostics);
    statespec::Validator validator;
    validator.validate(spec, diagnostics);
    return diagnostics;
}

[[maybe_unused]] bool has_error_code(
    const statespec::DiagnosticBag& diagnostics,
    const std::string& code
)
{
    for (const auto& diagnostic : diagnostics.all())
    {
        if (diagnostic.severity == statespec::DiagnosticSeverity::Error && diagnostic.code == code)
        {
            return true;
        }
    }
    return false;
}

[[maybe_unused]] bool has_error_message_containing(
    const statespec::DiagnosticBag& diagnostics,
    const std::string& fragment
)
{
    for (const auto& diagnostic : diagnostics.all())
    {
        if (diagnostic.severity == statespec::DiagnosticSeverity::Error &&
            diagnostic.message.find(fragment) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

[[maybe_unused]] bool has_warning_code(
    const statespec::DiagnosticBag& diagnostics,
    const std::string& code
)
{
    for (const auto& diagnostic : diagnostics.all())
    {
        if (diagnostic.severity == statespec::DiagnosticSeverity::Warning &&
            diagnostic.code == code)
        {
            return true;
        }
    }
    return false;
}

} // namespace
