#include "statespec/parser.hpp"

#include "parser_helpers.hpp"

#include <vector>

namespace statespec
{

using parser_detail::is_named_identifier;

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
            entity.key_fields = parse_identifier_list(diagnostics);
            consume_optional_semicolon();
        }
        else if (check(TokenKind::KeywordFields))
        {
            advance();
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
        }
        else if (check(TokenKind::KeywordOwnership))
        {
            entity.ownership = parse_ownership_decl(diagnostics);
        }
        else if (check(TokenKind::KeywordRelations))
        {
            advance();
            consume(TokenKind::LeftBrace, "expected '{' after relations", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                entity.relations.push_back(parse_relation_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after relations block", diagnostics);
        }
        else if (check(TokenKind::KeywordChildren))
        {
            advance();
            consume(TokenKind::LeftBrace, "expected '{' after children", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                entity.children.push_back(parse_child_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after children block", diagnostics);
        }
        else if (check(TokenKind::KeywordInvariants))
        {
            advance();
            consume(TokenKind::LeftBrace, "expected '{' after invariants", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                entity.invariants.push_back(parse_invariant_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after invariants block", diagnostics);
        }
        else if (check(TokenKind::KeywordIndexes))
        {
            advance();
            consume(TokenKind::LeftBrace, "expected '{' after indexes", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                entity.indexes.push_back(parse_index_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after indexes block", diagnostics);
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
        relation.kind = "parent";
    }
    else if (start.kind == TokenKind::KeywordRef)
    {
        relation.kind = "ref";
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
    if (relation.kind == "parent" && match(TokenKind::KeywordOptional))
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
            if (is_named_identifier(peek(), "kind"))
            {
                advance();
                consume(TokenKind::Colon, "expected ':' after relation kind", diagnostics);
                relation.relation_kind =
                    consume(TokenKind::Identifier, "expected relation kind", diagnostics).lexeme;
                consume_optional_semicolon();
            }
            else if (is_named_identifier(peek(), "on_parent_delete"))
            {
                advance();
                consume(TokenKind::Colon, "expected ':' after on_parent_delete", diagnostics);
                relation.on_parent_delete =
                    consume(TokenKind::Identifier, "expected parent delete behavior", diagnostics)
                        .lexeme;
                consume_optional_semicolon();
            }
            else if (is_named_identifier(peek(), "parent_must_be_in"))
            {
                advance();
                consume(TokenKind::Colon, "expected ':' after parent_must_be_in", diagnostics);
                relation.parent_must_be_in = parse_identifier_list(diagnostics);
                consume_optional_semicolon();
            }
            else if (is_named_identifier(peek(), "unique_within_parent"))
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
    if (by.lexeme != "by")
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
    const bool unique = kind.lexeme == "unique";
    if (kind.lexeme != "index" && kind.lexeme != "unique")
    {
        diagnostics.error(kind.range, "SSPEC0200", "expected 'index' or 'unique'");
    }

    const auto name = consume(TokenKind::Identifier, "expected index name", diagnostics);
    if (!is_named_identifier(peek(), "on") && !check(TokenKind::KeywordOn))
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
