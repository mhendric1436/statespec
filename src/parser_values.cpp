#include "statespec/parser.hpp"

#include "parser_helpers.hpp"
#include "statespec/language_constants.hpp"

#include <utility>

namespace statespec
{

using parser_detail::is_named_identifier;
using parser_detail::strip_quotes;

namespace
{

bool is_metadata_mapping_path_segment(TokenKind kind)
{
    return kind == TokenKind::Identifier || kind == TokenKind::KeywordEntity ||
           kind == TokenKind::KeywordEvent || kind == TokenKind::KeywordInput ||
           kind == TokenKind::KeywordWorkflow;
}

} // namespace

ValueDecl Parser::parse_value_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordValue, "expected value declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected value name", diagnostics);
    ValueDecl value;
    value.name = name.lexeme;

    consume(TokenKind::Colon, "expected ':' after value name", diagnostics);
    value.type = parse_type_name(diagnostics);
    if (check(TokenKind::KeywordWhere) || is_named_identifier(peek(), SyntaxKeywordWhere))
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
        if (is_named_identifier(peek(), SyntaxIdentifierMetadata))
        {
            auto metadata = parse_external_system_metadata_decl(diagnostics);
            if (external_system.metadata.has_value())
            {
                diagnostics.error(
                    metadata.range, "SSPEC0200", "duplicate external_system metadata block"
                );
            }
            else
            {
                external_system.metadata = std::move(metadata);
            }
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

ExternalSystemMetadataDecl Parser::parse_external_system_metadata_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::Identifier, "expected metadata block", diagnostics);
    ExternalSystemMetadataDecl metadata;

    consume(TokenKind::LeftBrace, "expected '{' after external_system metadata", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (check(TokenKind::KeywordEntity))
        {
            advance();
            metadata.entity = parse_qualified_name(diagnostics, "external_system metadata entity");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), SyntaxIdentifierProfileField))
        {
            advance();
            metadata.profile_field =
                consume(TokenKind::Identifier, "expected metadata profile_field", diagnostics)
                    .lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), SyntaxIdentifierRequiredFields))
        {
            advance();
            metadata.required_fields = parse_identifier_list(diagnostics);
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), SyntaxIdentifierMappings))
        {
            advance();
            consume(TokenKind::LeftBrace, "expected '{' after metadata mappings", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                metadata.mappings.push_back(
                    parse_external_system_metadata_mapping_decl(diagnostics)
                );
            }
            consume(
                TokenKind::RightBrace, "expected '}' after metadata mappings block", diagnostics
            );
        }
        else
        {
            diagnostics.error(
                peek().range, "SSPEC0200", "unexpected external_system metadata member"
            );
            advance();
        }
    }
    consume(
        TokenKind::RightBrace, "expected '}' after external_system metadata block", diagnostics
    );
    metadata.range = SourceRange{start.range.begin, previous().range.end};
    return metadata;
}

ExternalSystemMetadataMappingDecl
Parser::parse_external_system_metadata_mapping_decl(DiagnosticBag& diagnostics)
{
    const auto start = peek();
    ExternalSystemMetadataMappingDecl mapping;

    auto parse_mapping_path = [this, &diagnostics](const std::string& context)
    {
        if (!is_metadata_mapping_path_segment(peek().kind))
        {
            diagnostics.error(peek().range, "SSPEC0200", "expected " + context);
            if (!is_at_end())
            {
                advance();
            }
            return std::string{};
        }

        std::string name = advance().lexeme;
        while (match(TokenKind::Dot))
        {
            if (!is_metadata_mapping_path_segment(peek().kind))
            {
                diagnostics.error(peek().range, "SSPEC0200", "expected name after '.'");
                if (!is_at_end())
                {
                    advance();
                }
                break;
            }
            name += ".";
            name += advance().lexeme;
        }
        return name;
    };

    mapping.source = parse_mapping_path("metadata mapping source");
    consume(TokenKind::Arrow, "expected '->' in metadata mapping", diagnostics);
    mapping.target = parse_mapping_path("metadata mapping target");
    consume_optional_semicolon();
    mapping.range = SourceRange{start.range.begin, previous().range.end};
    return mapping;
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

} // namespace statespec
