#pragma once

#include "statespec/diagnostic.hpp"
#include "statespec/token.hpp"

#include <cstddef>
#include <initializer_list>
#include <string>
#include <vector>

namespace statespec
{

class ParserContext
{
  protected:
    explicit ParserContext(std::vector<Token> tokens);

    const Token& peek(std::size_t offset = 0) const;
    const Token& previous() const;
    bool is_at_end() const;
    bool check(TokenKind kind) const;
    bool check_any(std::initializer_list<TokenKind> kinds) const;
    bool match(TokenKind kind);
    bool match_any(std::initializer_list<TokenKind> kinds);
    Token advance();
    Token consume(
        TokenKind kind,
        const std::string& message,
        DiagnosticBag& diagnostics
    );
    void rewind_one();

    void skip_unknown_declaration(DiagnosticBag& diagnostics);
    void skip_balanced_block();
    void synchronize_to_member_boundary();

  private:
    std::vector<Token> tokens_;
    std::size_t current_ = 0;
};

} // namespace statespec
