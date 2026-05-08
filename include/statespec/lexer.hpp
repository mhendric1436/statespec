#pragma once

#include "statespec/diagnostic.hpp"
#include "statespec/source.hpp"
#include "statespec/token.hpp"

#include <vector>

namespace statespec
{

class Lexer
{
  public:
    explicit Lexer(const SourceFile& source);

    std::vector<Token> lex(DiagnosticBag& diagnostics);

  private:
    const SourceFile& source_;
};

} // namespace statespec
