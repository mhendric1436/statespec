#pragma once

#include "statespec/ast.hpp"
#include "statespec/diagnostic.hpp"
#include "statespec/token.hpp"

#include <vector>

namespace statespec {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    Spec parse(DiagnosticBag& diagnostics);

private:
    std::vector<Token> tokens_;
};

} // namespace statespec
