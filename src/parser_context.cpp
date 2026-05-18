#include "statespec/parser_context.hpp"

#include <utility>

namespace statespec
{

ParserContext::ParserContext(std::vector<Token> tokens)
    : tokens_(std::move(tokens))
{
}

const Token& ParserContext::peek(std::size_t offset) const
{
    const auto index = current_ + offset;
    if (index >= tokens_.size())
    {
        return tokens_.back();
    }
    return tokens_[index];
}

const Token& ParserContext::previous() const
{
    if (current_ == 0)
    {
        return tokens_.front();
    }
    return tokens_[current_ - 1];
}

bool ParserContext::is_at_end() const
{
    return peek().kind == TokenKind::EndOfFile;
}

bool ParserContext::check(TokenKind kind) const
{
    return !is_at_end() && peek().kind == kind;
}

bool ParserContext::check_any(std::initializer_list<TokenKind> kinds) const
{
    for (const auto kind : kinds)
    {
        if (check(kind))
        {
            return true;
        }
    }
    return false;
}

bool ParserContext::match(TokenKind kind)
{
    if (!check(kind))
    {
        return false;
    }
    advance();
    return true;
}

bool ParserContext::match_any(std::initializer_list<TokenKind> kinds)
{
    for (const auto kind : kinds)
    {
        if (match(kind))
        {
            return true;
        }
    }
    return false;
}

Token ParserContext::advance()
{
    if (!is_at_end())
    {
        ++current_;
    }
    return previous();
}

Token ParserContext::consume(
    TokenKind kind,
    const std::string& message,
    DiagnosticBag& diagnostics
)
{
    if (check(kind))
    {
        return advance();
    }

    diagnostics.error(peek().range, "SSPEC0200", message);
    return Token{kind, "", peek().range};
}

void ParserContext::rewind_one()
{
    if (current_ > 0)
    {
        --current_;
    }
}

void ParserContext::skip_unknown_declaration(DiagnosticBag& diagnostics)
{
    diagnostics.warning(
        peek().range, "SSPEC0299", "skipping unsupported declaration in current parser milestone"
    );

    if (check(TokenKind::LeftBrace))
    {
        skip_balanced_block();
        return;
    }

    advance();
    if (check(TokenKind::LeftBrace))
    {
        skip_balanced_block();
        return;
    }
    synchronize_to_member_boundary();
}

void ParserContext::skip_balanced_block()
{
    if (!match(TokenKind::LeftBrace))
    {
        return;
    }

    int depth = 1;
    while (depth > 0 && !is_at_end())
    {
        if (match(TokenKind::LeftBrace))
        {
            ++depth;
        }
        else if (match(TokenKind::RightBrace))
        {
            --depth;
        }
        else
        {
            advance();
        }
    }
}

void ParserContext::synchronize_to_member_boundary()
{
    while (!is_at_end())
    {
        if (match(TokenKind::Semicolon))
        {
            return;
        }
        if (check(TokenKind::RightBrace))
        {
            return;
        }
        advance();
    }
}

} // namespace statespec
