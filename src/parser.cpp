#include "statespec/parser.hpp"

#include "parser_helpers.hpp"

#include <string>
#include <utility>

namespace statespec
{

using parser_detail::is_named_identifier;
using parser_detail::strip_quotes;

Parser::Parser(std::vector<Token> tokens)
    : ParserContext(std::move(tokens))
{
}

Spec Parser::parse(DiagnosticBag& diagnostics)
{
    return parse_spec(diagnostics);
}

ValueDecl Parser::parse_value_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordValue, "expected value declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected value name", diagnostics);
    ValueDecl value;
    value.name = name.lexeme;

    consume(TokenKind::Colon, "expected ':' after value name", diagnostics);
    value.type = parse_type_name(diagnostics);
    if (check(TokenKind::KeywordWhere) || is_named_identifier(peek(), "where"))
    {
        advance();
        value.constraint = parse_simple_expression_until_boundary();
    }
    consume_optional_semicolon();

    value.range = SourceRange{start.range.begin, previous().range.end};
    return value;
}

EnumDecl Parser::parse_enum_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordEnum, "expected enum declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected enum name", diagnostics);
    EnumDecl enum_decl;
    enum_decl.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after enum name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        const auto member_start =
            consume(TokenKind::Identifier, "expected enum member name", diagnostics);
        EnumMemberDecl member;
        member.name = member_start.lexeme;
        if (match(TokenKind::Equals))
        {
            if (check_any({
                    TokenKind::StringLiteral,
                    TokenKind::IntegerLiteral,
                    TokenKind::DecimalLiteral,
                    TokenKind::BooleanLiteral,
                    TokenKind::Identifier,
                }))
            {
                const auto value = advance();
                member.value = strip_quotes(value.lexeme);
                member.value_kind = token_kind_name(value.kind);
            }
            else
            {
                diagnostics.error(peek().range, "SSPEC0200", "expected enum member value");
            }
        }
        consume_optional_semicolon();
        member.range = SourceRange{member_start.range.begin, previous().range.end};
        enum_decl.members.push_back(member);
    }
    consume(TokenKind::RightBrace, "expected '}' after enum block", diagnostics);

    enum_decl.range = SourceRange{start.range.begin, previous().range.end};
    return enum_decl;
}

EventDecl Parser::parse_event_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordEvent, "expected event declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected event name", diagnostics);
    EventDecl event;
    event.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after event name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (check(TokenKind::KeywordFields))
        {
            advance();
            consume(TokenKind::LeftBrace, "expected '{' after event fields", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                event.fields.push_back(parse_field_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after event fields block", diagnostics);
        }
        else if (check(TokenKind::KeywordAnnotations))
        {
            advance();
            skip_balanced_block();
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after event block", diagnostics);

    event.range = SourceRange{start.range.begin, previous().range.end};
    return event;
}

ExternalSystemDecl Parser::parse_external_system_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(
        TokenKind::KeywordExternalSystem, "expected external_system declaration", diagnostics
    );
    const auto name = consume(TokenKind::Identifier, "expected external_system name", diagnostics);
    ExternalSystemDecl external_system;
    external_system.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after external_system name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (check(TokenKind::KeywordAnnotations))
        {
            advance();
            skip_balanced_block();
            continue;
        }

        const auto property_start =
            consume(TokenKind::Identifier, "expected external_system property", diagnostics);
        ExternalSystemPropertyDecl property;
        property.name = property_start.lexeme;
        consume(TokenKind::Colon, "expected ':' after external_system property", diagnostics);
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
            property.value = strip_quotes(value.lexeme);
            property.value_kind = token_kind_name(value.kind);
        }
        else
        {
            diagnostics.error(peek().range, "SSPEC0200", "expected external_system property value");
        }
        consume_optional_semicolon();
        property.range = SourceRange{property_start.range.begin, previous().range.end};
        external_system.properties.push_back(std::move(property));
    }
    consume(TokenKind::RightBrace, "expected '}' after external_system block", diagnostics);

    external_system.range = SourceRange{start.range.begin, previous().range.end};
    return external_system;
}

ShapeDecl Parser::parse_shape_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordShape, "expected shape declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected shape name", diagnostics);
    ShapeDecl shape;
    shape.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after shape name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        shape.fields.push_back(parse_field_decl(diagnostics));
    }
    consume(TokenKind::RightBrace, "expected '}' after shape block", diagnostics);

    shape.range = SourceRange{start.range.begin, previous().range.end};
    return shape;
}

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
            advance();
            log.level = consume(TokenKind::Identifier, "expected log level", diagnostics).lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "event_name"))
        {
            advance();
            log.event_name = parse_simple_value(diagnostics, "log event_name");
            consume_optional_semicolon();
        }
        else if (check(TokenKind::KeywordFields))
        {
            advance();
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
            advance();
            metric.kind =
                consume(TokenKind::Identifier, "expected metric kind", diagnostics).lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "name"))
        {
            advance();
            metric.backend_name = parse_simple_value(diagnostics, "metric name");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "unit"))
        {
            advance();
            metric.unit = parse_simple_value(diagnostics, "metric unit");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "labels"))
        {
            advance();
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
