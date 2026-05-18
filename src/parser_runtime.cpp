#include "statespec/parser.hpp"

#include "parser_helpers.hpp"

namespace statespec
{

using parser_detail::is_named_identifier;
using parser_detail::parse_int_or_zero;

QueueDecl Parser::parse_queue_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordQueue, "expected queue declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected queue name", diagnostics);
    QueueDecl queue;
    queue.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after queue name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordNamespace))
        {
            queue.namespace_name = parse_simple_value(diagnostics, "queue namespace");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "channel"))
        {
            advance();
            queue.channel = parse_simple_value(diagnostics, "queue channel");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "visibility_timeout"))
        {
            advance();
            queue.visibility_timeout = parse_simple_value(diagnostics, "visibility timeout");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "max_attempts"))
        {
            advance();
            queue.max_attempts = parse_int_or_zero(
                consume(TokenKind::IntegerLiteral, "expected max_attempts integer", diagnostics)
            );
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "dead_letter"))
        {
            advance();
            queue.dead_letter = parse_qualified_name(diagnostics, "dead_letter target");
            consume_optional_semicolon();
        }
        else if (check(TokenKind::KeywordMessage))
        {
            queue.messages.push_back(parse_message_decl(diagnostics));
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after queue block", diagnostics);

    queue.range = SourceRange{start.range.begin, previous().range.end};
    return queue;
}

MessageDecl Parser::parse_message_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordMessage, "expected message declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected message name", diagnostics);
    MessageDecl message;
    message.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after message name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (is_named_identifier(peek(), "idempotency_key"))
        {
            advance();
            message.idempotency_key =
                consume(TokenKind::Identifier, "expected idempotency key field", diagnostics)
                    .lexeme;
            consume_optional_semicolon();
        }
        else if (check(TokenKind::KeywordPayload))
        {
            advance();
            consume(TokenKind::LeftBrace, "expected '{' after payload", diagnostics);
            while (!check(TokenKind::RightBrace) && !is_at_end())
            {
                message.payload_fields.push_back(parse_field_decl(diagnostics));
            }
            consume(TokenKind::RightBrace, "expected '}' after payload block", diagnostics);
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after message block", diagnostics);

    message.range = SourceRange{start.range.begin, previous().range.end};
    return message;
}

LeaseDecl Parser::parse_lease_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordLease, "expected lease declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected lease name", diagnostics);
    LeaseDecl lease;
    lease.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after lease name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (is_named_identifier(peek(), "resource"))
        {
            advance();
            lease.resource = parse_simple_value(diagnostics, "lease resource");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "ttl"))
        {
            advance();
            lease.ttl = parse_simple_value(diagnostics, "lease ttl");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "renew_every"))
        {
            advance();
            lease.renew_every = parse_simple_value(diagnostics, "lease renew_every");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "holder"))
        {
            advance();
            lease.holder = parse_simple_value(diagnostics, "lease holder");
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "fencing_token"))
        {
            advance();
            const auto value =
                consume(TokenKind::BooleanLiteral, "expected fencing_token boolean", diagnostics);
            lease.fencing_token = value.lexeme == "true";
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "max_ttl"))
        {
            advance();
            lease.max_ttl = parse_simple_value(diagnostics, "lease max_ttl");
            consume_optional_semicolon();
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after lease block", diagnostics);

    lease.range = SourceRange{start.range.begin, previous().range.end};
    return lease;
}

WorkerDecl Parser::parse_worker_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordWorker, "expected worker declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected worker name", diagnostics);
    WorkerDecl worker;
    worker.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after worker name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordSingleton))
        {
            const auto value =
                consume(TokenKind::BooleanLiteral, "expected singleton boolean", diagnostics);
            worker.singleton = value.lexeme == "true";
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordLease))
        {
            worker.lease = parse_qualified_name(diagnostics, "worker lease");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordPolls))
        {
            worker.polls = parse_qualified_name(diagnostics, "worker polls target");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordExecutes))
        {
            worker.executes = parse_qualified_name(diagnostics, "worker executes target");
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordConcurrency))
        {
            worker.concurrency = parse_int_or_zero(
                consume(TokenKind::IntegerLiteral, "expected concurrency integer", diagnostics)
            );
            consume_optional_semicolon();
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after worker block", diagnostics);

    worker.range = SourceRange{start.range.begin, previous().range.end};
    return worker;
}

ApiServerDecl Parser::parse_api_server_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordApiServer, "expected api_server declaration", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected api_server name", diagnostics);
    ApiServerDecl api_server;
    api_server.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after api_server name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordServes))
        {
            api_server.serves.push_back(parse_qualified_name(diagnostics, "served API"));
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordConcurrency))
        {
            api_server.concurrency = parse_int_or_zero(
                consume(TokenKind::IntegerLiteral, "expected concurrency integer", diagnostics)
            );
            consume_optional_semicolon();
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after api_server block", diagnostics);

    api_server.range = SourceRange{start.range.begin, previous().range.end};
    return api_server;
}

} // namespace statespec
