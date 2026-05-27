#include "statespec/parser.hpp"

#include "parser_helpers.hpp"
#include "statespec/language_constants.hpp"

#include <vector>

namespace statespec
{

using parser_detail::is_named_identifier;

namespace
{

bool is_entity_api_member_identifier(const Token& token)
{
    return is_named_identifier(token, SyntaxIdentifierResource) ||
           is_named_identifier(token, SyntaxIdentifierGet) ||
           is_named_identifier(token, SyntaxIdentifierList) ||
           is_named_identifier(token, SyntaxIdentifierUpdateStatus) ||
           is_named_identifier(token, SyntaxIdentifierDelete);
}

bool can_parse_entity_api_override_name(const Token& token)
{
    return token.kind == TokenKind::Identifier && !is_entity_api_member_identifier(token);
}

} // namespace

EntityDecl Parser::parse_entity_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordEntity, "expected entity declaration", diagnostics);
    auto name = consume(TokenKind::Identifier, "expected entity name", diagnostics);
    EntityDecl entity;
    entity.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after entity name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordKey))
        {
            entity.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordKey}, previous().range}
            );
            entity.key_fields = parse_identifier_list(diagnostics);
            consume_optional_semicolon();
        }
        else if (check(TokenKind::KeywordFields))
        {
            const auto fields_start = advance();
            entity.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordFields}, fields_start.range}
            );
            consume(TokenKind::LeftBrace, "expected '{' after fields", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                entity.fields.push_back(parse_field_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after fields block", diagnostics);
        }
        else if (check(TokenKind::KeywordStateMachine))
        {
            entity.state_machine = parse_state_machine_decl(diagnostics);
            entity.member_order.push_back(
                BlockMemberOrder{
                    std::string{SyntaxKeywordStateMachine}, entity.state_machine->range
                }
            );
        }
        else if (check(TokenKind::KeywordOwnership))
        {
            entity.ownership = parse_ownership_decl(diagnostics);
            entity.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordOwnership}, entity.ownership->range}
            );
        }
        else if (check(TokenKind::KeywordRelations))
        {
            const auto relations_start = advance();
            entity.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordRelations}, relations_start.range}
            );
            consume(TokenKind::LeftBrace, "expected '{' after relations", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                entity.relations.push_back(parse_relation_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after relations block", diagnostics);
        }
        else if (check(TokenKind::KeywordChildren))
        {
            const auto children_start = advance();
            entity.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordChildren}, children_start.range}
            );
            consume(TokenKind::LeftBrace, "expected '{' after children", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                entity.children.push_back(parse_child_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after children block", diagnostics);
        }
        else if (check(TokenKind::KeywordInvariants))
        {
            const auto invariants_start = advance();
            entity.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordInvariants}, invariants_start.range}
            );
            consume(TokenKind::LeftBrace, "expected '{' after invariants", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                entity.invariants.push_back(parse_invariant_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after invariants block", diagnostics);
        }
        else if (check(TokenKind::KeywordIndexes))
        {
            const auto indexes_start = advance();
            entity.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordIndexes}, indexes_start.range}
            );
            consume(TokenKind::LeftBrace, "expected '{' after indexes", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                entity.indexes.push_back(parse_index_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after indexes block", diagnostics);
        }
        else if (check(TokenKind::KeywordApi))
        {
            entity.api = parse_entity_api_decl(diagnostics);
            entity.member_order.push_back(
                BlockMemberOrder{std::string{SyntaxKeywordApi}, entity.api->range}
            );
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after entity block", diagnostics);

    entity.range = SourceRange{start.range.begin, previous().range.end};
    return entity;
}

OwnershipDecl Parser::parse_ownership_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordOwnership, "expected ownership", diagnostics);
    OwnershipDecl ownership;

    consume(TokenKind::LeftBrace, "expected '{' after ownership", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordAuthority))
        {
            consume(TokenKind::Colon, "expected ':' after authority", diagnostics);
            if (match(TokenKind::KeywordSystem) || match(TokenKind::Identifier))
            {
                ownership.authority = previous().lexeme;
            }
            else
            {
                diagnostics.error(peek().range, "SSPEC0200", "expected ownership authority");
            }
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordSystemOfRecord))
        {
            consume(TokenKind::Colon, "expected ':' after system_of_record", diagnostics);
            ownership.system_of_record = parse_qualified_name(diagnostics, "system_of_record");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordLifecycle))
        {
            consume(TokenKind::Colon, "expected ':' after lifecycle", diagnostics);
            ownership.lifecycle =
                consume(TokenKind::Identifier, "expected lifecycle mode", diagnostics).lexeme;
            consume_optional_semicolon();
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after ownership block", diagnostics);

    ownership.range = SourceRange{start.range.begin, previous().range.end};
    return ownership;
}

RelationDecl Parser::parse_relation_decl(DiagnosticBag& diagnostics)
{
    const auto start = advance();
    RelationDecl relation;
    if (start.kind == TokenKind::KeywordParent)
    {
        relation.kind = std::string{SyntaxKeywordParent};
    }
    else if (start.kind == TokenKind::KeywordRef)
    {
        relation.kind = std::string{SyntaxKeywordRef};
    }
    else
    {
        diagnostics.error(start.range, "SSPEC0200", "expected relation declaration");
        synchronize_to_member_boundary();
        relation.range = start.range;
        return relation;
    }

    relation.name = consume(TokenKind::Identifier, "expected relation name", diagnostics).lexeme;
    consume(TokenKind::Colon, "expected ':' after relation name", diagnostics);
    if (relation.kind == SyntaxKeywordParent && match(TokenKind::KeywordOptional))
    {
        relation.optional = true;
    }
    relation.target = parse_type_name(diagnostics);
    if (!relation.target.empty() && relation.target.back() == '?')
    {
        relation.optional = true;
    }
    if (match(TokenKind::Question))
    {
        relation.optional = true;
        relation.target += "?";
    }

    if (match(TokenKind::LeftBrace))
    {
        while (!check(TokenKind::RightBrace) && !is_at_end())
        {
            if (is_named_identifier(peek(), SyntaxIdentifierKind))
            {
                advance();
                consume(TokenKind::Colon, "expected ':' after relation kind", diagnostics);
                relation.relation_kind =
                    consume(TokenKind::Identifier, "expected relation kind", diagnostics).lexeme;
                consume_optional_semicolon();
            }
            else if (is_named_identifier(peek(), SyntaxIdentifierOnParentDelete))
            {
                advance();
                consume(TokenKind::Colon, "expected ':' after on_parent_delete", diagnostics);
                relation.on_parent_delete =
                    consume(TokenKind::Identifier, "expected parent delete behavior", diagnostics)
                        .lexeme;
                consume_optional_semicolon();
            }
            else if (is_named_identifier(peek(), SyntaxIdentifierParentMustBeIn))
            {
                advance();
                consume(TokenKind::Colon, "expected ':' after parent_must_be_in", diagnostics);
                relation.parent_must_be_in = parse_identifier_list(diagnostics);
                consume_optional_semicolon();
            }
            else if (is_named_identifier(peek(), SyntaxIdentifierUniqueWithinParent))
            {
                advance();
                consume(TokenKind::Colon, "expected ':' after unique_within_parent", diagnostics);
                relation.unique_within_parent = parse_identifier_list(diagnostics);
                consume_optional_semicolon();
            }
            else
            {
                skip_unknown_declaration(diagnostics);
            }
        }
        consume(TokenKind::RightBrace, "expected '}' after relation options", diagnostics);
    }
    consume_optional_semicolon();

    relation.range = SourceRange{start.range.begin, previous().range.end};
    return relation;
}

ChildDecl Parser::parse_child_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::Identifier, "expected child collection name", diagnostics);
    ChildDecl child;
    child.name = start.lexeme;
    consume(TokenKind::Colon, "expected ':' after child collection name", diagnostics);
    child.target_entity = parse_qualified_name(diagnostics, "child entity");
    const auto by = consume(TokenKind::Identifier, "expected by in child collection", diagnostics);
    if (by.lexeme != SyntaxIdentifierBy)
    {
        diagnostics.error(by.range, "SSPEC0200", "expected by in child collection");
    }
    child.relation =
        consume(TokenKind::Identifier, "expected child relation name", diagnostics).lexeme;
    consume_optional_semicolon();
    child.range = SourceRange{start.range.begin, previous().range.end};
    return child;
}

InvariantDecl Parser::parse_invariant_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::Identifier, "expected invariant name", diagnostics);
    InvariantDecl invariant;
    invariant.name = start.lexeme;
    consume(TokenKind::Colon, "expected ':' after invariant name", diagnostics);
    invariant.expression = parse_simple_expression_until_boundary();
    consume_optional_semicolon();
    invariant.range = SourceRange{start.range.begin, previous().range.end};
    return invariant;
}

EntityApiDecl Parser::parse_entity_api_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordApi, "expected entity api block", diagnostics);
    EntityApiDecl api;

    consume(TokenKind::LeftBrace, "expected '{' after entity api", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (is_named_identifier(peek(), SyntaxIdentifierResource))
        {
            advance();
            api.resource = parse_simple_value(diagnostics, "entity API resource path");
            consume_optional_semicolon();
        }
        else if (check(TokenKind::KeywordCreate))
        {
            api.create = parse_entity_api_create_decl(diagnostics);
        }
        else if (is_named_identifier(peek(), SyntaxIdentifierGet))
        {
            api.get = parse_entity_api_operation_decl(diagnostics, SyntaxIdentifierGet);
        }
        else if (is_named_identifier(peek(), SyntaxIdentifierList))
        {
            api.lists.push_back(parse_entity_api_list_decl(diagnostics));
        }
        else if (is_named_identifier(peek(), SyntaxIdentifierUpdateStatus))
        {
            api.update_status =
                parse_entity_api_operation_decl(diagnostics, SyntaxIdentifierUpdateStatus);
        }
        else if (is_named_identifier(peek(), SyntaxIdentifierDelete))
        {
            api.delete_ = parse_entity_api_operation_decl(diagnostics, SyntaxIdentifierDelete);
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after entity api block", diagnostics);

    api.range = SourceRange{start.range.begin, previous().range.end};
    return api;
}

EntityApiCreateDecl Parser::parse_entity_api_create_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordCreate, "expected create", diagnostics);
    EntityApiCreateDecl create;

    if (can_parse_entity_api_override_name(peek()))
    {
        create.name = advance().lexeme;
    }

    if (match(TokenKind::LeftBrace))
    {
        while (!check(TokenKind::RightBrace) && !is_at_end())
        {
            if (match(TokenKind::KeywordFields))
            {
                create.fields = parse_identifier_list(diagnostics);
                consume_optional_semicolon();
            }
            else
            {
                skip_unknown_declaration(diagnostics);
            }
        }
        consume(TokenKind::RightBrace, "expected '}' after create block", diagnostics);
    }
    consume_optional_semicolon();

    create.range = SourceRange{start.range.begin, previous().range.end};
    return create;
}

EntityApiOperationDecl Parser::parse_entity_api_operation_decl(
    DiagnosticBag& diagnostics,
    std::string_view operation
)
{
    const auto start = consume(TokenKind::Identifier, "expected entity api operation", diagnostics);
    EntityApiOperationDecl operation_decl;
    if (start.lexeme != operation)
    {
        diagnostics.error(start.range, "SSPEC0200", "expected entity api operation");
    }

    if (can_parse_entity_api_override_name(peek()))
    {
        operation_decl.name = advance().lexeme;
    }
    consume_optional_semicolon();

    operation_decl.range = SourceRange{start.range.begin, previous().range.end};
    return operation_decl;
}

EntityApiListDecl Parser::parse_entity_api_list_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::Identifier, "expected list declaration", diagnostics);
    EntityApiListDecl list;

    if (can_parse_entity_api_override_name(peek()))
    {
        list.name = advance().lexeme;
    }

    consume(TokenKind::LeftBrace, "expected '{' after list declaration", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordPath))
        {
            list.path = parse_simple_value(diagnostics, "entity API list path");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), SyntaxIdentifierBy))
        {
            advance();
            list.by = parse_identifier_list(diagnostics);
            consume_optional_semicolon();
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after list block", diagnostics);

    list.range = SourceRange{start.range.begin, previous().range.end};
    return list;
}

FieldDecl Parser::parse_field_decl(DiagnosticBag& diagnostics)
{
    const auto name = consume(TokenKind::Identifier, "expected field name", diagnostics);
    FieldDecl field;
    field.name = name.lexeme;
    field.type = parse_type_name(diagnostics);

    if (check(TokenKind::LeftBrace))
    {
        skip_balanced_block();
    }

    if (match(TokenKind::KeywordWhere))
    {
        synchronize_to_member_boundary();
    }

    consume_optional_semicolon();
    field.range = SourceRange{name.range.begin, previous().range.end};
    return field;
}

IndexDecl Parser::parse_index_decl(DiagnosticBag& diagnostics)
{
    const auto kind = consume(TokenKind::Identifier, "expected index kind", diagnostics);
    const bool unique = kind.lexeme == SyntaxIdentifierUnique;
    if (kind.lexeme != SyntaxIdentifierIndex && kind.lexeme != SyntaxIdentifierUnique)
    {
        diagnostics.error(kind.range, "SSPEC0200", "expected 'index' or 'unique'");
    }

    const auto name = consume(TokenKind::Identifier, "expected index name", diagnostics);
    if (!is_named_identifier(peek(), SyntaxKeywordOn) && !check(TokenKind::KeywordOn))
    {
        diagnostics.error(peek().range, "SSPEC0200", "expected 'on' after index name");
    }
    else
    {
        advance();
    }

    std::vector<std::string> fields;
    fields.push_back(consume(TokenKind::Identifier, "expected index field", diagnostics).lexeme);
    while (match(TokenKind::Comma))
    {
        fields.push_back(
            consume(TokenKind::Identifier, "expected index field after ','", diagnostics).lexeme
        );
    }

    if (check(TokenKind::LeftBrace))
    {
        skip_balanced_block();
    }

    consume_optional_semicolon();
    return IndexDecl{
        name.lexeme, fields, unique, SourceRange{kind.range.begin, previous().range.end}
    };
}

} // namespace statespec
