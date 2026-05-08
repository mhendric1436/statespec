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
        if (check(TokenKind::KeywordEntity))
        {
            system.entities.push_back(parse_entity_decl(diagnostics));
        }
        else if (check(TokenKind::KeywordWorkflow))
        {
            system.workflows.push_back(parse_workflow_decl(diagnostics));
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
        else
        {
            skip_unknown_declaration(diagnostics);
        }
    }
    consume(TokenKind::RightBrace, "expected '}' after entity block", diagnostics);

    entity.range = SourceRange{start.range.begin, previous().range.end};
    return entity;
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
        else if (check(TokenKind::Identifier) && peek().lexeme == "expected_execution_time")
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
        if (check(TokenKind::Identifier) && peek().lexeme == "expected_execution_time")
        {
            advance();
            const auto value =
                consume(TokenKind::DurationLiteral, "expected step duration", diagnostics);
            step.expected_execution_time = value.lexeme;
            consume_optional_semicolon();
        }
        else if (check(TokenKind::Identifier) && peek().lexeme == "max_retries")
        {
            advance();
            step.max_retries = parse_int_or_zero(
                consume(TokenKind::IntegerLiteral, "expected max_retries integer", diagnostics)
            );
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
    const auto first = consume(TokenKind::Identifier, "expected type name", diagnostics);
    std::string type = first.lexeme;

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
