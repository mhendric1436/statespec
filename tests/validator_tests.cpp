#include "test_support.hpp"

#include "catch2/catch_amalgamated.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/validator.hpp"

namespace
{

void validator_rejects_missing_system()
{
    statespec::Spec spec;
    statespec::DiagnosticBag diagnostics;
    statespec::Validator validator;
    validator.validate(spec, diagnostics);
    statespec::test::require(
        diagnostics.has_errors(), "validator should reject spec without system"
    );
}

} // namespace

TEST_CASE("validator rejects missing systems")
{
    validator_rejects_missing_system();
}
