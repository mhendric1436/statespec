#include "statespec/diagnostic.hpp"

#include <utility>

namespace statespec
{

void DiagnosticBag::info(
    SourceRange range,
    std::string code,
    std::string message
)
{
    diagnostics_.push_back(
        Diagnostic{DiagnosticSeverity::Info, range, std::move(code), std::move(message)}
    );
}

void DiagnosticBag::warning(
    SourceRange range,
    std::string code,
    std::string message
)
{
    diagnostics_.push_back(
        Diagnostic{DiagnosticSeverity::Warning, range, std::move(code), std::move(message)}
    );
}

void DiagnosticBag::error(
    SourceRange range,
    std::string code,
    std::string message
)
{
    diagnostics_.push_back(
        Diagnostic{DiagnosticSeverity::Error, range, std::move(code), std::move(message)}
    );
}

bool DiagnosticBag::has_errors() const noexcept
{
    for (const auto& diagnostic : diagnostics_)
    {
        if (diagnostic.severity == DiagnosticSeverity::Error)
        {
            return true;
        }
    }
    return false;
}

const std::vector<Diagnostic>& DiagnosticBag::all() const noexcept
{
    return diagnostics_;
}

} // namespace statespec
