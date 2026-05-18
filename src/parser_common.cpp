#include "statespec/parser.hpp"

#include "parser_helpers.hpp"

namespace statespec
{

using parser_detail::strip_quotes;

std::string Parser::parse_qualified_name(
    DiagnosticBag& diagnostics,
    const std::string& context
)
{
    auto name = consume(TokenKind::Identifier, "expected " + context, diagnostics).lexeme;
    while (match(TokenKind::Dot))
    {
        name += ".";
        name += consume(TokenKind::Identifier, "expected name after '.'", diagnostics).lexeme;
    }
    return name;
}

std::string Parser::parse_type_name(DiagnosticBag& diagnostics)
{
    if (!check_any({TokenKind::Identifier, TokenKind::KeywordOptional, TokenKind::KeywordRef}))
    {
        diagnostics.error(peek().range, "SSPEC0200", "expected type name");
        if (!is_at_end())
        {
            advance();
        }
        return "";
    }

    std::string type = advance().lexeme;

    if (match(TokenKind::Less))
    {
        type += "<";
        int depth = 1;
        while (depth > 0 && !is_at_end())
        {
            if (match(TokenKind::Less))
            {
                ++depth;
                type += "<";
            }
            else if (match(TokenKind::Greater))
            {
                --depth;
                type += ">";
            }
            else
            {
                type += advance().lexeme;
            }
        }
    }

    if (match(TokenKind::LeftBracket))
    {
        type += "[";
        consume(TokenKind::RightBracket, "expected ']' after array type", diagnostics);
        type += "]";
    }

    if (match(TokenKind::Question))
    {
        type += "?";
    }

    return type;
}

std::vector<std::string> Parser::parse_identifier_list(DiagnosticBag& diagnostics)
{
    std::vector<std::string> values;

    const bool bracketed = match(TokenKind::LeftBracket);
    values.push_back(consume(TokenKind::Identifier, "expected identifier", diagnostics).lexeme);
    while (match(TokenKind::Comma))
    {
        values.push_back(
            consume(TokenKind::Identifier, "expected identifier after ','", diagnostics).lexeme
        );
    }
    if (bracketed)
    {
        consume(TokenKind::RightBracket, "expected ']' after identifier list", diagnostics);
    }

    return values;
}

std::string Parser::parse_simple_value(
    DiagnosticBag& diagnostics,
    const std::string& context
)
{
    if (check(TokenKind::Identifier))
    {
        return parse_qualified_name(diagnostics, context);
    }
    if (match_any({
            TokenKind::StringLiteral,
            TokenKind::DurationLiteral,
            TokenKind::IntegerLiteral,
            TokenKind::DecimalLiteral,
            TokenKind::BooleanLiteral,
        }))
    {
        return strip_quotes(previous().lexeme);
    }

    diagnostics.error(peek().range, "SSPEC0203", "expected " + context);
    return "";
}

std::string Parser::parse_simple_expression_until_boundary()
{
    std::string expression;
    while (!is_at_end() && !check(TokenKind::Semicolon) && !check(TokenKind::RightBrace))
    {
        if (!expression.empty())
        {
            expression += ' ';
        }
        expression += previous().lexeme.empty() ? peek().lexeme : peek().lexeme;
        advance();
    }
    return expression;
}

std::string Parser::parse_expression_until(TokenKind kind)
{
    std::string expression;
    while (!is_at_end() && !check(kind) && !check(TokenKind::Semicolon) &&
           !check(TokenKind::RightBrace))
    {
        if (!expression.empty())
        {
            expression += ' ';
        }
        expression += peek().lexeme;
        advance();
    }
    return expression;
}

void Parser::consume_optional_semicolon()
{
    match(TokenKind::Semicolon);
}

} // namespace statespec
