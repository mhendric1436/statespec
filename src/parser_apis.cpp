#include "statespec/parser.hpp"

#include "statespec/language_constants.hpp"

namespace statespec
{

ApiDecl Parser::parse_api_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordApi, "expected api declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected api name", diagnostics);
    ApiDecl api;
    api.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after api name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordMethod))
        {
            api.method = parse_simple_value(diagnostics, "HTTP method");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordPath))
        {
            api.path = parse_simple_value(diagnostics, "API path");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordInput))
        {
            api.input = parse_qualified_name(diagnostics, "API input");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordOutput))
        {
            api.output = parse_qualified_name(diagnostics, "API output");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordError))
        {
            api.error = parse_qualified_name(diagnostics, "API error");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordStarts))
        {
            consume(TokenKind::KeywordWorkflow, "expected workflow after starts", diagnostics);
            api.starts_workflow = parse_qualified_name(diagnostics, "started workflow");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordEnqueues))
        {
            api.enqueues = parse_qualified_name(diagnostics, "enqueued message");
            consume_optional_semicolon();
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after api block", diagnostics);

    api.range = SourceRange{start.range.begin, previous().range.end};
    return api;
}

PolicyDecl Parser::parse_policy_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordPolicy, "expected policy declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected policy name", diagnostics);
    PolicyDecl policy;
    policy.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after policy name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordTenant))
        {
            policy.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordTenant}, previous().range}
            );
            consume(TokenKind::KeywordScopedBy, "expected scoped_by after tenant", diagnostics);
            policy.tenant_scoped_by =
                consume(TokenKind::Identifier, "expected tenant scope field", diagnostics).lexeme;
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordAllow))
        {
            policy.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordAllow}, previous().range}
            );
            PolicyRuleDecl rule;
            const auto rule_start = previous();
            rule.action = parse_qualified_name(diagnostics, "allow action");
            if (match(TokenKind::KeywordWhen))
            {
                rule.condition = parse_simple_expression_until_boundary();
            }
            consume_optional_semicolon();
            rule.range = SourceRange{rule_start.range.begin, previous().range.end};
            policy.allows.push_back(rule);
        }
        else if (match(TokenKind::KeywordDeny))
        {
            policy.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordDeny}, previous().range}
            );
            PolicyRuleDecl rule;
            const auto rule_start = previous();
            rule.action = parse_qualified_name(diagnostics, "deny action");
            if (match(TokenKind::KeywordWhen))
            {
                rule.condition = parse_simple_expression_until_boundary();
            }
            consume_optional_semicolon();
            rule.range = SourceRange{rule_start.range.begin, previous().range.end};
            policy.denies.push_back(rule);
        }
        else if (match(TokenKind::KeywordQuota))
        {
            policy.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordQuota}, previous().range}
            );
            QuotaDecl quota;
            const auto quota_start = previous();
            quota.name = consume(TokenKind::Identifier, "expected quota name", diagnostics).lexeme;
            consume(TokenKind::Colon, "expected ':' after quota name", diagnostics);
            quota.expression = parse_simple_expression_until_boundary();
            consume_optional_semicolon();
            quota.range = SourceRange{quota_start.range.begin, previous().range.end};
            policy.quotas.push_back(quota);
        }
        else if (match(TokenKind::KeywordAudit))
        {
            policy.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordAudit}, previous().range}
            );
            policy.audits.push_back(parse_qualified_name(diagnostics, "audit action"));
            consume_optional_semicolon();
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after policy block", diagnostics);

    policy.range = SourceRange{start.range.begin, previous().range.end};
    return policy;
}

} // namespace statespec
