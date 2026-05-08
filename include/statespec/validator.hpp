#pragma once

#include "statespec/ast.hpp"
#include "statespec/diagnostic.hpp"

namespace statespec
{

class Validator
{
  public:
    void validate(
        const Spec& spec,
        DiagnosticBag& diagnostics
    );
};

} // namespace statespec
