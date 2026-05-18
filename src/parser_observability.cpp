#include "statespec/parser.hpp"

#include "parser_helpers.hpp"

namespace statespec
{

using parser_detail::is_named_identifier;
using parser_detail::strip_quotes;

FeatureFlagDecl Parser::parse_feature_flag_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordFeatureFlag, "expected feature_flag declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected feature flag name", diagnostics);
    FeatureFlagDecl feature_flag;
    feature_flag.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after feature flag name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (is_named_identifier(peek(), "type"))
        {
            advance();
            feature_flag.type =
                consume(TokenKind::Identifier, "expected feature flag type", diagnostics).lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "default"))
        {
            advance();
            if (check_any({
                    TokenKind::StringLiteral,
                    TokenKind::DurationLiteral,
                    TokenKind::IntegerLiteral,
                    TokenKind::DecimalLiteral,
                    TokenKind::BooleanLiteral,
                    TokenKind::Identifier,
                }))
            {
                const auto value = advance();
                feature_flag.default_value = strip_quotes(value.lexeme);
                feature_flag.default_value_kind = token_kind_name(value.kind);
            }
            else
            {
                diagnostics.error(peek().range, "SSPEC0203", "expected feature flag default value");
            }
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "scope"))
        {
            advance();
            if (match(TokenKind::KeywordSystem))
            {
                feature_flag.scope = "system";
            }
            else if (match(TokenKind::KeywordTenant))
            {
                feature_flag.scope = "tenant";
            }
            else if (is_named_identifier(peek(), "user"))
            {
                advance();
                feature_flag.scope = "user";
            }
            else if (match(TokenKind::KeywordEntity))
            {
                feature_flag.scope =
                    "entity " + parse_qualified_name(diagnostics, "feature flag entity scope");
            }
            else
            {
                diagnostics.error(peek().range, "SSPEC0203", "expected feature flag scope");
            }
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "owner"))
        {
            advance();
            feature_flag.owner = parse_simple_value(diagnostics, "feature flag owner");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "description"))
        {
            advance();
            feature_flag.description = parse_simple_value(diagnostics, "feature flag description");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "expires"))
        {
            advance();
            feature_flag.expires = parse_simple_value(diagnostics, "feature flag expiration");
            consume_optional_semicolon();
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after feature flag block", diagnostics);

    feature_flag.range = SourceRange{start.range.begin, previous().range.end};
    return feature_flag;
}

LogDecl Parser::parse_log_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordLog, "expected log declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected log name", diagnostics);
    LogDecl log;
    log.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after log name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (is_named_identifier(peek(), "level"))
        {
            const auto level = advance();
            log.member_order.push_back(BlockMemberOrder{"level", level.range});
            log.level = consume(TokenKind::Identifier, "expected log level", diagnostics).lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "event_name"))
        {
            const auto event_name = advance();
            log.member_order.push_back(BlockMemberOrder{"event_name", event_name.range});
            log.event_name = parse_simple_value(diagnostics, "log event_name");
            consume_optional_semicolon();
        }
        else if (check(TokenKind::KeywordFields))
        {
            const auto fields = advance();
            log.member_order.push_back(BlockMemberOrder{"fields", fields.range});
            consume(TokenKind::LeftBrace, "expected '{' after log fields", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                log.fields.push_back(parse_field_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after log fields block", diagnostics);
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after log block", diagnostics);

    log.range = SourceRange{start.range.begin, previous().range.end};
    return log;
}

MetricDecl Parser::parse_metric_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordMetric, "expected metric declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected metric name", diagnostics);
    MetricDecl metric;
    metric.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after metric name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (is_named_identifier(peek(), "kind"))
        {
            const auto kind = advance();
            metric.member_order.push_back(BlockMemberOrder{"kind", kind.range});
            metric.kind =
                consume(TokenKind::Identifier, "expected metric kind", diagnostics).lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "name"))
        {
            const auto name_member = advance();
            metric.member_order.push_back(BlockMemberOrder{"name", name_member.range});
            metric.backend_name = parse_simple_value(diagnostics, "metric name");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "unit"))
        {
            const auto unit = advance();
            metric.member_order.push_back(BlockMemberOrder{"unit", unit.range});
            metric.unit = parse_simple_value(diagnostics, "metric unit");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "labels"))
        {
            const auto labels = advance();
            metric.member_order.push_back(BlockMemberOrder{"labels", labels.range});
            consume(TokenKind::LeftBrace, "expected '{' after metric labels", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                metric.labels.push_back(parse_field_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after metric labels block", diagnostics);
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after metric block", diagnostics);

    metric.range = SourceRange{start.range.begin, previous().range.end};
    return metric;
}

} // namespace statespec
