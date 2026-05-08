#include "statespec/validator.hpp"

namespace statespec {

void Validator::validate(const Spec& spec, DiagnosticBag& diagnostics)
{
    if (!spec.system.has_value())
    {
        diagnostics.error(SourceRange{}, "SSPEC1001", "spec must contain a system declaration");
    }
}

} // namespace statespec
