#pragma once

#include "statespec/source.hpp"

#include <string>
#include <vector>

namespace statespec {

enum class DiagnosticSeverity {
    Info,
    Warning,
    Error,
};

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    SourceRange range;
    std::string code;
    std::string message;
};

class DiagnosticBag {
public:
    void info(SourceRange range, std::string code, std::string message);
    void warning(SourceRange range, std::string code, std::string message);
    void error(SourceRange range, std::string code, std::string message);

    bool has_errors() const noexcept;
    const std::vector<Diagnostic>& all() const noexcept;

private:
    std::vector<Diagnostic> diagnostics_;
};

} // namespace statespec
