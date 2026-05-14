#include "statespec/parser.hpp"

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>

namespace statespec
{

namespace
{

int parse_int_or_zero(const Token& token)
{
    try
    {
        return std::stoi(token.lexeme);
    }
    catch (const std::exception&)
    {
        return 0;
    }
}

std::string strip_quotes(const std::string& value)
{
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
    {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

bool is_named_identifier(
    const Token& token,
    const std::string& name
)
{
    return token.kind == TokenKind::Identifier && token.lexeme == name;
}

} // namespace

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens))
{
}

Spec Parser::parse(DiagnosticBag& diagnostics)
{
    return parse_spec(diagnostics);
}

const Token& Parser::peek(std::size_t offset) const
{
    const auto index = current_ + offset;
    if (index >= tokens_.size())
    {
        return tokens_.back();
    }
    return tokens_[index];
}

const Token& Parser::previous() const
{
    if (current_ == 0)
    {
        return tokens_.front();
    }
    return tokens_[current_ - 1];
}

bool Parser::is_at_end() const
{
    return peek().kind == TokenKind::EndOfFile;
}

bool Parser::check(TokenKind kind) const
{
    return !is_at_end() && peek().kind == kind;
}

bool Parser::check_any(std::initializer_list<TokenKind> kinds) const
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

bool Parser::match(TokenKind kind)
{
    if (!check(kind))
    {
        return false;
    }
    advance();
    return true;
}

bool Parser::match_any(std::initializer_list<TokenKind> kinds)
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

Token Parser::advance()
{
    if (!is_at_end())
    {
        ++current_;
    }
    return previous();
}

Token Parser::consume(
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

Spec Parser::parse_spec(DiagnosticBag& diagnostics)
{
    Spec spec;

    if (match(TokenKind::KeywordStatespec))
    {
        const auto version =
            consume(TokenKind::DecimalLiteral, "expected StateSpec version", diagnostics);
        spec.version = version.lexeme;
        consume(TokenKind::Semicolon, "expected ';' after StateSpec version", diagnostics);
    }

    while (match(TokenKind::KeywordImport))
    {
        --current_;
        spec.imports.push_back(parse_import_decl(diagnostics));
    }

    if (check(TokenKind::KeywordSystem))
    {
        spec.system = parse_system_decl(diagnostics);
    }
    else
    {
        diagnostics.error(peek().range, "SSPEC0201", "expected system declaration");
    }

    while (!is_at_end())
    {
        diagnostics.error(peek().range, "SSPEC0202", "unexpected token after system declaration");
        advance();
    }

    return spec;
}

ImportDecl Parser::parse_import_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordImport, "expected import declaration", diagnostics);
    ImportDecl import_decl;
    import_decl.name = parse_qualified_name(diagnostics, "import name");
    if (match(TokenKind::KeywordAs))
    {
        import_decl.alias =
            consume(TokenKind::Identifier, "expected import alias", diagnostics).lexeme;
    }
    consume(TokenKind::Semicolon, "expected ';' after import declaration", diagnostics);
    import_decl.range = SourceRange{start.range.begin, previous().range.end};
    return import_decl;
}

SystemDecl Parser::parse_system_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordSystem, "expected system declaration", diagnostics);
    auto name = consume(TokenKind::Identifier, "expected system name", diagnostics);
    SystemDecl system;
    system.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after system name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordTenant))
        {
            const auto tenant_start = previous();
            consume(TokenKind::KeywordScopedBy, "expected scoped_by after tenant", diagnostics);
            TenantScopeDecl tenant_scope;
            tenant_scope.field_name =
                consume(TokenKind::Identifier, "expected tenant scope field", diagnostics).lexeme;
            tenant_scope.range = SourceRange{tenant_start.range.begin, previous().range.end};
            system.tenant_scope = tenant_scope;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "system_tenant"))
        {
            const auto system_tenant_start = advance();
            if (!is_named_identifier(peek(), "configured"))
            {
                diagnostics.error(
                    peek().range, "SSPEC0200", "expected configured after system_tenant"
                );
            }
            else
            {
                advance();
            }
            SystemTenantDecl system_tenant;
            system_tenant.range =
                SourceRange{system_tenant_start.range.begin, previous().range.end};
            system.system_tenant = system_tenant;
            consume_optional_semicolon();
        }
        else if (check(TokenKind::KeywordEntity))
        {
            system.entities.push_back(parse_entity_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordFeatureFlag))
        {
            system.feature_flags.push_back(parse_feature_flag_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordQueue))
        {
            system.queues.push_back(parse_queue_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordLease))
        {
            system.leases.push_back(parse_lease_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordWorker))
        {
            system.workers.push_back(parse_worker_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordApi))
        {
            system.apis.push_back(parse_api_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordWorkflow))
        {
            system.workflows.push_back(parse_workflow_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordPolicy))
        {
            system.policies.push_back(parse_policy_decl(diagnostics));
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after system block", diagnostics);

    system.range = SourceRange{start.range.begin, previous().range.end};
    return system;
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
    if (!is_named_identifier(peek(), "on"))
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

StateMachineDecl Parser::parse_state_machine_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordStateMachine, "expected state_machine block", diagnostics);
    StateMachineDecl state_machine;
    consume(TokenKind::LeftBrace, "expected '{' after state_machine", diagnostics);

    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordState))
        {
            const auto name = consume(TokenKind::Identifier, "expected state name", diagnostics);
            StateDecl state;
            state.name = name.lexeme;
            state.range = name.range;
            if (check(TokenKind::LeftBrace))
            {
                skip_balanced_block();
            }
            consume_optional_semicolon();
            state_machine.states.push_back(state);
        }
        else if (match(TokenKind::KeywordInitial))
        {
            state_machine.initial_state =
                consume(TokenKind::Identifier, "expected initial state", diagnostics).lexeme;
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordTerminal))
        {
            if (check(TokenKind::LeftBracket))
            {
                state_machine.terminal_states = parse_identifier_list(diagnostics);
            }
            else
            {
                state_machine.terminal_states.push_back(
                    consume(TokenKind::Identifier, "expected terminal state", diagnostics).lexeme
                );
            }
            consume_optional_semicolon();
        }
        else if (check(TokenKind::Identifier) && peek(1).kind == TokenKind::Arrow)
        {
            const auto from = advance();
            advance();
            const auto to =
                consume(TokenKind::Identifier, "expected transition target state", diagnostics);
            TransitionDecl transition;
            transition.from = from.lexeme;
            transition.to = to.lexeme;
            transition.range = SourceRange{from.range.begin, to.range.end};
            if (check(TokenKind::LeftBrace))
            {
                skip_balanced_block();
            }
            consume_optional_semicolon();
            state_machine.transitions.push_back(transition);
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }

    consume(TokenKind::RightBrace, "expected '}' after state_machine block", diagnostics);
    state_machine.range = SourceRange{start.range.begin, previous().range.end};
    return state_machine;
}

WorkflowDecl Parser::parse_workflow_decl(DiagnosticBag& diagnostics)
{
    const auto start =
        consume(TokenKind::KeywordWorkflow, "expected workflow declaration", diagnostics);
    auto name = consume(TokenKind::Identifier, "expected workflow name", diagnostics);
    WorkflowDecl workflow;
    workflow.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after workflow name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (match(TokenKind::KeywordVersion))
        {
            workflow.version = parse_int_or_zero(
                consume(TokenKind::IntegerLiteral, "expected workflow version integer", diagnostics)
            );
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordSingleton))
        {
            const auto value =
                consume(TokenKind::BooleanLiteral, "expected singleton boolean", diagnostics);
            workflow.singleton = value.lexeme == "true";
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "expected_execution_time"))
        {
            advance();
            auto value =
                consume(TokenKind::DurationLiteral, "expected workflow duration", diagnostics);
            workflow.expected_execution_time = value.lexeme;
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordStart))
        {
            workflow.start_step =
                consume(TokenKind::Identifier, "expected start step name", diagnostics).lexeme;
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordLoad))
        {
            parse_qualified_name(diagnostics, "workflow load entity");
            if (!is_named_identifier(peek(), "by"))
            {
                diagnostics.error(peek().range, "SSPEC0200", "expected by in workflow load");
            }
            else
            {
                advance();
            }
            consume(TokenKind::Identifier, "expected workflow load key", diagnostics);
            consume(TokenKind::KeywordAs, "expected as in workflow load", diagnostics);
            consume(TokenKind::Identifier, "expected workflow load binding", diagnostics);
            consume_optional_semicolon();
        }
        else if (check(TokenKind::KeywordStep))
        {
            workflow.steps.push_back(parse_workflow_step_decl(diagnostics));
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after workflow block", diagnostics);

    workflow.range = SourceRange{start.range.begin, previous().range.end};
    return workflow;
}

WorkflowStepDecl Parser::parse_workflow_step_decl(DiagnosticBag& diagnostics)
{
    const auto start = consume(TokenKind::KeywordStep, "expected workflow step", diagnostics);
    const auto name = consume(TokenKind::Identifier, "expected workflow step name", diagnostics);
    WorkflowStepDecl step;
    step.name = name.lexeme;

    consume(TokenKind::LeftBrace, "expected '{' after workflow step name", diagnostics);
    while (!check(TokenKind::RightBrace) && !is_at_end())
    {
        if (is_named_identifier(peek(), "expected_execution_time"))
        {
            advance();
            const auto value =
                consume(TokenKind::DurationLiteral, "expected step duration", diagnostics);
            step.expected_execution_time = value.lexeme;
            consume_optional_semicolon();
        }
        else if (is_named_identifier(peek(), "max_retries"))
        {
            advance();
            step.max_retries = parse_int_or_zero(
                consume(TokenKind::IntegerLiteral, "expected max_retries integer", diagnostics)
            );
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordRequire))
        {
            parse_simple_expression_until_boundary();
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordWhen))
        {
            while (!is_at_end() && !check(TokenKind::LeftBrace) && !check(TokenKind::RightBrace))
            {
                advance();
            }
            if (check(TokenKind::LeftBrace))
            {
                skip_balanced_block();
            }
        }
        else if (match(TokenKind::KeywordTransitionTo))
        {
            consume(TokenKind::Identifier, "expected transition_to target", diagnostics);
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordComplete))
        {
            consume_optional_semicolon();
        }
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after workflow step block", diagnostics);

    step.range = SourceRange{start.range.begin, previous().range.end};
    return step;
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
            consume(TokenKind::KeywordScopedBy, "expected scoped_by after tenant", diagnostics);
            policy.tenant_scoped_by =
                consume(TokenKind::Identifier, "expected tenant scope field", diagnostics).lexeme;
            consume_optional_semicolon();
        }
        else if (match(TokenKind::KeywordAllow))
        {
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

void Parser::consume_optional_semicolon()
{
    match(TokenKind::Semicolon);
}

void Parser::skip_unknown_declaration(DiagnosticBag& diagnostics)
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

void Parser::skip_balanced_block()
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

void Parser::synchronize_to_member_boundary()
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
